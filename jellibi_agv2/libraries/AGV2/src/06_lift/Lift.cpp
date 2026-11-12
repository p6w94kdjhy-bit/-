#include "06_lift/Lift.h"
#include <Servo.h>
#include "config.h"
#include "pins.h"

static Servo s_servo;
static int s_angle = LIFT_DOWN_ANGLE;
static bool s_up = false;

static void moveTo(int target)
{
    if (!s_servo.attached()) {
        s_servo.write(s_angle);        // 켜기 전에 지금 각도를 먼저 넣어 둠 → 켜는 순간 튀지 않음
        s_servo.attach(PIN_SERVO);
    }
    int step = (target > s_angle) ? LIFT_STEP_DEG : -LIFT_STEP_DEG;
    while (abs(target - s_angle) > LIFT_STEP_DEG) {
        s_angle += step;
        s_servo.write(s_angle);
        delay(LIFT_STEP_MS);
    }
    s_angle = target;
    s_servo.write(s_angle);
    delay(LIFT_SETTLE_MS);
}

void liftInit()
{
    // 서보는 켜는(attach) 순간 '마지막으로 넣어 둔 각도'로 움직인다. 아무것도 안 넣으면 가운데(90°)로 가서
    // 리프트가 잠깐 들린다 → 내린 각도를 먼저 넣고 나서 켠다.
    s_angle = LIFT_DOWN_ANGLE;
    s_servo.write(s_angle);
    s_servo.attach(PIN_SERVO);
    delay(300);
    s_servo.detach();          // 서보 소음 방지 (예제와 동일)
    s_up = false;
}

void LiftUp()
{
    moveTo(LIFT_UP_ANGLE);
    s_up = true;
#if !LIFT_HOLD_WHILE_CARRY
    s_servo.detach();
#endif
}

void LiftDown()
{
    moveTo(LIFT_DOWN_ANGLE);
    s_up = false;
    s_servo.detach();
}

bool liftIsUp() { return s_up; }
