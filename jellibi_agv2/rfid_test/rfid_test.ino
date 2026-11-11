// =====================================================================
//  RFID(MFRC522) 확인 · 카드 번호 읽기     (시리얼 모니터 115200)
//   - 처음에 리더 칩 버전 출력: 0x91 / 0x92 = 정상, 0x00 / 0xFF = 연결 안 됨
//   - 카드를 대면 번호(UID)와, course2_pallet.ino 의 RFID_DEST[] 에 붙여넣을 줄을 출력
//   - 카드를 대고 있는 동안 0.5초마다 다시 읽음 → 팔레트 밑에서 얼마나 떨어져도 읽히는지 확인 가능
//   필요: 라이브러리 매니저에서 'MFRC522' (by GithubCommunity) 설치
// =====================================================================
#include <SPI.h>
#include <MFRC522.h>

MFRC522 rfid(2, 4);   // SS=D2, RST=D4

void setup()
{
    Serial.begin(115200);
    SPI.begin();
    rfid.PCD_Init();
    byte v = rfid.PCD_ReadRegister(MFRC522::VersionReg);
    Serial.print(F("rfid_test: reader version 0x"));
    Serial.print(v, HEX);
    Serial.println((v == 0x00 || v == 0xFF) ? F("  → 연결 안 됨 (배선/전원 확인)") : F("  → 정상"));
    Serial.println(F("카드를 대세요"));
}

void loop()
{
    byte atqa[2];
    byte size = sizeof(atqa);
    bool present = (rfid.PICC_WakeupA(atqa, &size) == MFRC522::STATUS_OK) || rfid.PICC_IsNewCardPresent();
    if (!present || !rfid.PICC_ReadCardSerial()) return;

    Serial.print(F("UID:"));
    for (byte i = 0; i < rfid.uid.size; i++) {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(rfid.uid.uidByte[i], HEX);
    }
    // course2 RFID_DEST[] 에 붙여넣을 줄 (배송지는 직접 바꿈)
    Serial.print(F("    붙여넣기용:  { "));
    Serial.print(rfid.uid.size);
    Serial.print(F(", { "));
    for (byte i = 0; i < rfid.uid.size; i++) {
        if (i) Serial.print(F(", "));
        Serial.print(F("0x"));
        if (rfid.uid.uidByte[i] < 0x10) Serial.print('0');
        Serial.print(rfid.uid.uidByte[i], HEX);
    }
    Serial.println(F(" }, P('H', 6) },"));
    tone(3, 1047, 80);
    rfid.PICC_HaltA();
    delay(500);
}
