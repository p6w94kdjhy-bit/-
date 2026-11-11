#include "13_rfid/Rfid.h"
#include "config.h"
#include "pins.h"
#include "12_signal/Signal.h"

#if RFID_USED
#include <SPI.h>
#include <MFRC522.h>

static MFRC522 s_reader(PIN_RFID_SS, PIN_RFID_RST);
static bool s_on = false;
static bool s_reported = false;
static unsigned long s_lastInit = 0;

// 리더 초기화: 안테나 감도 최대 (인식 거리 늘리기)
static void initReader()
{
    s_reader.PCD_Init();
    delay(5);
    s_reader.PCD_SetAntennaGain(MFRC522::RxGain_max);
    s_reader.PCD_AntennaOn();
    s_lastInit = millis();
}

void rfidOn()
{
    if (s_on) return;
    SPI.begin();
    initReader();
    s_on = true;
    if (!s_reported) {                       // 처음 한 번 리더 상태 출력
        s_reported = true;
        byte v = s_reader.PCD_ReadRegister(MFRC522::VersionReg);
        Serial.print(F("[rfid] reader version 0x")); Serial.print(v, HEX);
        if (v == 0x00 || v == 0xFF) Serial.println(F("  → 리더 연결 안 됨 (전원 ON / RFID 케이블 확인)"));
        else                        Serial.println(F("  → 정상"));
    }
}

void rfidOff()
{
    if (!s_on) return;
    s_reader.PCD_AntennaOff();
    SPI.end();
    pinMode(PIN_LED, OUTPUT);
    s_on = false;
}

uint8_t rfidVersion()
{
    rfidOn();
    return s_reader.PCD_ReadRegister(MFRC522::VersionReg);
}

bool rfidRead(RfidUid& uid)
{
    rfidOn();
    // WakeupA: 이미 읽고 쉬는(HALT) 카드도 다시 깨워서 읽는다 → 같은 카드를 여러 번 확인 가능
    byte atqa[2];
    byte size = sizeof(atqa);
    bool present = (s_reader.PICC_WakeupA(atqa, &size) == MFRC522::STATUS_OK)
                || s_reader.PICC_IsNewCardPresent();
    if (!present || !s_reader.PICC_ReadCardSerial()) {
        // 리더가 가끔 멈추는 경우 대비: 2초 동안 못 읽으면 다시 초기화
        if (millis() - s_lastInit > 2000) initReader();
        return false;
    }
    uid.len = s_reader.uid.size > RFID_UID_MAX ? RFID_UID_MAX : s_reader.uid.size;
    for (uint8_t i = 0; i < uid.len; i++) uid.b[i] = s_reader.uid.uidByte[i];
    s_reader.PICC_HaltA();
    return true;
}

#else   // RFID 기능을 하나도 안 쓰면 빈 함수 (MFRC522 라이브러리 불필요)

void rfidOn() {}
void rfidOff() {}
uint8_t rfidVersion() { return 0; }
bool rfidRead(RfidUid& uid) { uid.len = 0; return false; }

#endif

bool rfidWait(RfidUid& uid, unsigned long ms)
{
    unsigned long t0 = millis();
    do {
        if (rfidRead(uid)) return true;
        uiCheckAbort();
    } while (millis() - t0 < ms);
    return false;
}

bool rfidSame(const RfidUid& a, const RfidUid& b)
{
    if (a.len == 0 || a.len != b.len) return false;
    for (uint8_t i = 0; i < a.len; i++) if (a.b[i] != b.b[i]) return false;
    return true;
}

void rfidPrint(const RfidUid& uid)
{
    if (uid.len == 0) { Serial.print(F("(없음)")); return; }
    for (uint8_t i = 0; i < uid.len; i++) {
        if (i) Serial.print(' ');
        if (uid.b[i] < 0x10) Serial.print('0');
        Serial.print(uid.b[i], HEX);
    }
}
