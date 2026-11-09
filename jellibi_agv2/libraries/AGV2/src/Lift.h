// 리프트 서보 (D9)
#ifndef AGV2_LIFT_H
#define AGV2_LIFT_H

#include <Arduino.h>

void liftInit();     // 내린 상태로 초기화 후 detach
void LiftUp();
void LiftDown();
bool liftIsUp();

#endif
