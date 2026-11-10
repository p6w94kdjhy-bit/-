// ===================================================================
//  [06 리프트] 서보(D9)로 리프트 올리기/내리기 (강의안 2차시)
//   천천히 움직이고, 내린 뒤에는 서보를 꺼서(detach) 떨림 소음 방지
// ===================================================================
#ifndef AGV2_LIFT_H
#define AGV2_LIFT_H

#include <Arduino.h>

void liftInit();     // 내린 상태로 초기화 후 detach
void LiftUp();
void LiftDown();
bool liftIsUp();

#endif
