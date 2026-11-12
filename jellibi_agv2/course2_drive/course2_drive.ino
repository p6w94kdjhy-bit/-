// =====================================================================
//  종목② 팔레트 운반 + 장애물 (제한 180초) — 종목①과 같은 주행 방식으로 다시 짠 판
//   배송 ① A2 → H6, ② A6 → H2 (한 번에 1개, 순서는 자동), 두 배송 후 H4 복귀 2초 정지.
//   2열·4열·6열의 D 교차점(D2 D4 D6) 중 2곳이 막힘 → 전방 IR 로 찾아서 열린 통로 1곳만 사용.
//
//  주행: 종목①과 똑같다 — 선 따라 달리기(LT_KP), 교차점 정렬(ALIGN_MS), 제자리 회전(TURN_*). 값은 config.h 그대로.
//
//  흐름
//   ① 통로 찾기 : H4 출발 → D4 → D2 → D6 순서로 확인. 4열로 북쪽으로 가다 전방 거리센서(IR)가 장애물을 보면 정지
//                 → G행에서 앞으로 간 칸 수만큼 '교차점을 세며 후진' → G행 교차점에 다시 정렬 → 다른 통로로
//                 (2곳이 막힌 걸 알면 남은 1곳은 확인 없이 열린 통로로 사용)
//   ② 배송 2건 : 적재함 밑 교차점(A2/A6)까지 → 정렬 → 리프트 올림 → 든 채로 B행까지 후진
//                → 열린 통로로 남쪽 → 도착 교차점(H6/H2) 정렬 → 리프트 내림 → G행까지 후진(적재함 밑에서 빠져나옴)
//   ③ 복귀     : H4 정렬 → 2초 정지
//
//  주행 기록: 주행 후 USB 를 꽂고 시리얼 모니터(115200)를 열면 '지난 주행 기록'이 나온다 → 복사해서 전달
//  ※ 맵(파란 통로)·출발 위치는 현장 확인 필요 — 아래 '맵' 부분만 고치면 된다.
// =====================================================================
#include <AGV2.h>

// ----------------------------------------------------------------- 맵   (운영안 11-09 그림과 일치, '변경될 수 있음')
//  행: A(0, 북쪽) ~ H(7, 남쪽)   열: 1(0, 서쪽) ~ 8(7)
//  파란 통로: 2열 A~H, 6열 A~H, 4열 B~H, B행 2~6열, G행 2~6열.  ① A2 → H6 하역, ② A6 → H2 하역, H4 출발·복귀
#define ROW_B 1                                     // 북쪽 연결 행 (B행)
#define ROW_G 6                                     // 남쪽 연결 행 (G행)
#define ROW_D 3                                     // 장애물 후보 행 (D행)
const uint8_t LANE_COL[3]  = { 1, 3, 5 };           // 통로 열: 2열, 4열, 6열 (0부터 센 번호)
const uint8_t OPEN_LANES   = 1;                     // 3곳 중 열린 통로 수
// 통로 확인 순서: D4 → D2 → D6   (LANE_COL 의 번호: 0 = 2열, 1 = 4열, 2 = 6열)
//   D4 가 열려 있으면 4열만 사용 / 막혔으면 D2 확인 → 열려 있으면 2열만 사용 / D2 도 막혔으면 D6 는 확인 없이 6열 사용
const uint8_t PROBE_ORDER[3] = { 1, 0, 2 };

// ----------------------------------------------------------------- 임무
struct Job { uint8_t from, to; };
const Job JOBS[2] = {
    { P('A', 2), P('H', 6) },                       // ① 적재함 A2 → 도착지 H6
    { P('A', 6), P('H', 2) },                       // ② 적재함 A6 → 도착지 H2
};
const uint8_t PARK_NODE  = P('H', 4);               // 복귀, 2초 정지

// 출발: START_NODE 바깥(남쪽 50mm)에서 START_HDG 를 보고 대기   ※ 현장 확인
const uint8_t START_NODE = P('H', 4);
const uint8_t START_HDG  = HN;

#define AUTO_ORDER 1                                // 1 = 열린 통로를 안 뒤 더 빠른 배송 순서 자동 선택, 0 = ① → ②

// ----------------------------------------------------------------- 상태
Nav nav;
LaneState lane[3] = { L_UNKNOWN, L_UNKNOWN, L_UNKNOWN };
int8_t openLane = -1;                               // 열린 통로 번호 (0~2), 모르면 -1

static uint8_t rowNode(uint8_t row, uint8_t col) { return (uint8_t)(row * 8 + col); }

static int8_t laneOfCol(uint8_t col)
{
    for (uint8_t i = 0; i < 3; i++) if (LANE_COL[i] == col) return i;
    return -1;
}

static void logLanes()
{
    uint8_t packed = 0xC0;                          // 4번째 칸은 없음(3)
    Serial.print(F("[lane]"));
    for (uint8_t i = 0; i < 3; i++) {
        packed |= (uint8_t)lane[i] << (2 * i);
        Serial.print(' '); printNode(rowNode(ROW_D, LANE_COL[i])); Serial.print('='); Serial.print("?OX"[lane[i]]);
    }
    Serial.println();
    runlogAdd(RL_LANE, packed);
}

// 통로 상태 기록 + 추론: 열린 곳을 찾았으면 나머지는 막힘, 막힌 곳을 다 찾았으면 남은 곳은 열림
static void setLane(uint8_t i, LaneState st)
{
    if (lane[i] == st) return;
    lane[i] = st;
    uint8_t nOpen = 0, nBlk = 0;
    for (uint8_t k = 0; k < 3; k++) { nOpen += lane[k] == L_OPEN; nBlk += lane[k] == L_BLOCKED; }
    LaneState fill = L_UNKNOWN;
    if (nOpen >= OPEN_LANES) fill = L_BLOCKED;
    else if (nBlk >= 3 - OPEN_LANES) fill = L_OPEN;
    if (fill != L_UNKNOWN) for (uint8_t k = 0; k < 3; k++) if (lane[k] == L_UNKNOWN) lane[k] = fill;
    for (uint8_t k = 0; k < 3; k++) if (lane[k] == L_OPEN) openLane = k;
    logLanes();
}

// ----------------------------------------------------------------- Nav 콜백
// 전방 IR 장애물 감시: 아직 모르는 통로를 G행 → C행 사이로 지나갈 때만 (그 밖에서는 적재함 등을 장애물로 착각하지 않게)
static bool hopCheck(uint8_t from, uint8_t to)
{
    if (colOf(from) != colOf(to)) return false;
    int8_t i = laneOfCol(colOf(to));
    if (i < 0 || lane[i] != L_UNKNOWN) return false;
    uint8_t r1 = rowOf(from), r2 = rowOf(to);
    return r1 >= 2 && r1 <= ROW_G && r2 >= 2 && r2 <= ROW_G;       // C(2) ~ G(6)
}

// D 교차점을 지나갔다 = 그 통로는 열림
static void onNode(uint8_t n)
{
    if (rowOf(n) != ROW_D) return;
    int8_t i = laneOfCol(colOf(n));
    if (i >= 0) setLane(i, L_OPEN);
}

// ----------------------------------------------------------------- 경로 (교차점 목록 → 직선 구간마다 이동)
static uint8_t g_pts[12];
static uint8_t g_np = 0;

static void addPt(uint8_t n)
{
    if (g_np == 0 || g_pts[g_np - 1] != n) g_pts[g_np++] = n;
}

static bool isNorth(uint8_t n) { return rowOf(n) <= ROW_B; }

// from → to 경로: 같은 열의 연결 행(B/G) → 연결 행을 따라 → (남↔북이면) 열린 통로 → 연결 행 → to 의 열 → to
static void buildPath(uint8_t from, uint8_t to)
{
    g_np = 0;
    addPt(from);
    uint8_t fr = isNorth(from) ? ROW_B : ROW_G;
    uint8_t tr = isNorth(to) ? ROW_B : ROW_G;
    addPt(rowNode(fr, colOf(from)));
    if (fr != tr) {
        if (openLane < 0) uiFatal(F("open lane unknown"), 23);
        addPt(rowNode(fr, LANE_COL[openLane]));
        addPt(rowNode(tr, LANE_COL[openLane]));
    }
    addPt(rowNode(tr, colOf(to)));
    addPt(to);
}

// g_pts 를 따라 이동. 방향이 바뀌는 곳과 마지막에서만 정지 (그 사이 교차점은 멈추지 않고 통과)
static NavResult runPath()
{
    Serial.print(F("[path]"));
    for (uint8_t i = 0; i < g_np; i++) { Serial.print(' '); printNode(g_pts[i]); }
    Serial.println();
    runlogAdd(RL_PLAN, g_pts[g_np - 1], g_np);
    uint8_t prev = g_pts[0];
    for (uint8_t i = 1; i < g_np; i++) {
        uint8_t next = (i + 1 < g_np) ? g_pts[i + 1] : NODE_NONE;
        if (next != NODE_NONE && !Nav::mustStopAt(prev, g_pts[i], next)) continue;
        NavResult r = nav.leg(g_pts[i], true);
        if (r != NAV_OK) return r;
        prev = g_pts[i];
    }
    return NAV_OK;
}

static void goTo(uint8_t to)
{
    buildPath(nav.node, to);
    if (runPath() != NAV_OK) uiFatal(F("obstacle on known lane"), 24);
}

// ----------------------------------------------------------------- ① 통로 찾기
//  PROBE_ORDER 순서대로 (D4 → D2 → D6). 2곳이 막힌 걸 알면 남은 곳은 setLane 이 열림으로 정해서 반복이 끝난다
static void findLane()
{
    while (openLane < 0) {
        int8_t pick = -1;
        for (uint8_t k = 0; k < 3 && pick < 0; k++)
            if (lane[PROBE_ORDER[k]] == L_UNKNOWN) pick = PROBE_ORDER[k];
        if (pick < 0) uiFatal(F("no lane left"), 12);

        uint8_t g = rowNode(ROW_G, LANE_COL[pick]);
        uint8_t b = rowNode(ROW_B, LANE_COL[pick]);
        Serial.print(F("[probe] ")); printNode(rowNode(ROW_D, LANE_COL[pick])); Serial.println();

        // 지금 위치 → G행 → 그 통로의 G 교차점 → 북쪽 B 교차점 (장애물 감시)
        g_np = 0;
        addPt(nav.node);
        addPt(rowNode(ROW_G, colOf(nav.node)));
        addPt(g);
        addPt(b);
        if (runPath() == NAV_OK) {
            setLane(pick, L_OPEN);                   // B행 도착 = 열린 통로
        } else {
            setLane(pick, L_BLOCKED);
#if OBST_BACK
            nav.backTo(g);                           // 장애물 → G행까지 앞으로 간 칸 수만큼 교차점을 세며 후진 → G 교차점에 정렬
#else
            nav.uturnOnEdge();                       // 장애물 → 그 자리에서 U턴 → (다음 경로에서) 앞으로 간 칸 수만큼 되돌아 G행으로
#endif
        }
    }
}

// ----------------------------------------------------------------- ② 적재함 들기·내리기
static void shortMove(int ms)                        // 정렬 후 앞(+)/뒤(-)로 조금: 리프트를 적재함 중심에 맞춤
{
    if (ms > 0)      { Forward(SPEED_APPROACH);  waitMs(ms);  Brake(); }
    else if (ms < 0) { Backward(SPEED_APPROACH); waitMs(-ms); Brake(); }
}

static void pickAt(uint8_t at)
{
    goTo(at);                                        // 적재함 밑 교차점에 정렬 (리프트는 내린 상태로 밑으로 들어감)
    runlogAdd(RL_PICK, at);
    Serial.println(F("[pallet] PICK"));
    shortMove(PICK_ADJ_MS);
    LiftUp();
    nav.cruise = min(SPEED_CRUISE, SPEED_CARRY);     // 들고 있는 동안 천천히 (낙하 방지)
#if PICK_BACK
    nav.backTo(rowNode(ROW_B, colOf(at)));           // 든 채로 B행까지 후진
#endif                                               // (0 이면 다음 이동에서 교차점 위 U턴)
}

static void dropAt(uint8_t at)
{
    goTo(at);                                        // 도착 교차점에 정렬 = 적재함 중심이 도착 구역 안
    runlogAdd(RL_DROP, at);
    Serial.println(F("[pallet] DROP"));
    shortMove(DROP_ADJ_MS);
    LiftDown();
    nav.cruise = SPEED_CRUISE;
#if DROP_BACK
    nav.backTo(rowNode(ROW_G, colOf(at)));           // G행까지 후진 → 적재함 밑에서 완전히 빠져나옴
#else
    Backward(EXIT_BACK_PWM);                         // 적재함 밑에서 빠져나올 만큼 후진
    waitMs(EXIT_BACK_MS);
    Brake();
    uint8_t back = reverseH(nav.hdg);
    uint8_t behind = stepNode(nav.node, back);
    runlogAdd(RL_TURN, 'E');
    UTurnOnEdge();                                   // 칸 중간 U턴 (선을 찾아 멈춤) → G행 교차점으로 접근
    nav.setApproach(behind, back);
#endif
}

// ----------------------------------------------------------------- 배송 순서 (예상 시간 비교)
static uint16_t orderCost(uint8_t first)
{
    const Job& j1 = JOBS[first];
    const Job& j2 = JOBS[1 - first];
    const uint8_t goals[5] = { j1.from, j1.to, j2.from, j2.to, PARK_NODE };
    uint8_t n = nav.node, h = nav.hdg;
    uint16_t cost = 0;
    for (uint8_t k = 0; k < 5; k++) {
        buildPath(n, goals[k]);
        for (uint8_t i = 0; i + 1 < g_np; i++) {
            uint8_t d = (uint8_t)headingTo(g_pts[i], g_pts[i + 1]);
            uint8_t dt = (d - h) & 3;
            cost += straightDist(g_pts[i], g_pts[i + 1]) * PLAN_COST_HOP
                  + (dt == 2 ? PLAN_COST_UTURN : (dt ? PLAN_COST_TURN : 0));
            h = d;
        }
        n = goals[k];
        if ((k == 0 || k == 2) && PICK_BACK) { n = rowNode(ROW_B, colOf(n)); cost += PLAN_COST_HOP; }   // 든 뒤 B행까지 후진
        if (k == 1 || k == 3) {                                                                  // 내린 뒤 G행 쪽으로
            n = rowNode(ROW_G, colOf(n));
            cost += PLAN_COST_HOP;
            if (!DROP_BACK) h = reverseH(h);
        }
    }
    return cost;
}

// ----------------------------------------------------------------- main
void setup()
{
    Serial.begin(115200);
    driveInit();
    uiInit();
    showConfig();
    runlogDump();                    // 지난 주행 기록 출력
    liftInit();

    if (!irCalibrated())    uiFatal(F("config.h IR 현장값 미입력"));
    if (!frontCalibrated()) uiFatal(F("config.h FRONT_OBSTACLE_TH 미입력"));

    nav.hopCheck = hopCheck;
    nav.onNode = onNode;

    Serial.println(F("COURSE2 drive ready - waiting start"));
    beepReady();
    uiWaitStart();                   // 출발 신호: 버튼 또는 RFID 카드 태그 (config.h START_MODE)
    uiArmAbort();
    unsigned long t0 = millis();
    runlogBegin();

    if (START_XING_PASSED) nav.setAt(START_NODE, START_HDG);
    else                   nav.setApproach(START_NODE, START_HDG);

    findLane();                      // ① 빈 차로 열린 통로 찾기

    uint8_t first = 0;
#if AUTO_ORDER
    if (orderCost(1) < orderCost(0)) first = 1;
#endif
    Serial.print(F("[order] first=")); Serial.println(first + 1);

    for (uint8_t k = 0; k < 2; k++) {               // ② 배송 2건
        const Job& j = JOBS[k == 0 ? first : 1 - first];
        pickAt(j.from);
        dropAt(j.to);
    }

    goTo(PARK_NODE);                                // ③ 복귀 + 2초 정지
    nav.hold(STOP_HOLD_MS);

    Stop();
    runlogAdd(RL_DONE);
    runlogFlush();
    Serial.print(F("DONE ms="));
    Serial.println(millis() - t0);
    beepDone();
}

void loop() {}
