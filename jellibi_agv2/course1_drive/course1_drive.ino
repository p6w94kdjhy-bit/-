// =====================================================================
//  종목① 지정경로 순회정차 (제한 90초)
//  H2 → F2 → F5 → C5(2초 정차) → C7 → B7(U턴) → C7 → F7 → F6 → H6(2초 정차, 남향)
//  H2 남쪽 50mm 바깥에서 북쪽(A행 방향)을 보고 출발.
//
//  동작 원리 — 교차점 하나마다 아래를 반복:
//    ① 라인트레이싱으로 다음 교차점까지 달린다      LineTraceToCross()   (LineTrace.cpp)
//    ② 그 교차점의 동작이 '통과(GO)'면 그대로 계속
//    ③ 아니면 교차점 위에 바퀴축을 맞추고 정지       AlignOnCross()       (LineTrace.cpp)
//    ④ 정차가 있으면 2초 정차                        Hold()
//    ⑤ 회전                                          TurnLeft90 / TurnRight90 / UTurn  (Turn.cpp)
//    ⑥ (경로표 adjDeg 가 0이 아니면) 그 교차점만 각도 보정  TurnAdjust()
//
//  보정값은 libraries/AGV2/src/config.h 에서만 수정.
// =====================================================================
#include <AGV2.h>

enum Action : uint8_t {
    GO,       // 직진 통과 (멈추지 않음)
    LEFT,     // 정지 → 좌회전 90°
    RIGHT,    // 정지 → 우회전 90°
    UTURN,    // 정지 → 180°
    END       // 정지 (마지막)
};

struct Step {
    char   cross[3];   // 교차점 이름 (로그용)
    Action act;        // 이 교차점에서 할 동작
    bool   hold;       // 동작 전에 2초 정차
    int8_t adjDeg;     // ★ 이 교차점의 회전만 각도 보정 (°): + = 더 돎, - = 덜 돎, 0 = 그대로
                       //   (config.h 좌·우·U턴 공통값은 그대로 두고 이 교차점만 따로 맞출 때)
};

// ---------------------------------------------------------------------
//  경로표: 만나는 교차점 순서대로 한 줄씩
// ---------------------------------------------------------------------
const Step ROUTE[] = {
    // 교차점  동작    2초정차  각도보정(°)   (진행 방향)
    { "H2",  GO,     false,     0 },   // 출발 직후 첫 교차점 (북쪽으로)
    { "G2",  GO,     false,     0 },
    { "F2",  RIGHT,  false,     0 },   // 북 → 동
    { "F3",  GO,     false,     0 },
    { "F4",  GO,     false,     0 },
    { "F5",  LEFT,   false,     0 },   // 동 → 북
    { "E5",  GO,     false,     0 },
    { "D5",  GO,     false,     0 },
    { "C5",  RIGHT,  true ,     0 },   // 북쪽으로 도착 → 2초 정차 → 우회전 (북 → 동)
    { "C6",  GO,     false,     0 },
    { "C7",  LEFT,   false,     0 },   // 동 → 북
    { "B7",  UTURN,  false,     0 },   // 북 → 남
    { "C7",  GO,     false,     0 },   // 돌아오면서 통과
    { "D7",  GO,     false,     0 },
    { "E7",  GO,     false,     0 },
    { "F7",  RIGHT,  false,     0 },   // 남 → 서
    { "F6",  LEFT,   false,     0 },   // 서 → 남   (11-11: -30 → 0, 회전 대칭 후 좌회전 정확)
    { "G6",  GO,     false,     0 },
    { "H6",  END,    true ,     0 },   // 남쪽으로 도착 → 2초 정차 (최종)
};
const uint8_t ROUTE_LEN = sizeof(ROUTE) / sizeof(ROUTE[0]);

void Hold(unsigned long ms)
{
    Stop();
    beep(1319, 80);
    waitMs(ms > 80 ? ms - 80 : 0);
}

void setup()
{
    Serial.begin(115200);
    driveInit();
    uiInit();
    showConfig();

    if (!irCalibrated()) uiFatal(F("config.h IR 현장값 미입력"));

    liftInit();                      // 리프터 내리기 — 교본 26쪽: 초기화가 끝난 뒤 실행 (전원 켤 때 올라가는 현상 처리)
    Serial.println(F("COURSE1 ready - waiting start"));
    beepReady();
    uiWaitStart();
    uiArmAbort();
    unsigned long t0 = millis();
    lineTraceReset();

    // 출발 위치에서 센서가 이미 H2 선을 지나 있으면(START_XING_PASSED=1) H2는 세지 않고 G2부터
    uint8_t first = START_XING_PASSED ? 1 : 0;

    for (uint8_t i = first; i < ROUTE_LEN; i++) {
        const Step& st = ROUTE[i];
        bool stopHere = (st.act != GO) || st.hold;

        // ① 다음 교차점까지 라인트레이싱 (멈출 교차점 직전 칸은 느리게)
        LineTraceToCross(stopHere ? SPEED_APPROACH : SPEED_CRUISE);
        beepNode();
        Serial.print(st.cross);

        // ② 통과
        if (!stopHere) {
            Serial.println(F(" GO"));
            continue;
        }

        // ③ 교차점 위에 정지
        AlignOnCross();

        // ④ 2초 정차
        if (st.hold) {
            Serial.print(F(" HOLD"));
            Hold(STOP_HOLD_MS);
        }

        // ⑤ 회전
        switch (st.act) {
            case LEFT:  Serial.println(F(" LEFT"));  TurnLeft90();  break;
            case RIGHT: Serial.println(F(" RIGHT")); TurnRight90(); break;
            case UTURN: Serial.println(F(" UTURN")); UTurn();       break;
            default:    Serial.println(F(" END"));                  break;
        }

        // ⑥ 이 교차점만의 각도 보정 (경로표 adjDeg)
        if (st.adjDeg != 0 && st.act != END) {
            bool turnLeft = (st.act == LEFT) || (st.act == UTURN && UTURN_LEFT);
            TurnAdjust(turnLeft, st.adjDeg);
        }
    }

    Stop();
    Serial.print(F("DONE ms="));
    Serial.println(millis() - t0);
    beepDone();
}

void loop() {}
