// =====================================================================
//  종목① 지정경로 순회정차 (제한 90초)
//  H2 → F2 → F5 → C5(2초 정차) → C7 → B7(U턴) → C7 → F7 → F6 → H6(2초 정차, 남향)
//  H2 남쪽 50mm 바깥에서 북쪽(A행 방향)을 보고 출발.
//  보정값은 libraries/AGV2/src/config.h 에서만 수정.
// =====================================================================
#include <AGV2.h>

struct Waypoint {
    uint8_t node;
    bool hold;          // 도착 후 2초 정차
};

// 경로: 교차점마다의 동작(직진/좌/우/U턴/정차)은 연속한 웨이포인트의 방향 차이로 자동 결정
//   H2 G2 F2(우) F3 F4 F5(좌) E5 D5 C5(정차→우) C6 C7(좌) B7(U턴) C7 D7 E7 F7(우) F6(좌) G6 H6(정차)
const uint8_t START_NODE = P('H', 2);
const uint8_t START_HDG  = HN;
const Waypoint ROUTE[] = {
    { P('F', 2), false },
    { P('F', 5), false },
    { P('C', 5), true  },   // 북쪽으로 도착 → 2초 정차 → 우회전
    { P('C', 7), false },
    { P('B', 7), false },   // U턴
    { P('C', 7), false },   // 정지 없이 통과
    { P('F', 7), false },
    { P('F', 6), false },
    { P('H', 6), true  },   // 남쪽으로 도착 → 2초 정차 (최종)
};
const uint8_t ROUTE_LEN = sizeof(ROUTE) / sizeof(ROUTE[0]);

Nav nav;

void setup()
{
    Serial.begin(115200);
    driveInit();
    uiInit();
    liftInit();

    if (!irCalibrated()) uiFatal(F("config.h IR 현장값 미입력"));

    Serial.println(F("COURSE1 ready - press button"));
    beepReady();
    uiWaitStart();
    uiArmAbort();
    unsigned long t0 = millis();

    nav.setApproach(START_NODE, START_HDG);
    uint8_t prev = START_NODE;
    for (uint8_t i = 0; i < ROUTE_LEN; i++) {
        uint8_t cur = ROUTE[i].node;
        uint8_t next = (i + 1 < ROUTE_LEN) ? ROUTE[i + 1].node : NODE_NONE;
        bool stop = ROUTE[i].hold || Nav::mustStopAt(prev, cur, next);
        nav.leg(cur, stop);
        if (ROUTE[i].hold) nav.hold(STOP_HOLD_MS);
        prev = cur;
    }

    Stop();
    Serial.print(F("DONE ms="));
    Serial.println(millis() - t0);
    beepDone();
}

void loop() {}
