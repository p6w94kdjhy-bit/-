// =====================================================================
//  모터 좌우 균형 찾기 — 속도 단계별로 '선 없이 똑바로 가는' 좌/우 보정값(%)을 찾는다   (시리얼 115200)
//
//  이 스케치는 config.h 의 MOTOR_L_GAIN / MOTOR_R_GAIN 을 쓰지 않고, 여기서 바꾼 값만으로 모터를 돌린다.
//
//  쓰는 법
//   1) 로봇을 긴 직선(선이나 타일 줄눈) 위에 똑바로 놓는다
//   2) 1 / 2 / 3 / 4 : 속도 단계 고르기   (1 = 정차 직전 속도, 2 = 운반 속도, 3 = 통과 속도, 4 = 더 빠른 속도)
//   3) g : 앞으로 달리기 (버튼을 눌러도 같음)       b : 뒤로 달리기 (후진용 값은 따로 저장됨)
//   4) 휘는 쪽을 보고 값 바꾸기 → 다시 g
//        오른쪽으로 휨 = 왼쪽 바퀴가 셈  →  a (왼쪽 -1)        크게 휘면 A (왼쪽 -5)
//        왼쪽으로 휨   = 오른쪽 바퀴가 셈 →  s (오른쪽 -1)      크게 휘면 S (오른쪽 -5)
//        너무 줄였으면  q (왼쪽 +1) / w (오른쪽 +1)             ※ 100 을 넘길 수 없음 (센 쪽을 줄여서 맞춘다)
//   5) 끝까지 선과 1cm 안쪽으로 가면 그 단계 완료 → 다음 단계
//   6) p : 찾은 값 전체 출력 → 그 블록을 복사해서 전달
//
//  달리는 거리는 단계마다 비슷하게(약 60~80cm) 맞춰 두었다. 더 길게 보려면 + , 짧게는 -  (0.5초씩)
// =====================================================================
#include <AGV2.h>

const uint8_t N_STAGE = 4;
const int STAGE_PWM[N_STAGE] = { SPEED_APPROACH, SPEED_CARRY, SPEED_CRUISE, 210 };
int runMs[N_STAGE]           = { 4000, 3000, 1800, 1500 };

// [단계][0 = 전진, 1 = 후진][0 = 왼쪽, 1 = 오른쪽]  보정 %
int gain[N_STAGE][2][2];
uint8_t stage = 0;

void rawDrive(int l, int r)                     // config 보정 없이 그대로 (-255 ~ +255, + = 전진)
{
    digitalWrite(PIN_L_DIR, l >= 0 ? LEFT_FWD_LEVEL : !LEFT_FWD_LEVEL);
    digitalWrite(PIN_R_DIR, r >= 0 ? RIGHT_FWD_LEVEL : !RIGHT_FWD_LEVEL);
    analogWrite(PIN_L_PWM, min(255, abs(l)));
    analogWrite(PIN_R_PWM, min(255, abs(r)));
}

void show()
{
    Serial.print(F("단계 ")); Serial.print(stage + 1);
    Serial.print(F("  PWM ")); Serial.print(STAGE_PWM[stage]);
    Serial.print(F("   전진 L/R = ")); Serial.print(gain[stage][0][0]); Serial.print('/'); Serial.print(gain[stage][0][1]);
    Serial.print(F("   후진 L/R = ")); Serial.print(gain[stage][1][0]); Serial.print('/'); Serial.print(gain[stage][1][1]);
    Serial.print(F("   시간 ")); Serial.print(runMs[stage]); Serial.println(F("ms"));
}

void printAll()
{
    Serial.println(F("===== 모터 좌우 균형 (이 줄부터 '끝'까지 복사해서 전달) ====="));
    for (uint8_t i = 0; i < N_STAGE; i++) {
        Serial.print(F("단계 ")); Serial.print(i + 1);
        Serial.print(F("  PWM ")); Serial.print(STAGE_PWM[i]);
        Serial.print(F("  전진 L ")); Serial.print(gain[i][0][0]); Serial.print(F(" R ")); Serial.print(gain[i][0][1]);
        Serial.print(F("  후진 L ")); Serial.print(gain[i][1][0]); Serial.print(F(" R ")); Serial.println(gain[i][1][1]);
    }
    Serial.println(F("===== 끝 ====="));
}

uint8_t lastDir = 0;                            // 마지막으로 달린 방향 (값 바꾸기 a s q w 가 적용될 쪽)

void run(uint8_t dir)                           // 0 = 전진, 1 = 후진
{
    lastDir = dir;
    int pwm = STAGE_PWM[stage];
    int gl = gain[stage][dir][0], gr = gain[stage][dir][1];
    Serial.print(dir ? F("> 후진  ") : F("> 전진  "));
    show();
    delay(600);                                 // 손 떼는 시간
    unsigned long t0 = millis();
    while (millis() - t0 < (unsigned long)runMs[stage]) {
        long e = millis() - t0;
        int s = e * ACCEL_PER_MS < pwm ? (int)(e * ACCEL_PER_MS) : pwm;      // 주행 코드와 같은 출발 가속
        int l = (int)((long)s * gl / 100), r = (int)((long)s * gr / 100);
        if (dir) rawDrive(-l, -r); else rawDrive(l, r);
        if (millis() - t0 > 400 && digitalRead(PIN_BUTTON) == LOW) break;    // 달리는 중 버튼 = 정지
    }
    rawDrive(0, 0);
    while (digitalRead(PIN_BUTTON) == LOW) {}
    delay(200);
}

void adjust(uint8_t side, int d)
{
    int& g = gain[stage][lastDir][side];
    g += d;
    if (g > 100) g = 100;
    if (g < 50) g = 50;
    Serial.print(lastDir ? F("(후진 값) ") : F("(전진 값) "));
    show();
}

void setup()
{
    Serial.begin(115200);
    driveInit();
    uiInit();
    for (uint8_t i = 0; i < N_STAGE; i++)
        for (uint8_t d = 0; d < 2; d++) { gain[i][d][0] = MOTOR_L_GAIN; gain[i][d][1] = MOTOR_R_GAIN; }
    liftInit();
    Serial.println(F("motor_balance: 1 2 3 4 = 속도 단계 / g 전진 b 후진 / a s (-1) A S (-5) q w (+1) / + - 시간 / p 출력"));
    show();
}

void loop()
{
    if (Serial.available()) {
        char c = Serial.read();
        switch (c) {
            case '1': case '2': case '3': case '4': stage = c - '1'; show(); break;
            case 'g': run(0); break;
            case 'b': run(1); break;
            case 'a': adjust(0, -1); break;
            case 'A': adjust(0, -5); break;
            case 'q': adjust(0, +1); break;
            case 's': adjust(1, -1); break;
            case 'S': adjust(1, -5); break;
            case 'w': adjust(1, +1); break;
            case '+': runMs[stage] += 500; show(); break;
            case '-': if (runMs[stage] > 500) runMs[stage] -= 500; show(); break;
            case 'p': printAll(); break;
        }
    }
    if (buttonDown()) {                         // 버튼 = 마지막 방향으로 다시 달리기
        while (buttonDown()) {}
        run(lastDir);
    }
}
