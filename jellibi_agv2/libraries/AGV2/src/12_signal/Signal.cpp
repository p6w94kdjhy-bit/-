#include "12_signal/Signal.h"
#include "config.h"
#include "pins.h"
#include "01_motor/Motor.h"

static bool s_abortArmed = false;
static unsigned long s_armTime = 0;

void uiInit()
{
    pinMode(PIN_BUTTON, INPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);
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

// 교차점 인식 표시 (비차단): '삑'(SOUND_CROSS) + LED 토글
void beepNode()
{
#if SOUND_ON || SOUND_CROSS
    tone(PIN_BUZZER, CROSS_BEEP_HZ, CROSS_BEEP_MS);    // 비차단: 소리 나는 동안에도 계속 주행
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

// 업로드된 프로그램 안의 config.h 값 (노트북에서 고친 값이 들어갔는지 확인)
void showConfig()
{
    Serial.println(F("----- config.h (로봇 안의 값) -----"));
    Serial.print(F("IR  L 흰/검 = ")); Serial.print(IR_L_WHITE); Serial.print('/'); Serial.print(IR_L_BLACK);
    Serial.print(F("   R 흰/검 = ")); Serial.print(IR_R_WHITE); Serial.print('/'); Serial.println(IR_R_BLACK);
    Serial.print(F("속도 CRUISE/APPROACH = ")); Serial.print(SPEED_CRUISE); Serial.print('/'); Serial.print(SPEED_APPROACH);
    Serial.print(F("   ALIGN_MS = ")); Serial.print(ALIGN_MS);
    Serial.print(F("   MOTOR_GAIN L/R = ")); Serial.print(MOTOR_L_GAIN); Serial.print('/'); Serial.println(MOTOR_R_GAIN);
    Serial.print(F("라인 LT_MODE = ")); Serial.print(LT_MODE); Serial.print(F("  LT_KP = ")); Serial.println(LT_KP);
    Serial.print(F("회전 TURN90_MS = ")); Serial.print(TURN90_MS);
    Serial.print(F("  AFTER 좌/우 = ")); Serial.print(TURN_AFTER_LEFT_DEG); Serial.print('/');
    Serial.print(TURN_AFTER_RIGHT_DEG); Serial.print(F("  MIN = ")); Serial.println(TURN_MIN_DEG);
    Serial.print(F("U턴 BACK_MS = ")); Serial.print(UTURN_BACK_MS); Serial.print(F("  BLIND = "));
    Serial.print(UTURN_BLIND_DEG); Serial.print(F("  AFTER = ")); Serial.print(UTURN_AFTER_DEG);
    Serial.print(F("  EXIT_MS = ")); Serial.println(UTURN_EXIT_MAX_MS);
    Serial.print(F("RFID 출발 START_MODE = ")); Serial.print(START_MODE);
    Serial.print(F("  팔레트확인 = ")); Serial.print(PICK_RFID_CHECK);
    Serial.print(F("  카드배송지 = ")); Serial.println(RFID_DEST_MODE);
    Serial.println(F("-----------------------------------"));
}
