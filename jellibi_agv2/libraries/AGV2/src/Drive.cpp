#include "Drive.h"
#include "pins.h"
#include "Ui.h"

// 교차점 판정 레벨: 흰 + (검-흰) * XING_LEVEL_PCT%  (50% = IR_x_TH)
#define XING_TH_L (IR_L_WHITE + (long)(IR_L_BLACK - IR_L_WHITE) * XING_LEVEL_PCT / 100)
#define XING_TH_R (IR_R_WHITE + (long)(IR_R_BLACK - IR_R_WHITE) * XING_LEVEL_PCT / 100)

static int s_lastL = 0, s_lastR = 0;   // Brake()용 마지막 명령
static int s_prevErr = 0;

// ===================================================================== 모터
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

void drive(int left, int right)
{
    s_lastL = left;
    s_lastR = right;
    int l = clampPwm((long)left * MOTOR_L_GAIN / 100);
    int r = clampPwm((long)right * MOTOR_R_GAIN / 100);
    digitalWrite(PIN_L_DIR, l >= 0 ? LEFT_FWD_LEVEL : !LEFT_FWD_LEVEL);
    digitalWrite(PIN_R_DIR, r >= 0 ? RIGHT_FWD_LEVEL : !RIGHT_FWD_LEVEL);
    analogWrite(PIN_L_PWM, abs(l));
    analogWrite(PIN_R_PWM, abs(r));
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

// ===================================================================== 센서
IrSample readIr()
{
    IrSample s;
    s.l = analogRead(PIN_IR_L);
    s.r = analogRead(PIN_IR_R);
    return s;
}

int irFront() { return analogRead(PIN_IR_FRONT); }

bool irCalibrated()
{
    return IR_L_WHITE >= 0 && IR_L_BLACK > IR_L_WHITE &&
           IR_R_WHITE >= 0 && IR_R_BLACK > IR_R_WHITE;
}

bool frontCalibrated() { return FRONT_OBSTACLE_TH > 0; }

bool isBlackL(int v) { return v > IR_L_TH; }
bool isBlackR(int v) { return v > IR_R_TH; }

static int norm(int v, int w, int b)
{
    if (b <= w) return 0;
    long n = (long)(v - w) * 1000 / (b - w);
    if (n < 0) n = 0;
    if (n > 1000) n = 1000;
    return (int)n;
}
int irNormL(int v) { return norm(v, IR_L_WHITE, IR_L_BLACK); }
int irNormR(int v) { return norm(v, IR_R_WHITE, IR_R_BLACK); }

int lineError(const IrSample& s)
{
    return irNormL(s.l) - irNormR(s.r) - LT_CENTER_OFFSET;
}

// ===================================================================== 라인트레이스
void lineTraceReset() { s_prevErr = 0; }

void lineTraceStep(const IrSample& s, int speed)
{
    int err = lineError(s);
    long corr = (long)err * LT_KP / 100 + (long)(err - s_prevErr) * LT_KD / 100;
    s_prevErr = err;
    // err>0 : 선이 왼쪽 → 왼바퀴 감속, 오른바퀴 가속
    drive(clampPwm(speed - corr), clampPwm(speed + corr));
}

// ===================================================================== 교차점
void XingDetector::reset()
{
    _cnt = 0;
    _armed = true;
    _last = millis() - XING_SKIP_MS;
}

bool XingDetector::update(const IrSample& s)
{
    bool both = (s.l > XING_TH_L) && (s.r > XING_TH_R);
    if (!both) {
        _cnt = 0;
        _armed = true;        // 선을 벗어나야 다시 셀 수 있음
        return false;
    }
    if (_cnt < 255) _cnt++;
    if (_armed && _cnt >= XING_CONFIRM_SAMPLES && millis() - _last >= XING_SKIP_MS) {
        _armed = false;
        _last = millis();
        return true;
    }
    return false;
}

// ===================================================================== 장애물
bool ObstacleDetector::update()
{
    if (FRONT_OBSTACLE_TH <= 0) return false;
    if (irFront() > FRONT_OBSTACLE_TH) {
        if (_cnt < 255) _cnt++;
    } else {
        _cnt = 0;
    }
    return _cnt >= FRONT_CONFIRM_SAMPLES;
}

// ===================================================================== 회전
// 제자리 피벗. 예상시간의 TURN_BLIND_PCT% 까지는 센서를 무시하고 돌고,
// 그 뒤 진행 방향 쪽 센서(왼회전=왼쪽)가 새 선을 만나면 멈춘다.
// → 교차점에 어떤 선이 있든(막다른 끝, 중간 U턴) 같은 방식으로 동작.
void pivotTurn(bool left, uint8_t quarters)
{
    int s = left ? -1 : 1;
    drive(s * TURN_PWM, -s * TURN_PWM);
    unsigned long total = (unsigned long)TURN90_MS * quarters;
#if TURN_USE_SENSOR
    waitMs(total * TURN_BLIND_PCT / 100);
    unsigned long seekMax = total * (100 - TURN_BLIND_PCT) / 100 + TURN_SEEK_MAX_MS;
    unsigned long t0 = millis();
    bool found = false;
    while (millis() - t0 < seekMax) {
        IrSample ir = readIr();
        if (left ? isBlackL(ir.l) : isBlackR(ir.r)) { found = true; break; }
        uiCheckAbort();
    }
    if (found) delay(TURN_CENTER_MS);
    else Serial.println(F("[turn] line not found"));
#else
    waitMs(total);
#endif
    Brake();
    delay(TURN_SETTLE_MS);
    lineTraceReset();
}

void TurnLeft90()  { pivotTurn(true, 1); }
void TurnRight90() { pivotTurn(false, 1); }
void UTurn()       { pivotTurn(UTURN_LEFT, 2); }
