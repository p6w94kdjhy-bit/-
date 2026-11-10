// ===================================================================
//  [11 출발] 출발 신호 기다리기 — 버튼(A3) / RFID IC 태그 (config.h START_MODE)
// ===================================================================
#ifndef AGV2_START_H
#define AGV2_START_H

#include <Arduino.h>

bool buttonDown();       // 버튼이 지금 눌려 있나 (눌림 = LOW, 디바운스)
void uiWaitStart();      // 출발 신호가 들어올 때까지 대기 → START_DELAY_MS 후 반환
                         //   START_MODE 0: 버튼 눌렀다 뗌 / 1: 태그 / 2: 둘 중 먼저

#endif
