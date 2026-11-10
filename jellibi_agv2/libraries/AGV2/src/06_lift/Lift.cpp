#include "06_lift/Lift.h"
#include <Servo.h>
#include "config.h"
#include "pins.h"

static Servo s_servo;
static int s_angle = LIFT_DOWN_ANGLE;
static bool s_up = false;

static void moveTo(int target)
{
    if (!s_servo.attached()) s_servo.attach(PIN_SERVO);
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
    s_servo.attach(PIN_SERVO);
    s_angle = LIFT_DOWN_ANGLE;
    s_servo.write(s_angle);
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
