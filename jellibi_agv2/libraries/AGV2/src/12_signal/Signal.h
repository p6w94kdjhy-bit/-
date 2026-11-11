// ===================================================================
//  [12 알림·안전] 부저(D3)·LED(D13) 상태 표시, 오류 정지, 버튼 비상정지
//   SOUND_ON=0 이면 소리 없이 LED만 같은 패턴으로 깜빡임 (교차점 삑은 SOUND_CROSS 로 따로 켬)
// ===================================================================
#ifndef AGV2_SIGNAL_H
#define AGV2_SIGNAL_H

#include <Arduino.h>

void uiInit();                                   // 버튼·부저·LED 핀 설정 (setup에서 1번)

void beep(unsigned int freq, unsigned int ms);   // 소리(+LED) ms 동안
void beepReady();                                // 준비 완료 (짧게 3번)
void beepNode();                                 // 교차점 인식할 때 '삑'(SOUND_CROSS) + LED 토글
void beepDone();                                 // 완료

void uiFatal(const __FlashStringHelper* msg);    // 오류 정지: 모터 끄고 LED 0.3s/0.7s 반복 (복귀 안 함)

void uiArmAbort();                               // 출발 후 호출 → 버튼 비상정지 활성화
void uiCheckAbort();                             // 주행 루프마다 호출: 버튼 눌리면 비상정지
void waitMs(unsigned long ms);                   // 기다리면서 비상정지도 확인

#endif
