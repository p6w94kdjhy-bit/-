#include "Ui.h"
#include "config.h"
#include "pins.h"
#include "Drive.h"

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

static bool s_abortArmed = false;
static unsigned long s_armTime = 0;

void uiInit()
{
    pinMode(PIN_BUTTON, INPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);
}

bool buttonDown()
{
    if (digitalRead(PIN_BUTTON) != LOW) return false;
    delay(5);
    return digitalRead(PIN_BUTTON) == LOW;
}

// 소리(SOUND_ON=1)와 함께 LED도 같은 시간 동안 켠다 → 무음이어도 같은 패턴이 LED로 보임
void beep(unsigned int freq, unsigned int ms)
{
    digitalWrite(PIN_LED, HIGH);
#if SOUND_ON
    tone(PIN_BUZZER, freq, ms);
#else
    (void)freq;
#endif
    delay(ms);
#if SOUND_ON
    noTone(PIN_BUZZER);
#endif
    digitalWrite(PIN_LED, LOW);
}

void beepReady() { beep(523, 120); beep(659, 120); beep(784, 160); }

// 교차점 카운트 표시 (비차단): 소리 '틱' + LED 토글
void beepNode()
{
#if SOUND_ON
    tone(PIN_BUZZER, 1568, 15);
#endif
    digitalWrite(PIN_LED, !digitalRead(PIN_LED));
}
void beepDone()  { beep(784, 150); beep(1047, 300); }

void uiFatal(const __FlashStringHelper* msg)
{
    Stop();
    Serial.print(F("[FATAL] "));
    Serial.println(msg);
    for (;;) {                       // LED: 0.3초 켜짐 / 0.7초 꺼짐 반복
        beep(220, 300);
        delay(700);
        Serial.print(F("[FATAL] "));
        Serial.println(msg);         // 나중에 시리얼 모니터를 열어도 이유가 보이게 반복 출력
    }
}

// 출발 신호 대기 (START_MODE): 버튼은 눌렀다 뗄 때, 태그는 카드를 대는 순간
void uiWaitStart()
{
#if START_MODE != 0
    SPI.begin();
    s_rfid.PCD_Init();
    Serial.println(F("[start] waiting for RFID tag"));
#endif
    for (;;) {
#if START_MODE != 1
        if (buttonDown()) {
            beep(1047, 60);
            while (buttonDown()) {}
            break;
        }
#endif
#if START_MODE != 0
        if (rfidTagged()) {
            beep(1047, 120);
            break;
        }
#endif
    }
#if START_MODE != 0
    SPI.end();                  // D13(SCK)을 상태 LED용으로 되돌림
#endif
    delay(START_DELAY_MS);
}

void uiArmAbort()
{
    s_abortArmed = true;
    s_armTime = millis();
}

void uiCheckAbort()
{
    // 출발 직후 500ms는 무시 (출발 버튼 채터링)
    if (!s_abortArmed || millis() - s_armTime < 500) return;
    if (digitalRead(PIN_BUTTON) == LOW) {
        Stop();
        Serial.println(F("[ABORT] button"));
        for (;;) {                   // LED: 0.1초 켜짐 / 0.9초 꺼짐 반복
            beep(440, 100);
            delay(900);
        }
    }
}

void waitMs(unsigned long ms)
{
    unsigned long t0 = millis();
    while (millis() - t0 < ms) uiCheckAbort();
}
