// ===================================================================
//  [02 센서] 바닥 IR 2개(흑/백 판정), 전방 IR 1개 원시값 (강의안 3·5차시)
//  흑백 판정 기준 = (흰 + 검) / 2  (config.h 현장 측정값으로 계산)
// ===================================================================
#ifndef AGV2_SENSOR_H
#define AGV2_SENSOR_H

#include <Arduino.h>
#include "config.h"

struct IrSample { int l; int r; };    // 바닥 IR 한 번 읽은 값 (왼쪽 A6, 오른쪽 A7)

IrSample readIr();                    // 바닥 IR 2개 읽기
int  irFront();                       // 전방 IR 읽기
bool isBlackL(int v);                 // 왼쪽 값이 검정인가 (v > IR_L_TH)
bool isBlackR(int v);                 // 오른쪽 값이 검정인가 (v > IR_R_TH)
bool irCalibrated();                  // config.h에 바닥 IR 현장값이 들어있나

// 적재함을 들었을 때(그림자) 쓰는 측정값으로 바꾸기 / 평소 값으로 되돌리기  (리프트가 자동으로 호출)
//   config.h 의 IR_x_xxx_CARRY 가 -1(미측정)이면 아무것도 바뀌지 않는다
void irUseCarry(bool carrying);
bool irCarryCalibrated();             // CARRY 값 4개가 모두 들어있나
int  irXingThL();                     // 교차점 판정 레벨 (지금 쓰는 측정값 기준, XING_LEVEL_PCT)
int  irXingThR();

// (라인트레이싱 방식 1, calib_ir 용) 흰=0 ~ 검=1000 으로 바꾼 값
int  irNormL(int v);
int  irNormR(int v);
int  lineError(const IrSample& s);    // 선이 왼쪽에 있으면 + 값
int  irCenterOffset();                // 지금 쓰는 선 중앙 보정값

#endif
