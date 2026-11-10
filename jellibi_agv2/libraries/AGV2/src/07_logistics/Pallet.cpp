#include "07_logistics/Pallet.h"
#include "config.h"
#include "01_motor/Motor.h"
#include "04_turn/Turn.h"
#include "06_lift/Lift.h"
#include "12_signal/Signal.h"

static bool s_carrying = false;

void palletInit() { s_carrying = false; }
bool palletCarrying() { return s_carrying; }

// 정렬 후 앞(+)/뒤(-)로 조금 움직여 리프트 위치를 팔레트 중심에 맞춤
static void shortMove(int ms)
{
    if (ms > 0)      { Forward(SPEED_APPROACH);  waitMs(ms);  Brake(); }
    else if (ms < 0) { Backward(SPEED_APPROACH); waitMs(-ms); Brake(); }
}

void PickPallet(Nav& nav)
{
    shortMove(PICK_ADJ_MS);
    LiftUp();
    s_carrying = true;
    nav.cruise = min(SPEED_CRUISE, SPEED_CARRY);       // 운반 중엔 천천히 (낙하 방지)
    Serial.println(F("[pallet] PICK"));
}

void DropPallet(Nav& nav)
{
    shortMove(DROP_ADJ_MS);
    LiftDown();
    s_carrying = false;
    nav.cruise = SPEED_CRUISE;

    // 팔레트 밑에서 후진으로 빠져나옴 (배송 완료 조건: 로봇이 팔레트에서 완전히 분리)
    Backward(EXIT_BACK_PWM);
    waitMs(EXIT_BACK_MS);
    Brake();

    // U턴 → 왔던 교차점(뒤쪽)을 향해 출발 준비
    uint8_t back = reverseH(nav.hdg);
    uint8_t behind = stepNode(nav.node, back);
    UTurn();
    nav.setApproach(behind, back);
    Serial.println(F("[pallet] DROP"));
}
