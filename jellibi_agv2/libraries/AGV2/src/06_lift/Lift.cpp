#include "06_lift/Lift.h"
#include <Servo.h>
#include "config.h"
#include "pins.h"
#include "02_sensor/Sensor.h"

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

// 전원을 켜면 리프터가 약간 올라가는 현상 처리 — 교본 26쪽 '전원을 켤 때 리프터가 올라가는 현상의 처리'
//   setup() 맨 처음에 리프터 내리기 실행: 서보 연결 → 내린 각도 → 0.3초 → 연결 해제  (교본 LifterDown 과 같음)
//   (연결하기 전에 내린 각도를 먼저 넣어 둠 → 연결하는 순간 가운데 90° 로 튀지 않음)
void liftInit()
{
    s_angle = LIFT_DOWN_ANGLE;
    s_servo.write(s_angle);
    s_servo.attach(PIN_SERVO);     // 서보 연결
    delay(10);
    s_servo.write(s_angle);        // 리프터 아래로 내림
    delay(300);
    s_servo.detach();              // 서보 연결 해제 (떨림·소음 방지)
    delay(10);
    s_up = false;
}

void LiftUp()
{
    moveTo(LIFT_UP_ANGLE);
    s_up = true;
    irUseCarry(true);              // 적재함 그림자용 바닥 IR 값으로 (config.h _CARRY, 미측정이면 그대로)
#if !LIFT_HOLD_WHILE_CARRY
    s_servo.detach();
#endif
}

void LiftDown()
{
    moveTo(LIFT_DOWN_ANGLE);
    s_up = false;
    irUseCarry(false);             // 평소 바닥 IR 값으로
    s_servo.detach();
}

bool liftIsUp() { return s_up; }

void LiftGoto(int angle)
{
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    moveTo(angle);
}
