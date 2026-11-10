#include "11_start/Start.h"
#include "config.h"
#include "pins.h"
#include "12_signal/Signal.h"

#if START_MODE != 0
#include <SPI.h>
#include <MFRC522.h>
static MFRC522 s_rfid(PIN_RFID_SS, PIN_RFID_RST);

// 새 카드가 태그되면 UID를 출력하고 true
static bool rfidTagged()
{
    if (!s_rfid.PICC_IsNewCardPresent()) return false;
    if (!s_rfid.PICC_ReadCardSerial()) return false;
    Serial.print(F("[RFID] UID"));
    for (byte i = 0; i < s_rfid.uid.size; i++) {
        Serial.print(s_rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(s_rfid.uid.uidByte[i], HEX);
    }
    Serial.println();
    s_rfid.PICC_HaltA();
    return true;
}
#endif

bool buttonDown()
{
    if (digitalRead(PIN_BUTTON) != LOW) return false;
    delay(5);
    return digitalRead(PIN_BUTTON) == LOW;
}

void uiWaitStart()
{
#if START_MODE != 0
    SPI.begin();
    s_rfid.PCD_Init();
    Serial.println(F("[start] waiting for RFID tag"));
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
        if (rfidTagged()) {                 // 태그: 카드를 대는 순간 출발
            beep(1047, 120);
            break;
        }
#endif
    }
#if START_MODE != 0
    SPI.end();                              // D13(SCK)을 상태 LED용으로 되돌림
#endif
    delay(START_DELAY_MS);
}
