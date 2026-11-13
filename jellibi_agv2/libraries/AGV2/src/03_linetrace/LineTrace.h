// ===================================================================
//  [03 라인트레이싱] 선을 따라 달리다가 교차점에서 멈추는 함수 (강의안 6차시)
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
#include "02_sensor/Sensor.h"

// 다음 교차점까지 라인트레이싱. 교차점을 세면 true.
// checkObstacle=true 면 전방 장애물을 보는 순간 급정지하고 false (종목②).
// HOP_TIMEOUT_MS 안에 교차점을 못 찾으면 정지(오류).
bool LineTraceToCross(int speed, bool checkObstacle = false);

// 교차점 감지 직후 호출: ALIGN_MS 동안 더 가서 바퀴축 중점을 교차점 위에 두고 정지
void AlignOnCross();
// 같은 동작을 접근 속도·정렬 시간을 지정해서 (속도를 올린 스케치용: 접근 속도가 다르면 정렬 시간도 달라야 같은 자리에 선다)
void AlignOnCrossAt(int speed, unsigned long ms);

// 후진하면서 가로선(교차점)을 count 개 셀 때까지 뒤로 간다. 다 세면 멈추지 않고(후진 중) true.
//   바닥 IR 이 바퀴축보다 앞에 있어 후진 중에는 선을 따라 조향할 수 없다 → 똑바로 후진 (MOTOR_x_GAIN 적용)
//   후진 중 한쪽 센서가 세로선에 걸려도 교차점으로 세지 않음 (양쪽이 동시에 검정일 때만)
//   한 칸에서 HOP_TIMEOUT_MS 안에 못 찾으면 오류 정지
bool LineTraceBackCross(uint8_t count, int speed);

// 출발용: 천천히 가며 첫 가로선들을 본다. T자 뒤에서 출발했으면 2(T자 + 교차점), T자와 교차점 사이에서 출발했으면 1.
//   돌아올 때 로봇은 첫 교차점을 막 지나 달리는 중 (멈추지 않음)
uint8_t LineTraceStartLines(int speed, unsigned long gapMs);

// T자(선 끝의 짧은 가로선)까지: 바깥 행 교차점에 정지한 상태에서 선 따라 천천히 전진 → 두 센서가 T자를 보는 순간 정지
//   찾으면 true. maxMs 안에 못 찾으면 그 자리에 정지하고 false
bool LineTraceToBar(int speed, unsigned long maxMs);
// 센서가 T자 위에 걸쳐 있으면 조금 더 전진해 완전히 지나감 (후진할 때 T자를 정확히 한 번 세도록)
void ClearBarForward(unsigned long maxMs);

// 후진 뒤: 똑바로(조향 없이) 천천히 앞으로 가서 가로선을 찾고 REV_ALIGN_MS 더 가서 정지 (바퀴축을 교차점 위에)
void RecatchCrossStraight();

// 다음 교차점은 센서가 양쪽 흰색을 지난 뒤에만 세게 함 (후진 뒤 회전처럼 센서가 옛 선 위에서 출발할 때)
void lineTraceDisarm();

// 비례 제어(LT_MODE 1) 강도를 스케치마다 따로 정할 때 (setup 에서 1번)
//   kp: 한쪽 IR이 선 위에 완전히 올라갔을 때 안쪽 바퀴를 줄이는 %
//   kd: 흔들림 억제 (0 = 안 씀)
//   maxPct: 안쪽 바퀴를 최대 몇 % 까지 줄일지 (100 = 정지까지)
//   deadband: 이보다 작은 치우침(0~1000)은 무시
//   호출 안 하면 config.h 의 LT_KP, LT_KD, 100, 0
void lineTraceSetGains(int kp, int kd, int maxPct, int deadband);

// 부드러운 PD 조향 켜기 (setup 에서 1번, 종목②). 호출하지 않으면 위의 기존 방식 그대로.
//   kp     : 한쪽 IR이 선 위에 완전히 올라갔을 때 안쪽 바퀴를 줄이는 %  (0 = 끄고 기존 방식)
//   tdMs   : 미분 시간(ms). 선이 다가오는 속도를 미리 반영 → 흔들림 억제 (0 = 미분 안 씀)
//   maxPct : 안쪽 바퀴를 최대 몇 % 까지 줄일지
//   slowPct: 한쪽으로 완전히 치우쳤을 때 속도를 (속도 - SPEED_APPROACH) 의 몇 % 만큼 낮출지 (0 = 감속 안 함)
void lineTraceSmooth(int kp, int tdMs, int maxPct, int slowPct);

// 늦은 감속: 바로 다음 LineTraceToCross 한 번에 한해, 처음 ms 동안은 fastSpeed 로 달리고 그 뒤 원래 속도로.
//   (정차할 칸을 처음부터 느리게 가지 않고, 앞부분은 빠르게 → 시간 단축. Nav 가 자동으로 호출)
void lineTraceBoost(int fastSpeed, unsigned long ms);

// 직전 호출 이후 가장 크게 치우친 정도(0~1000)를 돌려주고 0 으로 되돌림 (주행 기록용)
uint16_t lineTraceTakeMaxErr();

// --- 내부용 (회전·출발 직후 상태 초기화, 한 번의 조향) ---
void lineTraceReset();
void lineTraceStep(const IrSample& s, int speed);

#endif
