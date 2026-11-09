// 버튼(A3) · 부저(D3)
#ifndef AGV2_UI_H
#define AGV2_UI_H

#include <Arduino.h>

void uiInit();
bool buttonDown();                 // 현재 눌림 여부 (디바운스)
void uiWaitStart();                // 버튼 눌렀다 떼면 반환 (출발 신호)
void beep(unsigned int freq, unsigned int ms);
void beepReady();                  // 준비 완료 (도-미-솔)
void beepNode();                   // 짧은 틱
void beepDone();                   // 완료
void uiFatal(const __FlashStringHelper* msg);   // 정지 + 반복 경고음 (복귀 안 함)

// 주행 중 버튼 → 비상정지. 출발 후 uiArmAbort() 로 활성화.
void uiArmAbort();
void uiCheckAbort();               // 루프마다 호출
void waitMs(unsigned long ms);     // delay + 비상정지 체크

#endif
