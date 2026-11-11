#include "10_planner/Planner.h"
#include "config.h"
#include "07_logistics/Pallet.h"
#include "12_signal/Signal.h"
#include "14_runlog/RunLog.h"

#define MAX_CAND 8

static uint8_t s_adj[64];                 // 교차점마다 통로가 있는 방향 (비트 h)
static const uint8_t* s_cand = nullptr;
static uint8_t s_nCand = 0, s_openLanes = 1;
static LaneState s_lane[MAX_CAND];

uint8_t  planPath[64];
uint8_t  planLen = 0;
uint16_t planCost = 0;

// ---------------------------------------------------------------- 장애물 후보 통로 상태
static int8_t candIndex(uint8_t n)
{
    for (uint8_t i = 0; i < s_nCand; i++) if (s_cand[i] == n) return i;
    return -1;
}

static int8_t candInCol(uint8_t col)
{
    for (uint8_t i = 0; i < s_nCand; i++) if (colOf(s_cand[i]) == col) return i;
    return -1;
}

// 열린 곳을 다 찾았으면 나머지는 막힘, 막힌 곳을 다 찾았으면 나머지는 열림
static void deduceLanes()
{
    uint8_t nOpen = 0, nBlk = 0;
    for (uint8_t i = 0; i < s_nCand; i++) {
        if (s_lane[i] == L_OPEN) nOpen++;
        if (s_lane[i] == L_BLOCKED) nBlk++;
    }
    LaneState fill = L_UNKNOWN;
    if (nOpen >= s_openLanes) fill = L_BLOCKED;
    else if (nBlk >= s_nCand - s_openLanes) fill = L_OPEN;
    if (fill != L_UNKNOWN)
        for (uint8_t i = 0; i < s_nCand; i++) if (s_lane[i] == L_UNKNOWN) s_lane[i] = fill;
}

static void setLane(int8_t i, LaneState st)
{
    if (i < 0 || s_lane[i] == st) return;
    s_lane[i] = st;
    deduceLanes();
    Serial.print(F("[lane]"));
    uint8_t packed = 0xFF;                            // 주행 기록: 통로마다 2비트 (없는 칸은 3)
    for (uint8_t k = 0; k < s_nCand; k++) {
        Serial.print(' '); printNode(s_cand[k]); Serial.print('=');
        Serial.print("?OX"[s_lane[k]]);
        if (k < 4) packed = (uint8_t)((packed & ~(3 << (2 * k))) | (s_lane[k] << (2 * k)));
    }
    Serial.println();
    runlogAdd(RL_LANE, packed);
}

bool openLaneKnown()
{
    for (uint8_t i = 0; i < s_nCand; i++) if (s_lane[i] == L_OPEN) return true;
    return false;
}

// ---------------------------------------------------------------- Nav 콜백
// 미확인 후보 통로(세로, B~F행 사이) 칸에서만 전방 IR 감시
static bool hopCheck(uint8_t from, uint8_t to)
{
    if (palletCarrying() && !FRONT_CHECK_WHILE_CARRY) return false;
    if (colOf(from) != colOf(to)) return false;
    int8_t i = candInCol(colOf(to));
    if (i < 0 || s_lane[i] != L_UNKNOWN) return false;
    uint8_t r1 = rowOf(from), r2 = rowOf(to);
    return r1 >= 1 && r1 <= 5 && r2 >= 1 && r2 <= 5;        // B(1) ~ F(5)
}

static void onNode(uint8_t n)
{
    setLane(candIndex(n), L_OPEN);       // 후보 교차점을 지나갔다 = 열린 통로
}

// ---------------------------------------------------------------- 맵
void plannerBegin(const Seg* blue, uint8_t nBlue, const uint8_t* cand, uint8_t nCand,
                  uint8_t openLanes, Nav& nav)
{
    memset(s_adj, 0, sizeof(s_adj));
    for (uint8_t i = 0; i < nBlue; i++) {
        uint8_t n = blue[i].a;
        int8_t h = headingTo(blue[i].a, blue[i].b);
        if (h < 0) uiFatal(F("BLUE seg not straight"), 10);
        while (n != blue[i].b) {
            uint8_t m = stepNode(n, h);
            s_adj[n] |= 1 << h;
            s_adj[m] |= 1 << reverseH(h);
            n = m;
        }
    }
    if (nCand > MAX_CAND) uiFatal(F("too many candidates"), 11);
    s_cand = cand;
    s_nCand = nCand;
    s_openLanes = openLanes;
    for (uint8_t i = 0; i < nCand; i++) s_lane[i] = L_UNKNOWN;
    nav.hopCheck = hopCheck;
    nav.onNode = onNode;
}

bool plannerHasEdge(uint8_t a, uint8_t b)
{
    int8_t h = headingTo(a, b);
    return h >= 0 && straightDist(a, b) == 1 && (s_adj[a] & (1 << h));
}

// 0 = 사용 불가, 1 = 사용 가능, 2 = 미확인 통로(벌점)
static uint8_t edgeUse(uint8_t a, uint8_t h, bool allowUnknown)
{
    if (!(s_adj[a] & (1 << h))) return 0;
    uint8_t b = stepNode(a, h);
    int8_t ia = candIndex(a), ib = candIndex(b);
    LaneState st = L_OPEN;
    if (ia >= 0) st = s_lane[ia];
    if (ib >= 0 && s_lane[ib] != L_OPEN) st = s_lane[ib];
    if (st == L_BLOCKED) return 0;
    if (st == L_UNKNOWN) return allowUnknown ? 2 : 0;
    return 1;
}

// ---------------------------------------------------------------- 최단 경로 (Dijkstra, 상태 = 교차점×방향)
static uint16_t s_dist[256];
static uint8_t  s_prev[256];
static uint8_t  s_done[32];

bool plan(uint8_t start, uint8_t h0, uint8_t goal, bool allowUnknown)
{
    for (int i = 0; i < 256; i++) { s_dist[i] = 0xFFFF; s_prev[i] = 0; }
    memset(s_done, 0, sizeof(s_done));
    uint8_t s0 = start * 4 + h0;
    s_dist[s0] = 0;
    s_prev[s0] = s0;
    int best = -1;

    for (;;) {
        int u = -1;
        uint16_t du = 0xFFFF;
        for (int i = 0; i < 256; i++)
            if (!(s_done[i >> 3] & (1 << (i & 7))) && s_dist[i] < du) { du = s_dist[i]; u = i; }
        if (u < 0) break;
        s_done[u >> 3] |= 1 << (u & 7);
        uint8_t n = u >> 2, h = u & 3;
        if (n == goal) { best = u; break; }
        for (uint8_t d = 0; d < 4; d++) {
            uint8_t use = edgeUse(n, d, allowUnknown);
            if (!use) continue;
            uint8_t dt = (d - h) & 3;
            uint16_t c = du + PLAN_COST_HOP + (dt == 2 ? PLAN_COST_UTURN : (dt ? PLAN_COST_TURN : 0))
                         + (use == 2 ? PLAN_COST_UNKNOWN : 0);
            uint8_t v = stepNode(n, d) * 4 + d;
            if (c < s_dist[v]) { s_dist[v] = c; s_prev[v] = u; }
        }
    }
    if (best < 0) return false;
    planCost = s_dist[best];

    uint8_t rev[64];
    uint8_t k = 0;
    uint8_t st = best;
    for (;;) {
        rev[k++] = st >> 2;
        if (st == s0) break;
        st = s_prev[st];
    }
    planLen = 0;
    while (k) planPath[planLen++] = rev[--k];
    return true;
}

// ---------------------------------------------------------------- 이동
bool gotoNode(Nav& nav, uint8_t goal)
{
    for (uint8_t attempt = 0; attempt < 8; attempt++) {
        bool allowUnknown = !palletCarrying() || FRONT_CHECK_WHILE_CARRY;
        if (!plan(nav.node, nav.hdg, goal, allowUnknown)) return false;

        Serial.print(F("[plan]"));
        for (uint8_t i = 0; i < planLen; i++) { Serial.print(' '); printNode(planPath[i]); }
        Serial.println();
        runlogAdd(RL_PLAN, goal, planLen);

        if (planLen == 1) {                        // 이미 goal (접근 중이면 도착)
            nav.leg(goal, true);
            return true;
        }
        // 경로를 직선 구간으로 묶어 실행 (방향이 바뀌는 곳에서만 정지·회전)
        NavResult r = NAV_OK;
        uint8_t prev = planPath[0];
        for (uint8_t i = 1; i < planLen && r == NAV_OK; i++) {
            uint8_t next = (i + 1 < planLen) ? planPath[i + 1] : NODE_NONE;
            if (next != NODE_NONE && !Nav::mustStopAt(prev, planPath[i], next)) continue;
            r = nav.leg(planPath[i], true);
            prev = planPath[i];
        }
        if (r == NAV_OK) return true;

        // 장애물: 그 열의 후보 통로를 막힘 처리 → 칸 중간에서 U턴 → 다시 계산
        setLane(candInCol(colOf(nav.obsTo)), L_BLOCKED);
        nav.uturnOnEdge();
    }
    return false;
}

void ensureOpenLane(Nav& nav)
{
    while (!openLaneKnown()) {
        int8_t pick = -1;
        uint16_t bestCost = 0xFFFF;
        for (uint8_t i = 0; i < s_nCand; i++) {
            if (s_lane[i] != L_UNKNOWN) continue;
            if (plan(nav.node, nav.hdg, s_cand[i], true) && planCost < bestCost) {
                bestCost = planCost; pick = i;
            }
        }
        if (pick < 0) uiFatal(F("no lane to probe"), 12);
        Serial.print(F("[probe] ")); printNode(s_cand[pick]); Serial.println();
        gotoNode(nav, s_cand[pick]);   // 막혀 있으면 false로 돌아오고 통로 상태가 갱신됨
    }
}
