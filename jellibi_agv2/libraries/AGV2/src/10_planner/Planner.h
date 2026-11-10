// ===================================================================
//  [10 경로 계획] 맵(파란 통로) 위에서 최단 경로를 계산하고, 막힌 통로를 기억 (종목②)
//   - plannerBegin : 통로 목록·장애물 후보 등록
//   - gotoNode     : 목표 교차점까지 계산 → 이동. 장애물을 만나면 그 통로를 '막힘'으로 표시,
//                    U턴 후 다른 길로 다시 계산
//   - ensureOpenLane : (팔레트를 든 채 장애물을 못 볼 때) 빈 차로 열린 통로를 먼저 찾음
//   경로 비용 = 칸 수 + 회전 횟수 + 미확인 통로 벌점 (config.h [10])
// ===================================================================
#ifndef AGV2_PLANNER_H
#define AGV2_PLANNER_H

#include <Arduino.h>
#include "09_nav/Nav.h"

struct Seg { uint8_t a, b; };                          // 직선 통로: a ~ b
enum LaneState : uint8_t { L_UNKNOWN, L_OPEN, L_BLOCKED };

void plannerBegin(const Seg* blue, uint8_t nBlue,       // 사용 가능한 통로
                  const uint8_t* cand, uint8_t nCand,   // 장애물 후보 교차점 (예: D2 D4 D6)
                  uint8_t openLanes,                    // 그중 열린 통로 수
                  Nav& nav);                            // 이 Nav에 장애물 감시·통과 알림 연결

bool plannerHasEdge(uint8_t a, uint8_t b);              // a-b 사이 통로가 있나
bool openLaneKnown();                                   // 열린 통로를 찾았나

// 최단 경로 계산 → planPath[0..planLen-1], planCost
bool plan(uint8_t start, uint8_t h0, uint8_t goal, bool allowUnknown);
extern uint8_t  planPath[64];
extern uint8_t  planLen;
extern uint16_t planCost;

bool gotoNode(Nav& nav, uint8_t goal);                  // 목표까지 이동·정지 (경로 없으면 false)
void ensureOpenLane(Nav& nav);

#endif
