// =====================================================================
//  종목② 장애물 상황의 팔레트 이동 — 11-13 대회 당일 공개 맵
//
//        1   2   3   4   5   6   7   8
//   A                [②]     [①]              적재 ① A6, 적재 ② A4
//   B   ─────────── 북쪽 연결 행 ───────────
//   D   ?   ?   ?   ■■■■■■■■■   ?   ?         D4~D6 고정 가림막, D1~D3 중 1개·D7~D8 중 1개 장애물
//   F   ─── 출발 직후 서쪽으로 가는 행 ───
//   H      <①>         ▲   <②>                하역 ① H2, 출발 H5(북향), 하역 ② H6
//
//  순서(고정): 출발 H5 → ① A6 들기 → H2 내리기 → ② A4 들기 → H6 내리기 = 종료
//  파란 통로 제한 없음 (모든 선 사용 가능). 주행 방식은 종목①과 같다 (선 따라가기·정렬·제자리 회전).
//
//  D행을 건너는 통로: 서쪽 3열 → 2열 → 1열 순서로 확인해서 처음 열린 곳 하나만 쓴다 (동쪽 7·8열은 쓰지 않음)
//   - 확인은 첫 이동(빈 차) 때만: F 교차점 → E 교차점에 '정렬해서 정지' → 그 자리에서 전방 거리센서로 판단
//     (교차점에 선 뒤에 재므로 로봇이 자기 위치를 정확히 안다. E 교차점에서 D 의 장애물까지는 약 3cm)
//   - 장애물이면 조금 물러나 U턴 → F 교차점 → 서쪽 옆 통로의 E 교차점에서 다시 측정 (후진으로 돌아가지 않음)
//   - 추론하지 않는다: 3열이 막혀도 2열을 '열림'으로 단정하지 않고 2열도 서서 잰다
//
//  적재 위치 = A행 바깥 T자 (하역과 같은 방식): A 교차점에 정렬 → T자까지 → 올림 → A 교차점까지 짧게 후진 → 거기서 U턴
//  하역 위치 = H행 바깥 T자: 열의 선이 H행 밖으로 약 50mm 나와 끝에 짧은 가로선(T자)이 있다.
//   남쪽으로 내려와 H 교차점에 정렬 → 선 따라 천천히 더 가서 센서가 T자를 보는 순간 정지 → 내림
//   ① 은 내린 뒤 0.6초만 후진 → G~H 사이에서 바로 U턴 → 선 따라 북쪽으로 (DROP_BACK 2). ② 는 내리면 종료.
//   ※ 바닥 센서가 바퀴보다 앞에 있어 후진 중에는 선을 따라 조향할 수 없다 → 후진 좌우 균형(MOTOR_L/R_GAIN_REV)으로 똑바로 간다
//
//  경로 (3열이 열린 경우)
//   가는 길 ①  H5 → F5 → F3 → E3(정지·측정) → (D3) → B3 → B6 → A6 → T자
//   운반 ①     A6 → B6 → B3 → (D3) → F3 → F2 → H2 → T자        → 0.6초 후진 → U턴
//   가는 길 ②  G2 → G3 → (D3) → B3 → B4 → A4
//   운반 ②     A4 → B4 → B3 → (D3) → F3 → F6 → H6 → T자 = 종료
//
//  주행 기록: 주행 후 USB 를 꽂고 시리얼 모니터(115200)를 열면 '지난 주행 기록'이 나온다 → 복사해서 전달
// =====================================================================
#include <AGV2.h>

// ----------------------------------------------------------------- 맵
//  행: A(0, 북쪽) ~ H(7, 남쪽)   열: 1(0, 서쪽) ~ 8(7)
#define ROW_B 1                                     // 가림막 북쪽에서 동서로 이동하는 행
#define ROW_D 3                                     // 장애물·가림막 행
#define ROW_E 4                                     // 장애물 확인 행: E 교차점에 서서 전방을 잰다 (장애물까지 약 3cm)
#define ROW_F 5                                     // 출발 직후 서쪽으로 이동하는 행 / 장애물을 보면 물러나는 행
#define ROW_G 6
#define ROW_H 7
const uint8_t N_LANE = 3;
const uint8_t LANE_COL[N_LANE] = { 2, 1, 0 };       // 통로 확인 순서: 3열(D3) → 2열(D2) → 1열(D1)  (0부터 센 열 번호)

const uint8_t PICK1 = P('A', 6), DROP1 = P('H', 2); // ① 적재 → 하역
const uint8_t PICK2 = P('A', 4), DROP2 = P('H', 6); // ② 적재 → 하역 (내리면 종료)

// 출발: START_NODE 에서 START_HDG 를 보고 대기.  config.h START_XING_PASSED:
//   0 = 바닥 센서가 H5 가로선 앞(남쪽)에 있음 → 출발하면 H5 를 처음 셈   1 = 센서가 이미 H5 선을 지나 있음
const uint8_t START_NODE = P('H', 5);
const uint8_t START_HDG  = HN;

// ----------------------------------------------------------------- 상태
Nav nav;
LaneState lane[N_LANE] = { L_UNKNOWN, L_UNKNOWN, L_UNKNOWN };
int8_t openLane = -1;                               // 쓰는 통로 번호 (LANE_COL 의 순서), 모르면 -1

static uint8_t rowNode(uint8_t row, uint8_t col) { return (uint8_t)(row * 8 + col); }

static int8_t laneOfCol(uint8_t col)
{
    for (uint8_t i = 0; i < N_LANE; i++) if (LANE_COL[i] == col) return i;
    return -1;
}

static void setLane(uint8_t i, LaneState st)
{
    if (lane[i] == st) return;
    lane[i] = st;
    if (st == L_OPEN) openLane = i;
    uint8_t packed = 0xC0;                          // 주행 기록: 통로마다 2비트 (D3 D2 D1 순), 4번째 칸 없음
    Serial.print(F("[lane]"));
    for (uint8_t k = 0; k < N_LANE; k++) {
        packed |= (uint8_t)lane[k] << (2 * k);
        Serial.print(' '); printNode(rowNode(ROW_D, LANE_COL[k])); Serial.print('='); Serial.print("?OX"[lane[k]]);
    }
    Serial.println();
    runlogAdd(RL_LANE, packed);
}

// ----------------------------------------------------------------- Nav 콜백
// 달리는 중 전방 감시(안전망, config.h PROBE_SAFETY_NET 1 일 때만 동작): 아직 '모름'인 통로의 E → D 칸.
//   기본(0)은 E 교차점에서 서서 잰 결과로 바로 '열림'을 확정하므로 이 감시는 걸리지 않고 D 구역을 통과 속도로 지나간다
static bool hopCheck(uint8_t from, uint8_t to)
{
    if (colOf(from) != colOf(to)) return false;
    int8_t i = laneOfCol(colOf(to));
    if (i < 0 || lane[i] != L_UNKNOWN) return false;
    uint8_t r1 = rowOf(from), r2 = rowOf(to);
    return r1 >= ROW_D && r1 <= ROW_E && r2 >= ROW_D && r2 <= ROW_E;
}

// 교차점에 정지한 상태에서 전방 거리센서를 여러 번 읽어 평균 → th 보다 가까우면(값이 크면) true
static bool frontBlocked(int th)
{
    long sum = 0;
    for (uint8_t i = 0; i < 16; i++) { sum += irFront(); delay(4); }
    int v = (int)(sum / 16);
    bool blocked = FRONT_NEAR_IS_HIGH ? (v > th) : (v < th);
    Serial.print(F("[front] ")); printNode(nav.node); Serial.print(F(" F=")); Serial.print(v);
    Serial.println(blocked ? F(" → 장애물") : F(" → 없음"));
    runlogAdd(RL_FRONT, nav.node, (uint8_t)(v / 4 > 255 ? 255 : v / 4));
    return blocked;
}

// D 교차점을 지나갔다 = 그 통로는 열림
static void onNode(uint8_t n)
{
    if (rowOf(n) != ROW_D) return;
    int8_t i = laneOfCol(colOf(n));
    if (i >= 0) setLane(i, L_OPEN);
}

// ----------------------------------------------------------------- 경로 (교차점 목록 → 직선 구간마다 이동)
static uint8_t g_pts[8];
static uint8_t g_np = 0;

static void addPt(uint8_t n)
{
    if (g_np == 0 || g_pts[g_np - 1] != n) g_pts[g_np++] = n;
}

// 지금 위치 → (같은 열의 fromRow) → (fromRow 를 따라 통로 열) → 통로로 D행을 건너 toRow → (toRow 를 따라 to 의 열) → to
static void buildVia(uint8_t to, uint8_t fromRow, uint8_t toRow, uint8_t laneCol)
{
    g_np = 0;
    addPt(nav.node);
    addPt(rowNode(fromRow, colOf(nav.node)));
    addPt(rowNode(fromRow, laneCol));
    addPt(rowNode(toRow, laneCol));
    addPt(rowNode(toRow, colOf(to)));
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

// 확인된 통로로 이동 (운반 중·두 번째 가는 길)
static void goVia(uint8_t to, uint8_t fromRow, uint8_t toRow)
{
    if (openLane < 0) uiFatal(F("open lane unknown"), 23);
    buildVia(to, fromRow, toRow, LANE_COL[openLane]);
    if (runPath() != NAV_OK) uiFatal(F("obstacle on known lane"), 24);
}

// ----------------------------------------------------------------- 첫 이동: 통로를 확인하며 북쪽으로
//  통로마다:  F 교차점 → (북쪽으로 한 칸) E 교차점에 '정렬해서 정지' → 그 자리에서 전방을 재서 판단
//    없음 → 그대로 북쪽으로 D 를 지나 B행 → 적재 위치
//    있음 → 조금 물러나 U턴 → F 교차점으로 돌아가 서쪽 옆 통로에서 다시  (교차점을 세며 후진하지 않는다)
//  교차점에 선 뒤에 재므로 로봇은 언제나 자기 위치(E 교차점)를 정확히 안다.
static void goNorthProbing(uint8_t to)
{
    for (uint8_t k = 0; k < N_LANE; k++) {
        if (lane[k] == L_BLOCKED) continue;
        uint8_t c = LANE_COL[k];
        uint8_t fNode = rowNode(ROW_F, c), eNode = rowNode(ROW_E, c);
        Serial.print(F("[probe] ")); printNode(rowNode(ROW_D, c)); Serial.println();

        // ① F행을 따라 통로의 F 교차점까지
        g_np = 0;
        addPt(nav.node);
        addPt(rowNode(ROW_F, colOf(nav.node)));
        addPt(fNode);
        if (g_np > 1 && runPath() != NAV_OK) uiFatal(F("obstacle before probe"), 25);
#if FRONT_FAR_TH > 0
        nav.turnTo(HN);                              // (선택) F 교차점에서 북쪽을 보고 미리 보기
        if (frontBlocked(FRONT_FAR_TH)) { setLane(k, L_BLOCKED); continue; }
#endif
        // ② 북쪽으로 한 칸: E 교차점에 정렬해서 정지 (장애물이 있으면 약 3cm 앞)
        if (nav.leg(eNode, true) != NAV_OK) uiFatal(F("obstacle before probe"), 25);

        // ③ 서서 전방 측정
        bool blocked = frontBlocked(FRONT_STOP_TH);
        if (!blocked) {
            // ④ 없음 → 이 통로는 열림으로 확정 → D 구역을 감속·감시 없이 통과 속도로 지나간다
#if !PROBE_SAFETY_NET
            setLane(k, L_OPEN);
#endif
            g_np = 0;
            addPt(nav.node);
            addPt(rowNode(ROW_B, c));
            addPt(rowNode(ROW_B, colOf(to)));
            addPt(to);
            if (runPath() == NAV_OK) {
                setLane(k, L_OPEN);
                return;
            }
            setLane(k, L_BLOCKED);                   // (PROBE_SAFETY_NET 1 일 때만) 달리다 감지됨 → 그 자리에서 U턴
            nav.uturnOnEdge();
            continue;
        }

        // ⑤ 있음 → F행으로 돌아감
        setLane(k, L_BLOCKED);
#if OBST_BACK
        nav.backTo(fNode);                           // E·F 선을 세며 F 교차점까지 후진
#else
        Backward(UTURN_BACK_PWM);                    // 장애물에서 조금 더 물러난 뒤
        waitMs(OBST_UTURN_BACK_MS);
        Brake();
        nav.turnTo(reverseH(nav.hdg));               // 제자리 U턴 (센서로 선을 찾아 멈춤) → 다음 경로가 F 교차점으로 전진
#endif
    }
    uiFatal(F("no open lane"), 12);
}

// ----------------------------------------------------------------- 적재물 들기·내리기
static void shortMove(int ms)                        // 정렬 후 앞(+)/뒤(-)로 조금: 리프트를 적재물 중심에 맞춤
{
    if (ms > 0)      { Forward(SPEED_APPROACH);  waitMs(ms);  Brake(); }
    else if (ms < 0) { Backward(SPEED_APPROACH); waitMs(-ms); Brake(); }
}

// 바깥 행 교차점에 정렬된 상태 → 선 끝 T자까지 더 가서 정지. 찾았으면 true
static bool toBar()
{
    bool found = LineTraceToBar(SPEED_APPROACH, BAR_MAX_MS);
    Serial.println(found ? F("[bar] T자에서 정지") : F("[bar] T자를 못 찾음 → 그 자리에 정지"));
    return found;
}

// T자 위치(교차점보다 약 5cm 바깥)에서 빠져나오기
//   back = true : T자·교차점을 세며 한 칸 안쪽 교차점까지 후진 (H2 → G2) → 거기에 정렬
//   back = false: T자와 바깥 행 교차점만 세며 그 교차점까지 짧게(약 5cm) 후진 → 교차점 위에 정렬
//                 → 다음 이동이 그 교차점 위에서 평소처럼 U턴한다 (T자 위치에서 바로 돌면 가로선을 잘못 볼 수 있어서)
static void leaveOuter(bool back, bool atBar, bool barFound)
{
    uint8_t extra = (atBar && barFound) ? 1 : 0;                     // 후진하며 T자를 한 번 더 셈
    if (extra) ClearBarForward(300);                                 // 센서를 T자 바깥쪽으로 (T자를 '정확히 한 번' 세도록)
    if (back)       nav.backTo(stepNode(nav.node, reverseH(nav.hdg)), extra);
    else if (atBar) nav.backTo(nav.node, extra);
}

static void pickHere()                               // 적재물 밑 교차점에 정렬된 상태에서
{
    bool bar = false;
#if PICK_AT_BAR
    bar = toBar();
#endif
    runlogAdd(RL_PICK, nav.node);
    Serial.println(F("[pallet] PICK"));
    shortMove(PICK_ADJ_MS);
    LiftUp();                                        // 리프트 올림 (바닥 IR 값도 '든 상태' 값으로 바뀜)
    nav.cruise = min(SPEED_CRUISE, SPEED_CARRY);     // 들고 있는 동안 천천히 (낙하 방지)
    leaveOuter(PICK_BACK, PICK_AT_BAR, bar);
}

static void dropHere(bool last)                      // 하역 열의 H 교차점에 (남쪽을 보고) 정렬된 상태에서
{
    bool bar = false;
#if DROP_AT_BAR
    bar = toBar();                                   // T자를 센서가 보는 순간 정지 = 하역 위치
#endif
    runlogAdd(RL_DROP, nav.node);
    Serial.println(F("[pallet] DROP"));
    shortMove(DROP_ADJ_MS);
    LiftDown();
    nav.cruise = SPEED_CRUISE;
    if (last) return;                                // 마지막 하역 = 종료 (빠져나오지 않음)
#if DROP_BACK == 2
    // ① : 잠깐(DROP_BACK_MS) 후진 → G~H 사이에서 바로 U턴(센서로 선을 찾아 멈춤) → 북쪽으로 선 따라 전진, 다음 교차점이 G
    (void)bar;
    Backward(REV_PWM);
    waitMs(DROP_BACK_MS);
    Brake();
    delay(TURN_SETTLE_MS);
    uint8_t back = reverseH(nav.hdg);
    uint8_t inner = stepNode(nav.node, back);        // H2 → G2
    runlogAdd(RL_TURN, 'E');
    UTurnOnEdge();
    nav.setApproach(inner, back);
#else
    leaveOuter(DROP_BACK == 1, DROP_AT_BAR, bar);    // 1: G 교차점까지 후진해 정렬, 0: H 교차점까지만
#endif
}

// ----------------------------------------------------------------- main
void setup()
{
    Serial.begin(115200);
    driveInit();
    uiInit();
    showConfig();
    runlogDump();                    // 지난 주행 기록 출력

    if (!irCalibrated())    uiFatal(F("config.h IR 현장값 미입력"));
    if (!frontCalibrated()) uiFatal(F("config.h FRONT_OBSTACLE_TH 미입력"));

    nav.hopCheck = hopCheck;
    nav.onNode = onNode;

    liftInit();                      // 리프터 내리기 — 교본 26쪽: 초기화가 끝난 뒤 실행 (전원 켤 때 올라가는 현상 처리)
    Serial.println(F("COURSE2 drive ready - waiting start"));
    beepReady();
    uiWaitStart();                   // 출발 신호: 버튼 또는 RFID 카드 태그 (config.h START_MODE)
    uiArmAbort();
    unsigned long t0 = millis();
    runlogBegin();

    if (START_XING_PASSED) {
        nav.setAt(START_NODE, START_HDG);
    } else {
        // 출발 위치(T자 근처)에서 H5 를 지난다. 센서가 T자 뒤에 있었든 T자와 H5 사이에 있었든 스스로 구분해서
        // 'H5 를 막 지난 상태'로 맞춘다 → 놓는 위치가 조금 달라도 경로가 한 칸씩 밀리지 않는다
        uint8_t lines = LineTraceStartLines(SPEED_APPROACH, START_BAR_GAP_MS);
        Serial.print(F("[start] 가로선 ")); Serial.print(lines);
        Serial.println(lines == 2 ? F("개 (T자 뒤에서 출발)") : F("개 (T자와 H5 사이에서 출발)"));
        nav.node = START_NODE;
        nav.hdg = START_HDG;
        nav.approach = false;
        nav.moving = true;
        runlogAdd(RL_NODE, (uint8_t)(START_NODE | (START_HDG << 6)), lines);
    }

    goNorthProbing(PICK1);           // ① 적재물까지 (통로 확인)
    pickHere();
    goVia(DROP1, ROW_B, ROW_F);      // ① 운반 → F행에서 2열로 → 남쪽으로 H2 (T자까지는 dropHere 가)
    dropHere(false);                 //    내리고 0.6초 후진 → U턴 (다음 교차점 = G2)

    goVia(PICK2, rowOf(nav.node), ROW_B);   // ② 적재물까지 (지금 있는 행에서 통로 열로)
    pickHere();
    goVia(DROP2, ROW_B, ROW_F);      // ② 운반 → F행에서 6열로 → 남쪽으로 H6 (F행이라 H2 쪽 ① 과 멀다)
    dropHere(true);                  // T자에서 내리면 종료

    Stop();
    runlogAdd(RL_DONE);
    runlogFlush();
    Serial.print(F("DONE ms="));
    Serial.println(millis() - t0);
    beepDone();
}

void loop() {}
