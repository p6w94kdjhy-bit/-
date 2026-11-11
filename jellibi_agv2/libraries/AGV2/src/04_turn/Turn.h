// ===================================================================
//  [04 회전] 교차점 위(정지 상태)에서 제자리 회전 (강의안 6차시)
//
//  회전 순서:
//    ① 제자리 회전 시작 (왼회전 = 왼바퀴 뒤로, 오른바퀴 앞으로)
//    ② 무시 구간: 예상시간의 TURN_BLIND_PCT%(U턴은 UTURN_BLIND_PCT%) 동안 센서를 보지 않음
//    ③ 볼 센서가 흰 바닥으로 나온 뒤 → 새 검은 선에 닿으면 정지
//       볼 센서 = TURN_STOP_SENSOR 1(반대쪽): 우회전 → 왼쪽 IR, 좌회전 → 오른쪽 IR
//                                  0(진행쪽): 우회전 → 오른쪽 IR, 좌회전 → 왼쪽 IR
//    ④ 선에 닿은 뒤 TURN_AFTER_LEFT/RIGHT_MS (U턴 UTURN_AFTER_MS) 더 돌고, 최소 TURN_MIN_PCT%는 반드시 돈다
//    ⑤ 정지
//  TURN_USE_SENSOR 0 이면 센서 없이 TURN90_MS × 횟수 동안만 회전
//
//  제자리 회전이라 바퀴축 중점(정차 기준점)이 교차점 위에 그대로 남는다.
// ===================================================================
#ifndef AGV2_TURN_H
#define AGV2_TURN_H

#include <Arduino.h>

void TurnLeft90();       // 왼쪽 90°
void TurnRight90();      // 오른쪽 90°
void UTurn();            // 180° (UTURN_LEFT 방향)

// 공통 함수: left=true 왼쪽, quarters = 1(90°) / 2(180°)
void spinTurn(bool left, uint8_t quarters);

#endif
