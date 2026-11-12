// =====================================================================
//  주행 보정 테스트 (시리얼 115200). 명령 입력 후 버튼을 누르면 같은 명령 반복.
//   s : 라인 없이 직진 1.5초  → 휘는 방향으로 MOTOR_L/R_GAIN, 방향 반대면 *_FWD_LEVEL
//   f : 교차점 1칸 라인트레이스 + 정렬 정지 → 바퀴축이 교차점 위인지 확인 (ALIGN_MS)
//   F : 3칸 + 정지 (교차점 1회 카운트 확인, XING_SKIP_MS / XING_LEVEL_PCT)
//   l / r / u : 좌90 / 우90 / U턴 (센서 회전)
//   L : 센서 없이 TURN90_MS×4 왼쪽 회전 → 정확히 한 바퀴면 TURN90_MS OK
//   q : 사각 주행 (1칸 + 우회전) ×4 → 제자리 복귀 확인
//   p / d : 리프트 올림 / 내림 (LIFT_UP/DOWN_ANGLE)
//   , / . : 리프트 각도를 5° 내림 / 올림 (바로 움직임) → 완전히 내려간 각도를 찾아 config.h LIFT_DOWN_ANGLE 에,
//           적재함이 충분히 들린 각도를 LIFT_UP_ANGLE 에 적는다.  서보가 '웅' 소리를 내며 버티면 너무 간 것 → 5° 되돌림
//   x : 하역 후 후진 탈출 (EXIT_BACK_MS) + U턴
//   v : 시험 속도 바꾸기 (f F q 의 통과 속도) 168 → 190 → 210 → 230 → 255 → 168 ...  업로드 없이 속도별 조향 확인
//       F 를 달리면 칸마다 'err=' (가장 크게 치우친 정도 %) 가 나온다. 칸마다 40 이하면 그 속도에서 조향 OK
//   k / K : (종목② 조향) 게인 KP 5 내림 / 올림      t / T : 미분 TD 15 내림 / 올림
//       업로드 없이 바로 적용. 바꿀 때마다 config.h 에 적을 줄이 출력됨 → 마음에 드는 값을 config.h 에 옮겨 적고 업로드
//   B : 라인 없이 후진 3초 (선 위에서 출발해 끝에서 선과 얼마나 벌어졌는지 확인) → 비뚤어지는 방향으로 REV_L_GAIN / REV_R_GAIN (course2_drive 후진)
//   b : 후진 시험 — E4 앞에서 북쪽으로 C4 까지 3칸 → E4 까지 교차점을 세며 후진 → E4 에 다시 정렬
//       (선 3칸 이상 필요. 정지 후 바퀴축이 출발 교차점 위인지, 비뚤어졌는지 확인 → REV_* 조정)
//   c : 라인트레이싱 방식 바꾸기 — 종목①(비례) ↔ 종목②(부드러운 PD, config.h C2_LT_*). 바꾼 뒤 f / F / q 로 시험
//   g : 지난 주행 기록 출력 (종목② 가 저장한 것)
// =====================================================================
#include <AGV2.h>

Nav nav;
char lastCmd = 0;
bool smoothOn = false;
int liftAngle = LIFT_DOWN_ANGLE;
const int TEST_SPEEDS[] = { SPEED_CRUISE, 190, 210, 230, 255 };
uint8_t speedIdx = 0;
int kp = C2_LT_KP, td = C2_LT_TD_MS;

void applySmooth()
{
    lineTraceSmooth(smoothOn ? kp : 0, td, C2_LT_MAX_PCT, C2_LT_SLOW_PCT);
    Serial.print(smoothOn ? F("조향: 종목② 부드러운 PD") : F("조향: 종목① 비례 (k K t T 는 종목② 조향일 때만 효과)"));
    Serial.print(F("   시험 속도 ")); Serial.println(nav.cruise);
    Serial.print(F("  config.h 에 적을 값 →  #define C2_LT_KP ")); Serial.print(kp);
    Serial.print(F("   #define C2_LT_TD_MS ")); Serial.println(td);
}

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
        case 'p': LiftUp(); liftAngle = LIFT_UP_ANGLE; break;
        case 'd': LiftDown(); liftAngle = LIFT_DOWN_ANGLE; break;
        case ',': case '.':
            liftAngle += (c == '.') ? 5 : -5;
            if (liftAngle < 0) liftAngle = 0;
            if (liftAngle > 180) liftAngle = 180;
            LiftGoto(liftAngle);
            Serial.print(F("리프트 각도 = ")); Serial.print(liftAngle);
            Serial.println(F("   (내린 위치면 LIFT_DOWN_ANGLE, 올린 위치면 LIFT_UP_ANGLE 에 이 숫자)"));
            return;
        case 'x': Backward(EXIT_BACK_PWM); delay(EXIT_BACK_MS); Brake(); UTurnOnEdge(); break;
        case 'c':
            smoothOn = !smoothOn;
            applySmooth();
            return;
        case 'k': if (kp > 10) kp -= 5;  applySmooth(); return;
        case 'K': if (kp < 100) kp += 5; applySmooth(); return;
        case 't': if (td >= 15) td -= 15; applySmooth(); return;
        case 'T': if (td < 200) td += 15; applySmooth(); return;
        case 'g': runlogDump(); return;
        case 'B': { int sp = REV_PWM; drive(-(int)((long)sp * REV_L_GAIN / 100), -(int)((long)sp * REV_R_GAIN / 100)); delay(3000); Brake(); break; }
        case 'b':
            if (!needIr()) break;
            nav.setApproach(P('E', 4), HN); nav.leg(P('C', 4), true); delay(500); nav.backTo(P('E', 4)); break;
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
    liftInit();                      // 리프터 내리기 — 교본 26쪽: 초기화가 끝난 뒤 실행 (전원 켤 때 올라가는 현상 처리)
    Serial.println(F("tune_drive: s f F l r u L q p d , . x B b c g v k K t T  (버튼=반복)"));
}

void loop()
{
    if (Serial.available()) run(Serial.read());
    if (lastCmd && buttonDown()) {
        while (buttonDown()) {}
        run(lastCmd);
    }
}
