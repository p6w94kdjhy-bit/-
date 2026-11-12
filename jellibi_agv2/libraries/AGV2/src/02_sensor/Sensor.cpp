#include "02_sensor/Sensor.h"
#include "pins.h"

IrSample readIr()
{
    IrSample s;
    s.l = analogRead(PIN_IR_L);
    s.r = analogRead(PIN_IR_R);
    return s;
}

int irFront() { return analogRead(PIN_IR_FRONT); }

// 지금 쓰는 측정값 (평소 / 적재함을 든 상태)
static int s_lw = IR_L_WHITE, s_lb = IR_L_BLACK, s_rw = IR_R_WHITE, s_rb = IR_R_BLACK;
static int s_offset = LT_CENTER_OFFSET;
static int s_thL = IR_L_TH, s_thR = IR_R_TH, s_xL, s_xR;
static bool s_xInit = false;

static void recalc()
{
    s_thL = (s_lw + s_lb) / 2;
    s_thR = (s_rw + s_rb) / 2;
    s_xL = s_lw + (int)((long)(s_lb - s_lw) * XING_LEVEL_PCT / 100);
    s_xR = s_rw + (int)((long)(s_rb - s_rw) * XING_LEVEL_PCT / 100);
    s_xInit = true;
}

bool irCarryCalibrated()
{
    return IR_L_WHITE_CARRY >= 0 && IR_L_BLACK_CARRY > IR_L_WHITE_CARRY &&
           IR_R_WHITE_CARRY >= 0 && IR_R_BLACK_CARRY > IR_R_WHITE_CARRY;
}

void irUseCarry(bool carrying)
{
    if (carrying && irCarryCalibrated()) {
        s_lw = IR_L_WHITE_CARRY; s_lb = IR_L_BLACK_CARRY; s_rw = IR_R_WHITE_CARRY; s_rb = IR_R_BLACK_CARRY;
        s_offset = LT_CENTER_OFFSET_CARRY;
    } else {
        s_lw = IR_L_WHITE; s_lb = IR_L_BLACK; s_rw = IR_R_WHITE; s_rb = IR_R_BLACK;
        s_offset = LT_CENTER_OFFSET;
    }
    recalc();
}

int irXingThL() { if (!s_xInit) recalc(); return s_xL; }
int irXingThR() { if (!s_xInit) recalc(); return s_xR; }

// 반사광이 많을수록(흰색) 값이 작음 → 기준값보다 크면 검정
bool isBlackL(int v) { return v > s_thL; }
bool isBlackR(int v) { return v > s_thR; }

bool irCalibrated()
{
    return IR_L_WHITE >= 0 && IR_L_BLACK > IR_L_WHITE &&
           IR_R_WHITE >= 0 && IR_R_BLACK > IR_R_WHITE;
}


static int norm(int v, int w, int b)
{
    if (b <= w) return 0;
    long n = (long)(v - w) * 1000 / (b - w);
    if (n < 0) n = 0;
    if (n > 1000) n = 1000;
    return (int)n;
}
int irNormL(int v) { return norm(v, s_lw, s_lb); }
int irNormR(int v) { return norm(v, s_rw, s_rb); }

int irCenterOffset() { return s_offset; }

int lineError(const IrSample& s)
{
    return irNormL(s.l) - irNormR(s.r) - s_offset;
}
