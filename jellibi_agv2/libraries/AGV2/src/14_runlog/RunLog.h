// ===================================================================
//  [14 주행 기록] 주행 중 일어난 일을 로봇 안(EEPROM)에 저장 → 전원을 꺼도 남는다
//
//   쓰는 법: 주행이 끝난 뒤(또는 멈춘 뒤) USB 를 꽂고 시리얼 모니터(115200)를 열면
//           course2_pallet 이 시작할 때 '지난 주행 기록'을 출력한다. 그 블록을 그대로 복사해서 전달.
//
//   - 교차점·정지·회전·장애물·팔레트·오류와 그 시각(0.01초 단위)을 기록
//   - 주행을 방해하지 않게 한 번에 1바이트씩, EEPROM 이 한가할 때만 쓴다 (기다리지 않음)
//   - runlogBegin() 을 부른 스케치에서만 기록된다 (종목② ). 안 부르면 아무 일도 안 함
//   - config.h RUNLOG_ON 0 이면 코드에서 빠진다
// ===================================================================
#ifndef AGV2_RUNLOG_H
#define AGV2_RUNLOG_H

#include <Arduino.h>

enum RunLogType : uint8_t {
    RL_START = 1,   // 출발
    RL_NODE,        // 교차점을 셈          a = 교차점 + 방향,  b = 그 칸에서 가장 크게 치우친 정도 (%)
    RL_STOP,        // 교차점 위에 정지      a = 교차점
    RL_TURN,        // 회전 시작            a = 'L' 'R' 'U'(교차점 위 U턴) 'E'(칸 중간 U턴)
    RL_TMISS,       // 회전 중 선을 못 찾아 시간으로 멈춤
    RL_OBST,        // 장애물 감지          a = 막힌 쪽 교차점
    RL_LANE,        // 통로 상태            a = 후보 통로 상태 (2비트씩: 0 모름, 1 열림, 2 막힘)
    RL_PLAN,        // 경로 계산            a = 목적지,  b = 경로 칸 수
    RL_PICK,        // 팔레트 들기          a = 교차점
    RL_DROP,        // 팔레트 내리기        a = 교차점
    RL_HOLD,        // 2초 정차             a = 교차점
    RL_DONE,        // 완주
    RL_FATAL,       // 오류 정지            a = 오류 번호 (COURSE2_TEST.md 표)
    RL_ABORT,       // 버튼 비상정지
    RL_BACK,        // 후진 시작            a = 돌아갈 교차점, b = 셀 교차점 수
    RL_FRONT,       // 정지 상태 전방 측정   a = 교차점, b = 전방 센서 값 / 4
    RL_TYPE_END
};

void runlogBegin();                                        // 새 주행 시작 (출발 직후 1번). 이전 기록은 덮어쓴다
void runlogAdd(uint8_t type, uint8_t a = 0, uint8_t b = 0); // 기록 1줄 추가 (기다리지 않음)
void runlogPump();                                         // 밀린 기록을 1바이트 저장 (주행 루프에서 계속 호출됨)
void runlogFlush();                                        // 밀린 기록을 모두 저장할 때까지 기다림 (정지 상태에서만)
void runlogDump();                                         // 저장된 지난 주행 기록을 시리얼로 출력

#endif
