// =====================================================================
//  주행 보정 테스트 (시리얼 115200). 명령 입력 후 버튼을 누르면 같은 명령 반복.
//   s : 라인 없이 직진 1.5초  → 휘는 방향으로 MOTOR_L/R_GAIN, 방향 반대면 *_FWD_LEVEL
//   f : 교차점 1칸 라인트레이스 + 정렬 정지 → 바퀴축이 교차점 위인지 확인 (ALIGN_MS)
//   F : 3칸 + 정지 (교차점 1회 카운트 확인, XING_SKIP_MS / XING_LEVEL_PCT)
//   l / r / u : 좌90 / 우90 / U턴 (센서 회전)
//   L : 센서 없이 TURN90_MS×4 왼쪽 회전 → 정확히 한 바퀴면 TURN90_MS OK
//   q : 사각 주행 (1칸 + 우회전) ×4 → 제자리 복귀 확인
//   p / d : 리프트 올림 / 내림 (LIFT_UP/DOWN_ANGLE)
//   x : 하역 후 후진 탈출 (EXIT_BACK_MS) + U턴
//   v : 시험 속도 바꾸기 (f F q 의 통과 속도) 168 → 190 → 210 → 230 → 255 → 168 ...  업로드 없이 속도별 조향 확인
//       F 를 달리면 칸마다 'err=' (가장 크게 치우친 정도 %) 가 나온다. 칸마다 40 이하면 그 속도에서 조향 OK
//   c : 라인트레이싱 방식 바꾸기 — 종목①(비례) ↔ 종목②(부드러운 PD, config.h C2_LT_*). 바꾼 뒤 f / F / q 로 시험
//   g : 지난 주행 기록 출력 (종목② 가 저장한 것)
// =====================================================================
#include <AGV2.h>

Nav nav;
char lastCmd = 0;
bool smoothOn = false;
const int TEST_SPEEDS[] = { SPEED_CRUISE, 190, 210, 230, 255 };
uint8_t speedIdx = 0;

bool needIr()
{
    if (irCalibrated()) return true;
    Serial.println(F("config.h IR 현장값 먼저 입력"));
    beep(220, 300);
    return false;
}

void run(char c)
{
    Serial.print(F("> ")); Serial.println(c);
    delay(500);
    switch (c) {
        case 's': Forward(SPEED_CRUISE); delay(1500); Brake(); break;
        case 'f':
            if (!needIr()) break;
            nav.setApproach(P('D', 4), HN); nav.leg(P('D', 4), true); break;
        case 'F':
            if (!needIr()) break;
            nav.setApproach(P('E', 4), HN); nav.leg(P('C', 4), true); break;
        case 'l': TurnLeft90(); break;
        case 'r': TurnRight90(); break;
        case 'u': UTurn(); break;
        case 'L': spinDrive(-TURN_PWM, TURN_PWM); delay(4UL * TURN90_MS); Brake(); break;
        case 'q':
            if (!needIr()) break;
            nav.setAt(P('D', 4), HN);
            nav.leg(P('C', 4), true); nav.turnTo(HE);
            nav.leg(P('C', 5), true); nav.turnTo(HS);
            nav.leg(P('D', 5), true); nav.turnTo(HW);
            nav.leg(P('D', 4), true); nav.turnTo(HN);
            break;
        case 'p': LiftUp(); break;
        case 'd': LiftDown(); break;
        case 'x': Backward(EXIT_BACK_PWM); delay(EXIT_BACK_MS); Brake(); UTurnOnEdge(); break;
        case 'c':
            smoothOn = !smoothOn;
            lineTraceSmooth(smoothOn ? C2_LT_KP : 0, C2_LT_TD_MS, C2_LT_MAX_PCT, C2_LT_SLOW_PCT);
            Serial.println(smoothOn ? F("라인트레이싱: 종목② 부드러운 PD") : F("라인트레이싱: 종목① 비례"));
            return;
        case 'g': runlogDump(); return;
        case 'v':
            speedIdx = (speedIdx + 1) % (sizeof(TEST_SPEEDS) / sizeof(TEST_SPEEDS[0]));
            nav.cruise = TEST_SPEEDS[speedIdx];
            Serial.print(F("시험 속도 = ")); Serial.println(nav.cruise);
            return;
        default: return;
    }
    Stop();
    lastCmd = c;
    beep(1047, 40);
}

void setup()
{
    Serial.begin(115200);
    driveInit();
    uiInit();
    showConfig();
    liftInit();
    Serial.println(F("tune_drive: s f F l r u L q p d x c g v  (버튼=반복)"));
}

void loop()
{
    if (Serial.available()) run(Serial.read());
    if (lastCmd && buttonDown()) {
        while (buttonDown()) {}
        run(lastCmd);
    }
}
