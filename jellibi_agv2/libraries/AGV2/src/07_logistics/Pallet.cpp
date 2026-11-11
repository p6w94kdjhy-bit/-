#include "07_logistics/Pallet.h"
#include "config.h"
#include "01_motor/Motor.h"
#include "04_turn/Turn.h"
#include "06_lift/Lift.h"
#include "12_signal/Signal.h"
#include "14_runlog/RunLog.h"

static bool s_carrying = false;
static RfidUid s_uid;

const RfidUid& palletUid() { return s_uid; }

void palletInit() { s_carrying = false; s_uid.len = 0; }
bool palletCarrying() { return s_carrying; }

// 정렬 후 앞(+)/뒤(-)로 조금 움직여 리프트 위치를 팔레트 중심에 맞춤
static void shortMove(int ms)
{
    if (ms > 0)      { Forward(SPEED_APPROACH);  waitMs(ms);  Brake(); }
    else if (ms < 0) { Backward(SPEED_APPROACH); waitMs(-ms); Brake(); }
}

// (RFID) 팔레트 밑에서 카드 확인. 모드 2 면 못 읽을 때 앞뒤로 천천히 움직이며 찾는다
static void checkPalletCard()
{
    s_uid.len = 0;
#if PICK_RFID_CHECK
    bool found = rfidWait(s_uid, PICK_RFID_WAIT_MS);
#if PICK_RFID_CHECK == 2
    if (!found) {                                     // 앞으로 찾기
        Forward(PICK_RFID_SEARCH_PWM);
        found = rfidWait(s_uid, PICK_RFID_SEARCH_MS);
        Brake();
        if (!found) {                                 // 뒤로 (원래 자리 지나서) 찾기
            Backward(PICK_RFID_SEARCH_PWM);
            found = rfidWait(s_uid, 2UL * PICK_RFID_SEARCH_MS);
            Brake();
            if (!found) {                             // 못 찾으면 원래 자리로
                Forward(PICK_RFID_SEARCH_PWM);
                waitMs(PICK_RFID_SEARCH_MS);
                Brake();
            }
        }
    }
#endif
    rfidOff();
    Serial.print(F("[pallet] RFID "));
    if (found) rfidPrint(s_uid); else Serial.print(F("카드 못 읽음"));
    Serial.println();
#endif
}

void PickPallet(Nav& nav)
{
    runlogAdd(RL_PICK, nav.node);
    shortMove(PICK_ADJ_MS);
    checkPalletCard();
    LiftUp();
    s_carrying = true;
    nav.cruise = min(SPEED_CRUISE, SPEED_CARRY);       // 운반 중엔 천천히 (낙하 방지)
    Serial.println(F("[pallet] PICK"));
}

void DropPallet(Nav& nav)
{
    runlogAdd(RL_DROP, nav.node);
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
    UTurnOnEdge();                                 // 후진해서 칸 중간이므로 후진 없는 U턴
    nav.setApproach(behind, back);
    Serial.println(F("[pallet] DROP"));
}
