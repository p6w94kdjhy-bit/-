// ===================================================================
//  [07 물류] 팔레트 들기 · 내리기 · 팔레트 밑에서 빠져나오기 (종목②)
//   로봇이 팔레트 밑 교차점 위에 정지한 상태에서 호출한다.
//   PickPallet : 위치 미세조정(PICK_ADJ_MS) → 리프트 올림 → 운반 속도로 전환
//   DropPallet : 위치 미세조정(DROP_ADJ_MS) → 리프트 내림 → 후진으로 빠져나옴(EXIT_BACK_MS)
//                → U턴 → 방금 지나온 교차점 쪽으로 출발 준비
// ===================================================================
#ifndef AGV2_PALLET_H
#define AGV2_PALLET_H

#include <Arduino.h>
#include "09_nav/Nav.h"

void palletInit();               // 상태 초기화 (setup에서 1번)
bool palletCarrying();           // 지금 팔레트를 들고 있나
void PickPallet(Nav& nav);
void DropPallet(Nav& nav);

#endif
