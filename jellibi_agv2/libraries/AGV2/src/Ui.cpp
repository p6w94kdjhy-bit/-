#include "Ui.h"
#include "config.h"
#include "pins.h"
#include "Drive.h"

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

void uiWaitStart()
{
    while (!buttonDown()) {}
    beep(1047, 60);
    while (buttonDown()) {}
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
