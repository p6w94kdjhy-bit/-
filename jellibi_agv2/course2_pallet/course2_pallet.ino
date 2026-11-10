// =====================================================================
//  종목② 팔레트 운반 + 장애물 (제한 180초)
//   배송 ① A2 → H6, ② A6 → H2 (한 번에 1개), 이후 H4 복귀 2초 정지.
//   파란 통로만 사용. D2·D4·D6 중 2곳 차단(추첨) → 전방 IR로 감지 후 경로 재계산.
//
//  이 파일에는 '맵 + 임무 + 순서'만 있다. 실제 동작은 라이브러리 기능별 폴더:
//    이동·재탐색   10_planner (gotoNode)       팔레트   07_logistics (PickPallet, DropPallet)
//    라인트레이싱  03_linetrace               회전     04_turn          장애물  05_obstacle
//
//  13:20 제출 후 이 파일과 라이브러리 코드는 고정. 보정은 config.h 만 수정.
// =====================================================================
#include <AGV2.h>

// ----------------------------------------------------------------- 맵
// 파란 통로(직선 구간 목록).  ※ 현장 맵 사진으로 확인 필요
const Seg BLUE[] = {
    { P('A', 2), P('H', 2) },   // 2열 전체 (A2 픽업 ~ H2 배송)
    { P('A', 6), P('H', 6) },   // 6열 전체 (A6 픽업 ~ H6 배송)
    { P('B', 4), P('H', 4) },   // 4열 (B행 ~ H4 복귀점)
    { P('B', 2), P('B', 6) },   // B행 연결
    { P('G', 2), P('G', 6) },   // G행 연결
};

// 장애물 후보(추첨으로 일부 차단) — 이 교차점에 닿는 칸이 막힘
const uint8_t CAND[] = { P('D', 2), P('D', 4), P('D', 6) };
const uint8_t OPEN_LANES = 1;          // 열린 통로 수 (3곳 중 2곳 차단 → 1)

// ----------------------------------------------------------------- 임무
struct Job { uint8_t from, to; };
const Job JOBS[2] = {
    { P('A', 2), P('H', 6) },   // ①
    { P('A', 6), P('H', 2) },   // ②
};
const uint8_t PARK_NODE = P('H', 4);   // 두 배송 후 복귀, 2초 정지
#define AUTO_ORDER 1                   // 1 = 열린 통로를 안 뒤 더 빠른 배송 순서 자동 선택

// 출발: START_NODE 바깥 50mm에서 START_HDG 방향을 보고 대기   ※ 현장 확인 필요
const uint8_t START_NODE = P('H', 4);
const uint8_t START_HDG  = HN;

Nav nav;

// ----------------------------------------------------------------- 순서 정하기
// 예상 비용: 현재 위치 → 배송 a → 배송 b → 복귀 (경로 비용 합)
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
        if (planLen >= 2) h = headingTo(planPath[planLen - 2], planPath[planLen - 1]);
        n = goals[k];
        if (carry) {                                   // 하역 후 후진·U턴 → 뒤 교차점으로 접근
            h = reverseH(h);
            n = stepNode(n, h);
            total += PLAN_COST_UTURN + PLAN_COST_HOP;
        }
    }
    return total;
}

// ----------------------------------------------------------------- 배송 1건
void deliver(const Job& j)
{
    if (!FRONT_CHECK_WHILE_CARRY) ensureOpenLane(nav);       // 빈 차로 열린 통로 먼저 확인
    if (!gotoNode(nav, j.from)) uiFatal(F("no path to pallet"));
    PickPallet(nav);                                          // 들기
    if (!gotoNode(nav, j.to)) uiFatal(F("no path to drop"));
    DropPallet(nav);                                          // 내리기 + 빠져나오기
}

// ----------------------------------------------------------------- main
void setup()
{
    Serial.begin(115200);
    driveInit();
    uiInit();
    liftInit();
    palletInit();

    if (!irCalibrated())    uiFatal(F("config.h IR 현장값 미입력"));
    if (!frontCalibrated()) uiFatal(F("config.h FRONT_OBSTACLE_TH 미입력"));

    plannerBegin(BLUE, sizeof(BLUE) / sizeof(BLUE[0]), CAND, sizeof(CAND), OPEN_LANES, nav);

    Serial.println(F("COURSE2 ready - waiting start"));
    beepReady();
    uiWaitStart();
    uiArmAbort();
    unsigned long t0 = millis();

    // 출발 위치에서 센서가 첫 교차점을 이미 지났으면 그 교차점 위에 있는 것으로 시작
    if (START_XING_PASSED) nav.setAt(START_NODE, START_HDG);
    else                   nav.setApproach(START_NODE, START_HDG);
    if (!FRONT_CHECK_WHILE_CARRY) ensureOpenLane(nav);

    uint8_t first = 0;
#if AUTO_ORDER
    if (orderCost(1) < orderCost(0)) first = 1;
#endif
    Serial.print(F("[order] first=")); Serial.println(first + 1);
    deliver(JOBS[first]);
    deliver(JOBS[1 - first]);

    if (!gotoNode(nav, PARK_NODE)) uiFatal(F("no path to park"));
    nav.hold(STOP_HOLD_MS);

    Stop();
    Serial.print(F("DONE ms="));
    Serial.println(millis() - t0);
    beepDone();
}

void loop() {}
