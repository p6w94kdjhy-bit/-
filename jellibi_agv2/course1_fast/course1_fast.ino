// =====================================================================
//  종목① 지정경로 순회정차 — 속도 올린 판 (course1_fast).  원본 course1_drive 는 그대로
//  원본과 다른 점 (값은 config.h 의 C1F_...)
//   1) 통과 구간 속도  C1F_CRUISE (원본 SPEED_CRUISE 168)
//   2) 늦은 감속       정차할 칸의 앞부분 C1F_LATE_BRAKE_PCT % 를 통과 속도로 → 남은 구간만 SPEED_APPROACH
//                      (한 칸 통과 시간을 주행 중에 직접 재서 씀 → 배터리가 달라도 같은 거리에서 감속)
//   3) (선택) 부드러운 조향  C1F_LT_SMOOTH 1
//   경로표·회전·정차 방식은 원본과 같음
//
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
#if C1F_LT_SMOOTH
    lineTraceSmooth(V3_LT_KP, V3_LT_TD_MS, V3_LT_MAX_PCT, V3_LT_SLOW_PCT);   // 부드러운 조향 (선택)
#endif
    Serial.print(F("fast: CRUISE ")); Serial.print(C1F_CRUISE);
    Serial.print(F("  LATE_BRAKE ")); Serial.print(C1F_LATE_BRAKE_PCT);
    Serial.print(F("%  SMOOTH ")); Serial.println(C1F_LT_SMOOTH);
    Serial.println(F("COURSE1 fast ready - waiting start"));
    beepReady();
    uiWaitStart();
    uiArmAbort();
    unsigned long t0 = millis();
    lineTraceReset();

    // 출발 위치에서 센서가 이미 H2 선을 지나 있으면(START_XING_PASSED=1) H2는 세지 않고 G2부터
    uint8_t first = START_XING_PASSED ? 1 : 0;
    bool moving = false;             // 직전 교차점을 멈추지 않고 통과했나
    unsigned long hopMs = 0;         // 통과 속도로 한 칸 가는 시간 (가장 짧았던 값, 늦은 감속에 사용)

    for (uint8_t i = first; i < ROUTE_LEN; i++) {
        const Step& st = ROUTE[i];
        bool stopHere = (st.act != GO) || st.hold;

        // ① 다음 교차점까지 라인트레이싱 (멈출 교차점 직전 칸은 느리게, 단 앞부분은 통과 속도로 = 늦은 감속)
#if C1F_LATE_BRAKE_PCT > 0
        if (stopHere && hopMs > 0) lineTraceBoost(C1F_CRUISE, hopMs * C1F_LATE_BRAKE_PCT / 100);
#endif
        unsigned long tHop = millis();
        LineTraceToCross(stopHere ? SPEED_APPROACH : C1F_CRUISE);
        if (!stopHere && moving) {                        // 멈춤 없이 통과 속도로 한 칸 → 시간 기억
            unsigned long dt = millis() - tHop;
            if (hopMs == 0 || dt < hopMs) hopMs = dt;
        }
        beepNode();
        Serial.print(st.cross);

        // ② 통과
        if (!stopHere) {
            Serial.println(F(" GO"));
            moving = true;
            continue;
        }
        moving = false;

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
