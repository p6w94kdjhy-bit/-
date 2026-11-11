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
    bool uturn = (quarters >= 2);
    bool watchL = watchLeftSensor(left);                           // 볼 센서

    // config.h 의 각도(°) 값을 시간(ms)으로 환산 (TURN90_MS = 90° 도는 시간)
    unsigned long expectMs = TURN_DEG_TO_MS(90UL * quarters);                      // 목표 각도
    unsigned long blindMs  = TURN_DEG_TO_MS(uturn ? UTURN_BLIND_DEG : TURN_BLIND_DEG);
    unsigned long minMs    = TURN_DEG_TO_MS(uturn ? UTURN_MIN_DEG : TURN_MIN_DEG);
    unsigned long afterMs  = TURN_DEG_TO_MS(uturn ? UTURN_AFTER_DEG
                                            : (left ? TURN_AFTER_LEFT_DEG : TURN_AFTER_RIGHT_DEG));

    // ① 제자리 회전 시작
    if (left) drive(-TURN_PWM, TURN_PWM);
    else      drive(TURN_PWM, -TURN_PWM);
    unsigned long tStart = millis();

#if TURN_USE_SENSOR
    // ② 무시 구간 (TURN_BLIND_DEG / UTURN_BLIND_DEG)
    waitMs(blindMs);

    unsigned long seekMax = (expectMs > blindMs ? expectMs - blindMs : 0) + TURN_SEEK_MAX_MS;
    unsigned long t0 = millis();
    bool found = false;

    // ③-1 볼 센서가 흰 바닥으로 나올 때까지 (출발할 때 이미 선 위에 있던 경우를 건너뜀)
    while (millis() - t0 < seekMax && sensorBlack(watchL)) uiCheckAbort();

    // ③-2 볼 센서가 새 검은 선에 닿을 때까지
    while (millis() - t0 < seekMax) {
        if (sensorBlack(watchL)) { found = true; break; }
        uiCheckAbort();
    }

    // ④ 선에 닿은 뒤 조금 더 돌기 (TURN_AFTER_LEFT_DEG / TURN_AFTER_RIGHT_DEG / UTURN_AFTER_DEG)
    if (found) {
        waitMs(afterMs);
        unsigned long t1 = millis();
        while (millis() - t1 < TURN_EXIT_MAX_MS && sensorBlack(watchL)) uiCheckAbort();
    } else {
        Serial.println(F("[turn] 선을 찾지 못하고 시간으로 정지"));
    }

    // ④-2 최소 회전 보장 (TURN_MIN_DEG / UTURN_MIN_DEG): 센서가 일찍 선을 봤어도 이 각도까지는 돈다
    while (millis() - tStart < minMs) uiCheckAbort();
#else
    (void)watchL; (void)tStart; (void)blindMs; (void)minMs; (void)afterMs;
    waitMs(expectMs);                   // 센서 없이 시간으로만 회전
#endif

    // ⑤ 정지
    Brake();
    delay(TURN_SETTLE_MS);
    lineTraceReset();
}

void TurnLeft90()  { spinTurn(true, 1); }
void TurnRight90() { spinTurn(false, 1); }
void UTurn()       { spinTurn(UTURN_LEFT, 2); }
