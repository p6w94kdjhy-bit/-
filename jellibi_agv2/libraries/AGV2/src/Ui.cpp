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
}

bool buttonDown()
{
    if (digitalRead(PIN_BUTTON) != LOW) return false;
    delay(5);
    return digitalRead(PIN_BUTTON) == LOW;
}

void beep(unsigned int freq, unsigned int ms)
{
    tone(PIN_BUZZER, freq, ms);
    delay(ms);
    noTone(PIN_BUZZER);
}

void beepReady() { beep(523, 120); beep(659, 120); beep(784, 160); }
void beepNode()  { tone(PIN_BUZZER, 1568, 15); }          // 비차단
void beepDone()  { beep(784, 150); beep(1047, 300); }

void uiFatal(const __FlashStringHelper* msg)
{
    Stop();
    Serial.print(F("[FATAL] "));
    Serial.println(msg);
    for (;;) {
        beep(220, 300);
        delay(700);
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
        for (;;) {
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
