// ===================================================================
//  [13 RFID] MFRC522 카드 읽기 (SS=D2, RST=D4, SPI)     설정: config.h [11]
//   - 출발 태그 (11_start), 팔레트 확인·배송지 지정 (07_logistics, course2) 에서 사용
//   - config.h 에서 RFID 기능을 모두 끄면(RFID_USED=0) 아무 일도 안 하는 함수로 바뀌어
//     MFRC522 라이브러리 없이도 컴파일된다
//   - RFID 는 D13(SCK)을 상태 LED와 같이 쓴다: rfidOn() 중에는 LED 표시가 안 됨
// ===================================================================
#ifndef AGV2_RFID_H
#define AGV2_RFID_H

#include <Arduino.h>

#define RFID_UID_MAX 10

struct RfidUid {
    uint8_t len = 0;                     // 0 = 없음
    uint8_t b[RFID_UID_MAX];
};

void rfidOn();                           // SPI·리더 켜기
void rfidOff();                          // SPI 끄기 (D13 을 LED 로 되돌림)
uint8_t rfidVersion();                   // 리더 칩 버전: 0x91/0x92 정상, 0x00/0xFF 연결 안 됨
bool rfidRead(RfidUid& uid);             // 지금 카드가 있으면 읽어서 true (한 번 읽은 카드도 다시 읽음)
bool rfidWait(RfidUid& uid, unsigned long ms);   // ms 동안 카드를 기다림
bool rfidSame(const RfidUid& a, const RfidUid& b);
void rfidPrint(const RfidUid& uid);      // "DE AD BE EF" 출력

#endif
