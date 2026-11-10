// RFID(MFRC522) 동작 확인: 카드를 대면 시리얼 모니터(115200)에 UID 출력 + LED/부저
// 필요: 라이브러리 매니저에서 'MFRC522' 설치
#include <SPI.h>
#include <MFRC522.h>

MFRC522 rfid(2, 4);   // SS=D2, RST=D4

void setup()
{
    Serial.begin(115200);
    SPI.begin();
    rfid.PCD_Init();
    Serial.print(F("rfid_test: reader version 0x"));
    Serial.println(rfid.PCD_ReadRegister(MFRC522::VersionReg), HEX);   // 0x91/0x92 정상, 0x00/0xFF 연결 안 됨
    Serial.println(F("카드를 대세요"));
}

void loop()
{
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;
    Serial.print(F("UID:"));
    for (byte i = 0; i < rfid.uid.size; i++) {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(rfid.uid.uidByte[i], HEX);
    }
    Serial.println();
    tone(3, 1047, 100);
    rfid.PICC_HaltA();
    delay(300);
}
