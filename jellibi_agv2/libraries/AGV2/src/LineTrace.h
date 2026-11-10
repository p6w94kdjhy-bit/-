// ===================================================================
//  라인트레이싱 — 선을 따라 달리다가 교차점에서 멈추는 함수 (강의안 6차시)
//
//  기본 방식(LT_MODE 0, 강의안과 같음): 좌우 IR 흑/백으로 4가지 경우 판단
//     왼쪽 IR   오른쪽 IR   동작
//     흰색      흰색        직진 (선이 두 센서 사이 = 가운데)
//     검정      흰색        왼쪽으로 꺾기 (선이 왼쪽으로 치우침)
//     흰색      검정        오른쪽으로 꺾기
//     검정      검정        교차점 (가로선)
//
//  주행 코드에서 쓰는 함수는 2개뿐:
//     LineTraceToCross(속도)  : 다음 교차점을 만날 때까지 선 따라 달리기 (멈추지 않고 반환)
//     AlignOnCross()          : 교차점 위에 바퀴축이 오도록 조금 더 가서 정지
// ===================================================================
#ifndef AGV2_LINETRACE_H
#define AGV2_LINETRACE_H

#include <Arduino.h>
#include "Sensor.h"

// 다음 교차점까지 라인트레이싱. 교차점을 세면 true.
// checkObstacle=true 면 전방 장애물을 보는 순간 급정지하고 false (종목②).
// HOP_TIMEOUT_MS 안에 교차점을 못 찾으면 정지(오류).
bool LineTraceToCross(int speed, bool checkObstacle = false);

// 교차점 감지 직후 호출: ALIGN_MS 동안 더 가서 바퀴축 중점을 교차점 위에 두고 정지
void AlignOnCross();

// --- 내부용 (회전·출발 직후 상태 초기화, 한 번의 조향) ---
void lineTraceReset();
void lineTraceStep(const IrSample& s, int speed);

#endif
