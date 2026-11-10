#include "Sensor.h"
#include "pins.h"

IrSample readIr()
{
    IrSample s;
    s.l = analogRead(PIN_IR_L);
    s.r = analogRead(PIN_IR_R);
    return s;
}

int irFront() { return analogRead(PIN_IR_FRONT); }

// 반사광이 많을수록(흰색) 값이 작음 → 기준값보다 크면 검정
bool isBlackL(int v) { return v > IR_L_TH; }
bool isBlackR(int v) { return v > IR_R_TH; }

bool irCalibrated()
{
    return IR_L_WHITE >= 0 && IR_L_BLACK > IR_L_WHITE &&
           IR_R_WHITE >= 0 && IR_R_BLACK > IR_R_WHITE;
}

bool frontCalibrated() { return FRONT_OBSTACLE_TH > 0; }

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
