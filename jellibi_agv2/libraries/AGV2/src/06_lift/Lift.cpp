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
//   교본: setup() 에서 "먼저 필요한 초기화 코드 실행 이후" 리프터 내리기(LifterDown) 실행
//         LifterDown = 서보 연결 → 10ms → 내린 각도 쓰기 → 300ms → 연결 해제 → 10ms
//   전원이 들어온 직후에는 서보가 스스로 움찔하며 올라갈 수 있어서, 너무 일찍 내리면 그 뒤에 다시 올라온다.
//   → 다른 초기화가 끝난 뒤에 호출하고, 시작 전에 LIFT_INIT_WAIT_MS 만큼 기다렸다가 내린다.
void liftInit()
{
    delay(LIFT_INIT_WAIT_MS);      // 전원이 안정될 때까지 (서보의 전원 켤 때 움찔거림이 끝난 뒤)
    s_angle = LIFT_DOWN_ANGLE;
    s_servo.write(s_angle);        // (연결하는 순간 가운데 90° 로 튀지 않게 내린 각도를 먼저 넣어 둠)
    s_servo.attach(PIN_SERVO);     // 서보 연결
    delay(10);
    s_servo.write(s_angle);        // 리프터 아래로 내림
    delay(LIFT_INIT_HOLD_MS);
    s_servo.detach();              // 서보 연결 해제
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
