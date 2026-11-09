// =====================================================================
//  종목② 팔레트 운반 + 장애물 (제한 180초)
//   배송 ① A2 → H6, ② A6 → H2 (한 번에 1개), 이후 H4 복귀 2초 정지.
//   파란 통로만 사용. D2·D4·D6 중 2곳 차단(추첨) → 전방 IR로 감지 후 경로 재계산.
//
//  13:20 제출 후 이 파일은 고정. 보정은 libraries/AGV2/src/config.h 만 수정.
//  경로는 '맵(BLUE) + 임무(MISSION)'로부터 매번 최단경로(Dijkstra)를 계산하므로
//  막힌 통로 위치가 달라도 코드 수정 없이 동작한다.
// =====================================================================
#include <AGV2.h>

// ----------------------------------------------------------------- 맵
// 파란 통로(직선 구간 목록).  ※ 현장 맵 사진으로 확인 필요 (README '미확인 사항')
struct Seg { uint8_t a, b; };
const Seg BLUE[] = {
    { P('A', 2), P('H', 2) },   // 2열 전체 (A2 픽업 ~ H2 배송)
    { P('A', 6), P('H', 6) },   // 6열 전체 (A6 픽업 ~ H6 배송)
    { P('B', 4), P('H', 4) },   // 4열 (B행 ~ H4 복귀점)
    { P('B', 2), P('B', 6) },   // B행 연결
    { P('G', 2), P('G', 6) },   // G행 연결
};

// 장애물 후보(추첨으로 일부 차단) — 이 교차점에 닿는 칸이 막힘
const uint8_t CAND[] = { P('D', 2), P('D', 4), P('D', 6) };
const uint8_t N_CAND = sizeof(CAND);
const uint8_t OPEN_LANES = 1;          // 열린 통로 수 (3곳 중 2곳 차단 → 1)

// ----------------------------------------------------------------- 임무
struct Job { uint8_t from, to; };
const Job JOBS[2] = {
    { P('A', 2), P('H', 6) },   // ①
    { P('A', 6), P('H', 2) },   // ②
};
const uint8_t PARK_NODE = P('H', 4);   // 두 배송 후 복귀, 2초 정지
// 1 = 열린 통로를 알게 된 시점에 더 빠른 배송 순서를 자동 선택, 0 = 항상 ①→②
#define AUTO_ORDER 1

// 출발: START_NODE 바깥 50mm에서 START_HDG 방향을 보고 대기   ※ 현장 확인 필요
const uint8_t START_NODE = P('H', 4);
const uint8_t START_HDG  = HN;

// 경로 비용 (시간 비율): 한 칸 / 90°회전 / U턴 / 미확인 통로 벌점
const uint8_t COST_HOP = 10, COST_TURN = 6, COST_UTURN = 10, COST_UNKNOWN = 15;

// ----------------------------------------------------------------- 상태
enum LaneState : uint8_t { L_UNKNOWN, L_OPEN, L_BLOCKED };
uint8_t adj[64];             // 비트 h = 그 방향으로 파란 통로 있음
LaneState lane[N_CAND];
bool carrying = false;
Nav nav;

int8_t candIndex(uint8_t n)
{
    for (uint8_t i = 0; i < N_CAND; i++) if (CAND[i] == n) return i;
    return -1;
}

int8_t candInCol(uint8_t col)
{
    for (uint8_t i = 0; i < N_CAND; i++) if (colOf(CAND[i]) == col) return i;
    return -1;
}

void deduceLanes()
{
    uint8_t nOpen = 0, nBlk = 0;
    for (uint8_t i = 0; i < N_CAND; i++) {
        if (lane[i] == L_OPEN) nOpen++;
        if (lane[i] == L_BLOCKED) nBlk++;
    }
    LaneState fill = L_UNKNOWN;
    if (nOpen >= OPEN_LANES) fill = L_BLOCKED;                 // 열린 곳을 다 찾음
    else if (nBlk >= N_CAND - OPEN_LANES) fill = L_OPEN;       // 막힌 곳을 다 찾음
    if (fill != L_UNKNOWN)
        for (uint8_t i = 0; i < N_CAND; i++) if (lane[i] == L_UNKNOWN) lane[i] = fill;
}

void setLane(int8_t i, LaneState s)
{
    if (i < 0 || lane[i] == s) return;
    lane[i] = s;
    deduceLanes();
    Serial.print(F("[lane]"));
    for (uint8_t k = 0; k < N_CAND; k++) {
        Serial.print(' '); printNode(CAND[k]); Serial.print('=');
        Serial.print("?OX"[lane[k]]);
    }
    Serial.println();
}

bool openLaneKnown()
{
    for (uint8_t i = 0; i < N_CAND; i++) if (lane[i] == L_OPEN) return true;
    return false;
}

void buildMap()
{
    memset(adj, 0, sizeof(adj));
    for (uint8_t i = 0; i < sizeof(BLUE) / sizeof(BLUE[0]); i++) {
        uint8_t n = BLUE[i].a;
        int8_t h = headingTo(BLUE[i].a, BLUE[i].b);
        if (h < 0) uiFatal(F("BLUE seg not straight"));
        while (n != BLUE[i].b) {
            uint8_t m = stepNode(n, h);
            adj[n] |= 1 << h;
            adj[m] |= 1 << reverseH(h);
            n = m;
        }
    }
    for (uint8_t i = 0; i < N_CAND; i++) lane[i] = L_UNKNOWN;
}

// 0 = 사용 불가, 1 = 사용 가능, 2 = 미확인 통로(벌점)
uint8_t edgeUse(uint8_t a, uint8_t h, bool allowUnknown)
{
    if (!(adj[a] & (1 << h))) return 0;
    uint8_t b = stepNode(a, h);
    int8_t ia = candIndex(a), ib = candIndex(b);
    LaneState s = L_OPEN;
    if (ia >= 0) s = lane[ia];
    if (ib >= 0 && lane[ib] != L_OPEN) s = lane[ib];
    if (s == L_BLOCKED) return 0;
    if (s == L_UNKNOWN) return allowUnknown ? 2 : 0;
    return 1;
}

// ----------------------------------------------------------------- 경로 계획 (Dijkstra, 상태 = 교차점×방향)
uint16_t dist[256];
uint8_t  prevSt[256];
uint8_t  doneBits[32];

uint8_t path[64];
uint8_t pathLen = 0;
uint16_t planCost = 0;

bool plan(uint8_t start, uint8_t h0, uint8_t goal, bool allowUnknown)
{
    for (int i = 0; i < 256; i++) { dist[i] = 0xFFFF; prevSt[i] = 0; }
    memset(doneBits, 0, sizeof(doneBits));
    uint8_t s0 = start * 4 + h0;
    dist[s0] = 0;
    prevSt[s0] = s0;
    int best = -1;

    for (;;) {
        int u = -1;
        uint16_t du = 0xFFFF;
        for (int i = 0; i < 256; i++)
            if (!(doneBits[i >> 3] & (1 << (i & 7))) && dist[i] < du) { du = dist[i]; u = i; }
        if (u < 0) break;
        doneBits[u >> 3] |= 1 << (u & 7);
        uint8_t n = u >> 2, h = u & 3;
        if (n == goal) { best = u; break; }
        for (uint8_t d = 0; d < 4; d++) {
            uint8_t use = edgeUse(n, d, allowUnknown);
            if (!use) continue;
            uint8_t dt = (d - h) & 3;
            uint16_t c = du + COST_HOP + (dt == 2 ? COST_UTURN : (dt ? COST_TURN : 0))
                         + (use == 2 ? COST_UNKNOWN : 0);
            uint8_t v = stepNode(n, d) * 4 + d;
            if (c < dist[v]) { dist[v] = c; prevSt[v] = u; }
        }
    }
    if (best < 0) return false;
    planCost = dist[best];

    uint8_t rev[64];
    uint8_t k = 0;
    uint8_t st = best;
    for (;;) {
        rev[k++] = st >> 2;
        if (st == s0) break;
        st = prevSt[st];
    }
    pathLen = 0;
    while (k) path[pathLen++] = rev[--k];
    return true;
}

// ----------------------------------------------------------------- Nav 콜백
// 미확인 후보 통로(세로, B~F행 사이) 칸에서만 전방 IR 감시
bool hopCheck(uint8_t from, uint8_t to)
{
    if (carrying && !FRONT_CHECK_WHILE_CARRY) return false;
    if (colOf(from) != colOf(to)) return false;
    int8_t i = candInCol(colOf(to));
    if (i < 0 || lane[i] != L_UNKNOWN) return false;
    uint8_t r1 = rowOf(from), r2 = rowOf(to);
    return r1 >= 1 && r1 <= 5 && r2 >= 1 && r2 <= 5;        // B(1) ~ F(5)
}

void onNode(uint8_t n)
{
    setLane(candIndex(n), L_OPEN);     // 후보 교차점을 지나갔다 = 열린 통로
}

// ----------------------------------------------------------------- 이동
// goal까지 이동하고 정렬·정지. 장애물이면 U턴 후 재계획. 경로가 없으면 false.
bool gotoNode(uint8_t goal)
{
    for (uint8_t attempt = 0; attempt < 8; attempt++) {
        bool allowUnknown = !carrying || FRONT_CHECK_WHILE_CARRY;
        if (!plan(nav.node, nav.hdg, goal, allowUnknown)) return false;

        Serial.print(F("[plan]"));
        for (uint8_t i = 0; i < pathLen; i++) { Serial.print(' '); printNode(path[i]); }
        Serial.println();

        if (pathLen == 1) {                        // 이미 goal (접근 중이면 도착)
            nav.leg(goal, true);
            return true;
        }
        // 경로를 직선 구간으로 묶어 실행
        NavResult r = NAV_OK;
        uint8_t prev = path[0];
        for (uint8_t i = 1; i < pathLen && r == NAV_OK; i++) {
            uint8_t next = (i + 1 < pathLen) ? path[i + 1] : NODE_NONE;
            if (next != NODE_NONE && !Nav::mustStopAt(prev, path[i], next)) continue;
            // path[i]가 구간 끝: 다음 구간이 같은 방향이면 통과, 아니면 정지
            r = nav.leg(path[i], true);
            prev = path[i];
        }
        if (r == NAV_OK) return true;

        // 장애물: 그 열의 후보 통로를 막힘 처리 → 칸 중간에서 U턴 → 재계획
        setLane(candInCol(colOf(nav.obsTo)), L_BLOCKED);
        nav.uturnOnEdge();
    }
    return false;
}

// 팔레트를 든 채로는 장애물을 못 보는 경우: 빈 차로 열린 통로를 먼저 확인
void ensureOpenLane()
{
    while (!openLaneKnown()) {
        int8_t pick = -1;
        uint16_t bestCost = 0xFFFF;
        for (uint8_t i = 0; i < N_CAND; i++) {
            if (lane[i] != L_UNKNOWN) continue;
            if (plan(nav.node, nav.hdg, CAND[i], true) && planCost < bestCost) {
                bestCost = planCost; pick = i;
            }
        }
        if (pick < 0) uiFatal(F("no lane to probe"));
        Serial.print(F("[probe] ")); printNode(CAND[pick]); Serial.println();
        gotoNode(CAND[pick]);       // 막혀 있으면 false로 돌아오고 lane 상태가 갱신됨
    }
}

void shortMove(int ms)
{
    if (ms > 0) { Forward(SPEED_APPROACH); waitMs(ms); Brake(); }
    else if (ms < 0) { Backward(SPEED_APPROACH); waitMs(-ms); Brake(); }
}

void doPick()
{
    shortMove(PICK_ADJ_MS);
    LiftUp();
    carrying = true;
    nav.cruise = min(SPEED_CRUISE, SPEED_CARRY);
}

void doDrop()
{
    shortMove(DROP_ADJ_MS);
    LiftDown();
    carrying = false;
    nav.cruise = SPEED_CRUISE;
    // 팔레트 밑에서 후진으로 빠져나온 뒤 U턴 → 왔던 교차점으로 접근 상태
    Backward(EXIT_BACK_PWM);
    waitMs(EXIT_BACK_MS);
    Brake();
    uint8_t back = reverseH(nav.hdg);
    uint8_t behind = stepNode(nav.node, back);
    UTurn();
    nav.setApproach(behind, back);
}

// 예상 비용: 현재 위치 → 배송 a → 배송 b → 복귀  (경로 비용 합)
uint16_t orderCost(uint8_t first)
{
    const Job& j1 = JOBS[first];
    const Job& j2 = JOBS[1 - first];
    const uint8_t goals[5] = { j1.from, j1.to, j2.from, j2.to, PARK_NODE };
    uint8_t n = nav.node, h = nav.hdg;
    uint16_t total = 0;
    for (uint8_t k = 0; k < 5; k++) {
        bool carry = (k == 1 || k == 3);               // 팔레트를 들고 가는 구간
        if (!plan(n, h, goals[k], !carry || FRONT_CHECK_WHILE_CARRY)) return 0xFFFF;
        total += planCost;
        if (pathLen >= 2) h = headingTo(path[pathLen - 2], path[pathLen - 1]);
        n = goals[k];
        if (carry) {                                   // 하역 후 후진·U턴 → 뒤 교차점으로 접근
            h = reverseH(h);
            n = stepNode(n, h);
            total += COST_UTURN + COST_HOP;
        }
    }
    return total;
}

void deliver(const Job& j)
{
    if (!FRONT_CHECK_WHILE_CARRY) ensureOpenLane();
    if (!gotoNode(j.from)) uiFatal(F("no path to pallet"));
    doPick();
    if (!gotoNode(j.to)) uiFatal(F("no path to drop"));
    doDrop();
}

// ----------------------------------------------------------------- main
void setup()
{
    Serial.begin(115200);
    driveInit();
    uiInit();
    liftInit();

    if (!irCalibrated())    uiFatal(F("config.h IR 현장값 미입력"));
    if (!frontCalibrated()) uiFatal(F("config.h FRONT_OBSTACLE_TH 미입력"));

    buildMap();
    nav.hopCheck = hopCheck;
    nav.onNode = onNode;

    Serial.println(F("COURSE2 ready - press button"));
    beepReady();
    uiWaitStart();
    uiArmAbort();
    unsigned long t0 = millis();

    nav.setApproach(START_NODE, START_HDG);
    if (!FRONT_CHECK_WHILE_CARRY) ensureOpenLane();

    uint8_t first = 0;
#if AUTO_ORDER
    if (orderCost(1) < orderCost(0)) first = 1;
#endif
    Serial.print(F("[order] first=")); Serial.println(first + 1);
    deliver(JOBS[first]);
    deliver(JOBS[1 - first]);

    if (!gotoNode(PARK_NODE)) uiFatal(F("no path to park"));
    nav.hold(STOP_HOLD_MS);

    Stop();
    Serial.print(F("DONE ms="));
    Serial.println(millis() - t0);
    beepDone();
}

void loop() {}
