// ===================================================================
//  모터 — 바퀴 2개를 돌리는 가장 기본 함수 (강의안 4차시)
// ===================================================================
#ifndef AGV2_MOTOR_H
#define AGV2_MOTOR_H

#include <Arduino.h>
#include "config.h"

void driveInit();                     // 핀 설정 (setup에서 1번)
void drive(int left, int right);      // 왼쪽/오른쪽 바퀴 속도 -255 ~ +255 (+ = 전진). MOTOR_x_GAIN 적용
void Forward(int speed);              // 전진
void Backward(int speed);             // 후진
void Stop();                          // 정지 (바퀴 힘 끔)
void Brake();                         // 급정지: 반대 방향으로 짧게 밀고 정지

#endif
