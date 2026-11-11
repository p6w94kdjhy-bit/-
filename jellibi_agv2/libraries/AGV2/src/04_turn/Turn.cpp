#include "04_turn/Turn.h"
#include "config.h"
#include "01_motor/Motor.h"
#include "02_sensor/Sensor.h"
#include "03_linetrace/LineTrace.h"
#include "12_signal/Signal.h"

// 회전을 멈출 때 볼 센서 (config.h TURN_STOP_SENSOR)
//   1 = 반대쪽: 우회전이면 왼쪽 IR, 좌회전이면 오른쪽 IR  (11-11 실측으로 선택)
//   0 = 진행쪽: 우회전이면 오른쪽 IR, 좌회전이면 왼쪽 IR
static bool watchLeftSensor(bool turnLeft)
{
#if TURN_STOP_SENSOR == 1
    return !turnLeft;
#else
    return turnLeft;
#endif
}

static bool sensorBlack(bool leftSensor)
{
    IrSample s = readIr();
    return leftSensor ? isBlackL(s.l) : isBlackR(s.r);
}

void spinTurn(bool left, uint8_t quarters)
{
    unsigned long expect = (unsigned long)TURN90_MS * quarters;   // 예상 회전 시간
    unsigned long blindPct = (quarters >= 2) ? UTURN_BLIND_PCT : TURN_BLIND_PCT;
    bool watchL = watchLeftSensor(left);                           // 볼 센서

    // ① 제자리 회전 시작
    if (left) drive(-TURN_PWM, TURN_PWM);
    else      drive(TURN_PWM, -TURN_PWM);
    unsigned long tStart = millis();

#if TURN_USE_SENSOR
    // ② 무시 구간
    waitMs(expect * blindPct / 100);

    unsigned long seekMax = expect * (100 - blindPct) / 100 + TURN_SEEK_MAX_MS;
    unsigned long t0 = millis();
    bool found = false;

    // ③-1 볼 센서가 흰 바닥으로 나올 때까지 (출발할 때 이미 선 위에 있던 경우를 건너뜀)
    while (millis() - t0 < seekMax && sensorBlack(watchL)) uiCheckAbort();

    // ③-2 볼 센서가 새 검은 선에 닿을 때까지 → 닿으면 정지
    while (millis() - t0 < seekMax) {
        if (sensorBlack(watchL)) { found = true; break; }
        uiCheckAbort();
    }

    // ④ 선에 닿은 뒤 조금 더 돌기 (config.h: 좌 TURN_AFTER_LEFT_MS / 우 TURN_AFTER_RIGHT_MS / U턴 UTURN_AFTER_MS)
    if (found) {
        unsigned long afterMs = (quarters >= 2) ? UTURN_AFTER_MS
                              : (left ? TURN_AFTER_LEFT_MS : TURN_AFTER_RIGHT_MS);
        waitMs(afterMs);
        unsigned long t1 = millis();
        while (millis() - t1 < TURN_EXIT_MAX_MS && sensorBlack(watchL)) uiCheckAbort();
    } else {
        Serial.println(F("[turn] 선을 찾지 못하고 시간으로 정지"));
    }

    // ④-2 최소 회전 보장: 센서가 일찍 선을 봤어도 예상시간의 TURN_MIN_PCT% 까지는 돈다 (덜 도는 것 방지)
    unsigned long minMs = expect * TURN_MIN_PCT / 100;
    while (millis() - tStart < minMs) uiCheckAbort();
#else
    (void)watchL; (void)tStart;
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
