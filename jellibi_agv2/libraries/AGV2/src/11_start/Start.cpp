#include "11_start/Start.h"
#include "config.h"
#include "pins.h"
#include "12_signal/Signal.h"

#include "13_rfid/Rfid.h"

bool buttonDown()
{
    if (digitalRead(PIN_BUTTON) != LOW) return false;
    delay(5);
    return digitalRead(PIN_BUTTON) == LOW;
}

void uiWaitStart()
{
#if START_MODE != 0
    rfidOn();
    Serial.println(F("[start] waiting for RFID tag"));
    RfidUid tag;
#endif
    for (;;) {
#if START_MODE != 1
        if (buttonDown()) {                 // 버튼: 눌렀다 뗄 때 출발
            beep(1047, 60);
            while (buttonDown()) {}
            break;
        }
#endif
#if START_MODE != 0
        if (rfidRead(tag)) {                // 태그: 카드를 대는 순간 출발
            Serial.print(F("[start] RFID ")); rfidPrint(tag); Serial.println();
            beep(1047, 120);
            break;
        }
#endif
    }
#if START_MODE != 0
    rfidOff();                              // D13(SCK)을 상태 LED용으로 되돌림
#endif
    delay(START_DELAY_MS);
}
