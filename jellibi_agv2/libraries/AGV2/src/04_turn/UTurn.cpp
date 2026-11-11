// ===================================================================
//  [04 회전] U턴 (180°) — 강의안 6차시 '180도 뒤돌아 가기' 방식      설정: config.h [6-U]
//
//   ① 교차점 위에서 조금 후진 (UTURN_BACK_MS) → 바닥 IR이 교차점 가로선 뒤로 빠짐
//   ② 제자리 회전 시작 (UTURN_LEFT 방향)
//   ③ 무시 구간 (UTURN_BLIND_DEG)
//   ④ 진행쪽 IR(왼쪽 U턴 → 왼쪽 IR)이 돌아갈 선(검정)을 볼 때까지 회전
//   ⑤ 그 IR이 다시 흰색이 될 때까지 회전 → 선이 두 센서 사이에 옴 (UTURN_EXIT_MAX_MS)
//   ⑥ 정지, UTURN_AFTER_DEG 만큼 더 돌기(+) 또는 되돌리기(-)
//   선을 못 찾으면 180° + UTURN_SEEK_EXTRA_DEG 에서 정지 (무한 회전 없음)
// ===================================================================
#include "04_turn/Turn.h"
#include "config.h"
#include "01_motor/Motor.h"
#include "02_sensor/Sensor.h"
#include "03_linetrace/LineTrace.h"
#include "12_signal/Signal.h"

static bool leadBlack(bool left)            // 진행쪽 IR이 검정인가
{
    IrSample s = readIr();
    return left ? isBlackL(s.l) : isBlackR(s.r);
}

static void spin(bool left, int pwm)
{
    if (left) drive(-pwm, pwm);
    else      drive(pwm, -pwm);
}

static void uturnCore(bool backFirst)
{
    const bool left = UTURN_LEFT;

    // ① 후진: 교차점 가로선에서 센서를 빼낸다
    if (backFirst && UTURN_BACK_MS > 0) {
        Backward(UTURN_BACK_PWM);
        waitMs(UTURN_BACK_MS);
        Brake();
        delay(TURN_SETTLE_MS);
    }

    // ② 제자리 회전 시작
    spin(left, TURN_PWM);
    unsigned long t0 = millis();
    unsigned long maxMs = TURN_DEG_TO_MS(180 + UTURN_SEEK_EXTRA_DEG);   // 안전장치

    // ③ 무시 구간
    waitMs(TURN_DEG_TO_MS(UTURN_BLIND_DEG));

    // ④ 진행쪽 IR이 돌아갈 선을 볼 때까지 (먼저 흰 바닥으로 나온 뒤)
    while (millis() - t0 < maxMs && leadBlack(left)) uiCheckAbort();
    bool found = false;
    while (millis() - t0 < maxMs) {
        if (leadBlack(left)) { found = true; break; }
        uiCheckAbort();
    }

    // ⑤ 그 IR이 다시 흰색이 될 때까지 → 선이 두 센서 사이로
    if (found) {
        unsigned long t1 = millis();
        while (millis() - t1 < UTURN_EXIT_MAX_MS && leadBlack(left)) uiCheckAbort();
        // ⑥-a 더 돌기 (+)
        if (UTURN_AFTER_DEG > 0) waitMs(TURN_DEG_TO_MS(UTURN_AFTER_DEG));
    } else {
        Serial.println(F("[uturn] 선을 찾지 못하고 정지"));
    }

    // ⑥ 정지
    Brake();
    delay(TURN_SETTLE_MS);

    // ⑥-b 되돌리기 (-)
    if (found && UTURN_AFTER_DEG < 0) {
        spin(!left, TURN_PWM);
        waitMs(TURN_DEG_TO_MS(-(long)UTURN_AFTER_DEG));
        Brake();
        delay(TURN_SETTLE_MS);
    }
    lineTraceReset();
}

void UTurn()       { uturnCore(true); }     // 교차점 위
void UTurnOnEdge() { uturnCore(false); }    // 칸 중간 (가로선 없음)
