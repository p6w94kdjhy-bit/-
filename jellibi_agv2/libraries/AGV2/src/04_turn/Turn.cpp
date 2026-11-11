#include "04_turn/Turn.h"
#include "config.h"
#include "01_motor/Motor.h"
#include "02_sensor/Sensor.h"
#include "03_linetrace/LineTrace.h"
#include "12_signal/Signal.h"

// 진행쪽 IR이 검정인가 (왼회전이면 왼쪽 IR, 우회전이면 오른쪽 IR)
static bool leadIsBlack(bool left)
{
    IrSample s = readIr();
    return left ? isBlackL(s.l) : isBlackR(s.r);
}

void spinTurn(bool left, uint8_t quarters)
{
    unsigned long expect = (unsigned long)TURN90_MS * quarters;   // 예상 회전 시간
    unsigned long blindPct = (quarters >= 2) ? UTURN_BLIND_PCT : TURN_BLIND_PCT;

    // ① 제자리 회전 시작
    if (left) drive(-TURN_PWM, TURN_PWM);
    else      drive(TURN_PWM, -TURN_PWM);

#if TURN_USE_SENSOR
    // ② 무시 구간
    waitMs(expect * blindPct / 100);

    // ③ 진행쪽 IR이 검정을 볼 때까지
    unsigned long seekMax = expect * (100 - blindPct) / 100 + TURN_SEEK_MAX_MS;
    unsigned long t0 = millis();
    bool found = false;
    while (millis() - t0 < seekMax) {
        if (leadIsBlack(left)) { found = true; break; }
        uiCheckAbort();
    }

    // ④ 그 IR이 다시 흰색이 될 때까지 (선이 두 센서 사이로)
    if (found) {
        t0 = millis();
        while (millis() - t0 < TURN_EXIT_MAX_MS && leadIsBlack(left)) uiCheckAbort();
    } else {
        Serial.println(F("[turn] 선을 찾지 못하고 시간으로 정지"));
    }
#else
    waitMs(expect);                     // 센서 없이 시간으로만 회전
#endif

    // ⑤ 정지
    Brake();
    delay(TURN_SETTLE_MS);
    lineTraceReset();
}

void TurnLeft90()  { spinTurn(true, 1); }
void TurnRight90() { spinTurn(false, 1); }
void UTurn()       { spinTurn(UTURN_LEFT, 2); }
