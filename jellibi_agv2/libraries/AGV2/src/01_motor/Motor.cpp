#include "01_motor/Motor.h"
#include "pins.h"

static int s_lastL = 0, s_lastR = 0;   // Brake()가 반대 방향을 알기 위한 마지막 명령

void driveInit()
{
    pinMode(PIN_L_DIR, OUTPUT);
    pinMode(PIN_R_DIR, OUTPUT);
    pinMode(PIN_L_PWM, OUTPUT);
    pinMode(PIN_R_PWM, OUTPUT);
    pinMode(PIN_IR_L, INPUT);
    pinMode(PIN_IR_R, INPUT);
    pinMode(PIN_IR_FRONT, INPUT);
    Stop();
}

static int clampPwm(long v)
{
    if (v > 255) return 255;
    if (v < -255) return -255;
    return (int)v;
}

// 속도에 따른 좌우 보정 %: 느린 전진(MOTOR_GAIN_LOW_PWM 이하)은 gLow, 빠른 전진(MOTOR_GAIN_HIGH_PWM 이상)은 gHigh, 후진은 gRev
static int gainFor(int cmd, int gHigh, int gLow, int gRev)
{
    if (cmd < 0) return gRev;                                  // 후진
    if (cmd >= MOTOR_GAIN_HIGH_PWM) return gHigh;
    if (cmd <= MOTOR_GAIN_LOW_PWM) return gLow;
    return gLow + (int)((long)(gHigh - gLow) * (cmd - MOTOR_GAIN_LOW_PWM) / (MOTOR_GAIN_HIGH_PWM - MOTOR_GAIN_LOW_PWM));
}

void drive(int left, int right)
{
    s_lastL = left;
    s_lastR = right;
    int l = clampPwm((long)left * gainFor(left, MOTOR_L_GAIN, MOTOR_L_GAIN_LOW, MOTOR_L_GAIN_REV) / 100);    // 좌우 모터 성능 보정 (속도별)
    int r = clampPwm((long)right * gainFor(right, MOTOR_R_GAIN, MOTOR_R_GAIN_LOW, MOTOR_R_GAIN_REV) / 100);
    digitalWrite(PIN_L_DIR, l >= 0 ? LEFT_FWD_LEVEL : !LEFT_FWD_LEVEL);    // 방향
    digitalWrite(PIN_R_DIR, r >= 0 ? RIGHT_FWD_LEVEL : !RIGHT_FWD_LEVEL);
    analogWrite(PIN_L_PWM, abs(l));                                        // 속도
    analogWrite(PIN_R_PWM, abs(r));
}

// 제자리 회전: 직진용 좌우 보정(MOTOR_x_GAIN)을 빼고 양쪽 같은 힘으로 → 회전 중심이 바퀴축 가운데
void spinDrive(int left, int right)
{
#if TURN_USE_MOTOR_GAIN
    drive(left, right);
#else
    s_lastL = left;
    s_lastR = right;
    int l = clampPwm(left), r = clampPwm(right);
    digitalWrite(PIN_L_DIR, l >= 0 ? LEFT_FWD_LEVEL : !LEFT_FWD_LEVEL);
    digitalWrite(PIN_R_DIR, r >= 0 ? RIGHT_FWD_LEVEL : !RIGHT_FWD_LEVEL);
    analogWrite(PIN_L_PWM, abs(l));
    analogWrite(PIN_R_PWM, abs(r));
#endif
}

void Forward(int speed)  { drive(speed, speed); }
void Backward(int speed) { drive(-speed, -speed); }

void Stop()
{
    analogWrite(PIN_L_PWM, 0);
    analogWrite(PIN_R_PWM, 0);
    s_lastL = s_lastR = 0;
}

void Brake()
{
#if BRAKE_PWM > 0
    int bl = (s_lastL > 0) ? -BRAKE_PWM : (s_lastL < 0 ? BRAKE_PWM : 0);
    int br = (s_lastR > 0) ? -BRAKE_PWM : (s_lastR < 0 ? BRAKE_PWM : 0);
    if (bl || br) {
        drive(bl, br);
        delay(BRAKE_MS);
    }
#endif
    Stop();
}
