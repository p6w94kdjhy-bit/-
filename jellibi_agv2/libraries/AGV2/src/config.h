// =====================================================================
//  젤리비 AGV2 보정값 — 보정은 이 파일 한 곳에서만 한다.
//  값을 바꾸면 jellibi_agv2/CHANGELOG.md 에 "무엇을 왜" 한 줄 기록.
//  운반 프로그램 제출(13:20) 이후에는 이 파일의 '승인된 보정값'만 수정.
//
//  [현장] 표시 = 대회장 맵 위에서 직접 측정·확인해서 넣어야 하는 값 (Ctrl+F 로 '[현장]' 검색)
// =====================================================================
#ifndef AGV2_CONFIG_H
#define AGV2_CONFIG_H

#include <Arduino.h>

// ---------------------------------------------------------------------
// [0] 내 노트북 전용 값: config_local.h  (GitHub 에 올라가지 않음 → Pull 해도 충돌 없음)
//   현장에서 값을 바꿀 때는 이 파일(config.h)을 고치지 말고 같은 폴더의 config_local.h 에 적는다.
//   config_local.h 에 적은 값이 아래 기본값보다 먼저 적용된다. 파일이 없으면 아래 값을 그대로 쓴다.
//   처음 만들 때: jellibi_agv2/config_local_example.h 를 이 폴더(libraries/AGV2/src)에 복사 → 이름을 config_local.h 로
// ---------------------------------------------------------------------
#if defined(__has_include)
#if __has_include("config_local.h")
#include "config_local.h"
#define CONFIG_LOCAL_USED 1
#endif
#endif

// ---------------------------------------------------------------------
// [1] 바닥 IR 현장 측정값  (calib_ir 스케치 'w' 'b' 명령으로 측정)
//     흰색 = 값이 작음, 검은색 = 값이 큼.
//     예제 기본값(410/560/710, LT_AJDUST 등) 사용 금지.
//     -1 = 아직 측정 안 함 → 주행 스케치는 출발을 거부하고 경고음.
// ---------------------------------------------------------------------
#ifndef IR_L_WHITE
#define IR_L_WHITE  475   // [현장] 11-12 2차 측정 (A6, 흰 바닥) — calib_ir w
#endif
#ifndef IR_L_BLACK
#define IR_L_BLACK  957   // [현장] 11-12 2차 측정 (A6, 선 정중앙에 센서) — calib_ir b
#endif
#ifndef IR_R_WHITE
#define IR_R_WHITE  408   // [현장] 11-12 2차 측정 (A7, 흰 바닥) — calib_ir w
#endif
#ifndef IR_R_BLACK
#define IR_R_BLACK  952   // [현장] 11-12 2차 측정 (A7, 선 정중앙에 센서) — calib_ir b
#endif

// 적재함을 들고 있을 때의 바닥 IR 값 (적재함 그림자 때문에 값이 달라짐) — 종목②
//   측정: calib_ir 에서 적재함을 로봇 위에 놓고 'u'(리프트 올림) → w, b, c → p  →  출력된 _CARRY 줄을 여기에 붙여넣기
//   -1 = 측정 안 함 → 들고 있을 때도 위의 평소 값을 그대로 사용
//   리프트를 올리면 자동으로 이 값으로, 내리면 평소 값으로 바뀐다 (06_lift → 02_sensor irUseCarry)
#ifndef IR_L_WHITE_CARRY
#define IR_L_WHITE_CARRY  -1   // [현장]
#endif
#ifndef IR_L_BLACK_CARRY
#define IR_L_BLACK_CARRY  -1   // [현장]
#endif
#ifndef IR_R_WHITE_CARRY
#define IR_R_WHITE_CARRY  -1   // [현장]
#endif
#ifndef IR_R_BLACK_CARRY
#define IR_R_BLACK_CARRY  -1   // [현장]
#endif
#ifndef LT_CENTER_OFFSET_CARRY
#define LT_CENTER_OFFSET_CARRY  0   // [현장] 들고 있을 때 선 중앙에서의 err (calib_ir 가 출력)
#endif

// 흑백 판정 기준값 = (흰 + 검) / 2  — 왼쪽·오른쪽 각각 (수정 금지, 위 측정값만 바꾼다)
#ifndef IR_L_TH
#define IR_L_TH  ((IR_L_WHITE + IR_L_BLACK) / 2)
#endif
#ifndef IR_R_TH
#define IR_R_TH  ((IR_R_WHITE + IR_R_BLACK) / 2)
#endif

// 교차점(양쪽 동시 검정) 판정 레벨 %: 흰=0, 검=100.  50 = 위 기준값과 동일.
// 선을 따라가기만 해도 교차점이 잘못 세어지면 calib_ir 'c'(선 중앙 주행 상태) 값보다
// 높아지도록 60~75로 올린다.
#ifndef XING_LEVEL_PCT
#define XING_LEVEL_PCT   50   // [현장] 선 두께에 따라 다름 — calib_ir c → p 경고를 따름
#endif

// ---------------------------------------------------------------------
// [2] 전방 거리센서 IR (A0, 가까울수록 값이 큼: 없으면 100 미만) — 종목② 장애물 감지
// ---------------------------------------------------------------------
#ifndef FRONT_OBSTACLE_TH
#define FRONT_OBSTACLE_TH     250  // [현장] 11-13 대회장: 비었을 때 65~79, 검은 장애물 10cm = 467 → 그 사이. 이 값보다 크면 장애물 (10cm 보다 조금 멀리서 감지)
#endif
#ifndef FRONT_NEAR_IS_HIGH
#define FRONT_NEAR_IS_HIGH      1  // [현장] 11-12 실측: 가까울수록 값이 커짐 (없으면 100 미만) → 1
#endif
                                   //  (강의안 5차시: '장애물 없으면 약 970 이상' → 가까우면 작아지는 센서일 수 있음.
                                   //   calib_ir 연속출력에서 손을 가까이 대며 F 값 방향 확인)
#ifndef FRONT_CONFIRM_SAMPLES
#define FRONT_CONFIRM_SAMPLES   3  // 연속 n회 넘어야 장애물로 판정(노이즈 방지)
#endif
#ifndef FRONT_CHECK_WHILE_CARRY            // (PC 시뮬레이션에서 바꿔 시험할 수 있게)
#define FRONT_CHECK_WHILE_CARRY 0  // 팔레트를 든 상태에서도 전방 IR이 장애물을 보면 1
#endif
                                   // 0이면: 열린 통로를 빈 차로 먼저 확인한 뒤 팔레트를 든다

// ---------------------------------------------------------------------
// [3] 모터
// ---------------------------------------------------------------------
#ifndef LEFT_FWD_LEVEL
#define LEFT_FWD_LEVEL   HIGH   // [현장] 예제 기준 왼쪽 전진=HIGH — 실측 확인 (tune_drive 's')
#endif
#ifndef RIGHT_FWD_LEVEL
#define RIGHT_FWD_LEVEL  LOW    // [현장] 예제 기준 오른쪽 전진=LOW — 실측 확인
#endif
#ifndef MOTOR_L_GAIN
#define MOTOR_L_GAIN      94    // [현장] 11-11: 직진 시 오른쪽으로 휨 → 100→97→94. % — 직진('s')에서 오른쪽으로 휘면 L↓, 왼쪽으로 휘면 R↓
#endif
#ifndef MOTOR_R_GAIN
#define MOTOR_R_GAIN     100    // [현장]
#endif
// 느린 속도에서의 좌우 보정 (motor_balance 스케치로 측정). 모터는 속도에 따라 좌우 차이가 달라진다.
//   전진 명령이 MOTOR_GAIN_LOW_PWM 이하면 *_GAIN_LOW, MOTOR_GAIN_HIGH_PWM 이상이면 위의 MOTOR_x_GAIN, 그 사이는 비례해서 섞음
//   후진은 아래 MOTOR_x_GAIN_REV, 제자리 회전에는 보정을 쓰지 않음
//   11-12 측정: PWM 70 전진 L 98 / R 100,  PWM 98·168·210 전진 L 94 / R 100
#ifndef MOTOR_L_GAIN_LOW
#define MOTOR_L_GAIN_LOW     98    // [현장] PWM 70 에서 똑바로 가는 왼쪽 보정 %
#endif
#ifndef MOTOR_R_GAIN_LOW
#define MOTOR_R_GAIN_LOW    100    // [현장]
#endif
#ifndef MOTOR_GAIN_LOW_PWM
#define MOTOR_GAIN_LOW_PWM   70    // 이 속도 이하는 *_GAIN_LOW
#endif
#ifndef MOTOR_GAIN_HIGH_PWM
#define MOTOR_GAIN_HIGH_PWM  98    // 이 속도 이상은 MOTOR_x_GAIN
#endif
// 후진할 때의 좌우 보정 (motor_balance 'b' 로 측정, PWM 70). 후진하는 모든 동작에 적용: 교차점 세며 후진, U턴 전 후진 등
#ifndef MOTOR_L_GAIN_REV
#define MOTOR_L_GAIN_REV     94    // [현장] 94 = 후진 보정 적용 전과 같음 (전진 값 그대로). 11-12 motor_balance 측정값은 99 → 후진이 휘면 99 로
#endif
#ifndef MOTOR_R_GAIN_REV
#define MOTOR_R_GAIN_REV    100    // [현장]
#endif

// ---------------------------------------------------------------------
// [4] 속도 (PWM 0~255)  — 초기값. 회전·정차 안정화 후 CRUISE부터 올린다.
// ---------------------------------------------------------------------
#ifndef SPEED_CRUISE
#define SPEED_CRUISE    168     // [현장] 11-11: 90→120(완주 확인)→168(×1.4). 교차점 통과 구간 (안정화 후 올림)
#endif
#ifndef SPEED_APPROACH
#define SPEED_APPROACH   70     // [현장] 정차/회전할 교차점 직전 1칸 (ALIGN_MS는 이 속도 기준). 11-11: 60→70→98→70 (정차 위치 안정)
#endif
#ifndef SPEED_CARRY
#define SPEED_CARRY      98     // [현장] 팔레트 운반 중 최고속 (SPEED_APPROACH 이상으로 둘 것). 시간 단축 3단계에서 올림
#endif
#ifndef SPEED_ZONE
#define SPEED_ZONE       60     // 장애물 감시 칸(미확인 통로) 속도 — 감지 후 제동거리 확보, 밀면 시도 종료
#endif
#ifndef ACCEL_PER_MS
#define ACCEL_PER_MS      1     // 출발 가속: 1ms당 PWM 증가량 (급출발 슬립 방지)
#endif

// 라인트레이싱 방식 (LineTrace.cpp)
//   0 = 강의안 방식: 좌우 IR 흑/백 4가지 경우로 판단 → '직진 → 꺾기 → 직진' 반복
//         둘 다 흰색 → 직진 / 왼쪽만 검정 → 왼쪽으로 / 오른쪽만 검정 → 오른쪽으로 / 둘 다 검정 → 교차점
//   1 = 비례 제어: 달리면서 좌우 IR 값 차이만큼 계속 조금씩 조향 → 매끄러운 직진  (11-11 기본)
#ifndef LT_MODE
#define LT_MODE          1
#endif
#ifndef LT_INNER_PCT
#define LT_INNER_PCT    30      // [현장] (방식 0) 선 쪽으로 꺾을 때 안쪽 바퀴 속도 % (0=정지, 음수=역회전)
#endif
#ifndef LT_KP
#define LT_KP           70      // [현장] (방식 1) 한쪽 IR이 선 위에 완전히 올라갔을 때 안쪽 바퀴를 줄이는 % 
#endif
                                //  선을 놓치고 바깥으로 나가면 ↑(80~100), 좌우로 흔들리면 ↓(50~60)
#ifndef LT_KD
#define LT_KD            0      // [현장] (방식 1) 흔들림 억제. 흔들리면 50부터 시험 (0 = 안 씀)
#endif
#ifndef LT_CENTER_OFFSET
#define LT_CENTER_OFFSET -12     // (방식 1) 11-12 2차 측정. 선 중앙에 놓았을 때 err (calib_ir c → p 가 출력)
#endif

// course1_drive_v2 전용 라인트레이싱 (원본 course1_drive 는 위 LT_KP/LT_KD 그대로 사용)
//   직진 때 방향을 덜 급하게 고쳐서 더 빠르게 달리기
#ifndef V2_LT_KP
#define V2_LT_KP        35      // [현장] 원본 70의 절반: 같은 치우침에 절반만 꺾음 (흔들리면 ↓, 선 놓치면 ↑)
#endif
#ifndef V2_LT_KD
#define V2_LT_KD        15      // [현장] 흔들림 억제 (0 = 안 씀)
#endif
#ifndef V2_LT_MAX_PCT
#define V2_LT_MAX_PCT   45      // [현장] 안쪽 바퀴를 최대 45% 까지만 줄임 (원본 100% = 멈출 수 있음)
#endif
#ifndef V2_LT_DEADBAND
#define V2_LT_DEADBAND  80      // [현장] 치우침 8% 미만은 무시하고 직진
#endif

// 종목② 전용 라인트레이싱: 부드러운 PD 조향 (course2_pallet 만 사용. 종목①·tune_drive 는 위 LT_KP 그대로)
//   5ms 마다 조향, 선이 다가오는 속도까지 반영(미분) → 좌우 흔들림 없이 직진. 설명: 03_linetrace/LineTrace.cpp
//   ┌──────────────────────────────┬──────────────────────────────────────────┐
//   │ 증상                          │ 바꿀 값                                    │
//   ├──────────────────────────────┼──────────────────────────────────────────┤
//   │ 좌우로 계속 흔들림              │ C2_LT_TD_MS ↑ (75→90→120), 그래도면 C2_LT_KP ↓ │
//   │ 회전 직후 선을 놓치고 벗어남      │ C2_LT_KP ↑ (40→50→60), C2_LT_MAX_PCT ↑ (→100) │
//   │ 달리다 덜덜 떨림(잔진동)         │ C2_LT_TD_MS ↓ (75→60), C2_LT_KP ↓ (40→30)      │
//   │ 새 방식이 이상하면 즉시 되돌리기   │ C2_LT_SMOOTH 0  (종목①에서 검증한 방식으로)    │
//   └──────────────────────────────┴──────────────────────────────────────────┘
#ifndef C2_LT_SMOOTH
#define C2_LT_SMOOTH      1     // [현장] 1 = 부드러운 PD 조향, 0 = 종목①과 같은 비례 제어(LT_KP)
#endif
#ifndef C2_LT_KP
#define C2_LT_KP         40     // [현장] 한쪽 IR이 선 위에 완전히 올라갔을 때 안쪽 바퀴를 줄이는 %
#endif
#ifndef C2_LT_TD_MS
#define C2_LT_TD_MS      75     // [현장] 미분 시간(ms): 클수록 흔들림을 더 강하게 잡음 (0 = 미분 안 씀)
#endif
#ifndef C2_LT_MAX_PCT
#define C2_LT_MAX_PCT    80     // [현장] 안쪽 바퀴를 최대 몇 % 까지 줄일지
#endif
#ifndef C2_LT_SLOW_PCT
#define C2_LT_SLOW_PCT   50     // [현장] 한쪽으로 완전히 치우쳤을 때 (속도 - SPEED_APPROACH) 의 몇 % 를 줄일지 (0 = 감속 안 함)
#endif

// 늦은 감속 (종목②·tune_drive 'F'): 정차할 칸을 처음부터 SPEED_APPROACH 로 가지 않고
//   앞부분은 통과 속도로 달린 뒤 남은 구간만 SPEED_APPROACH 로 접근 → 정차 한 번마다 약 0.5~0.8초 단축
//   값 = 통과 속도로 달리는 구간 (한 칸의 %). 0 = 사용 안 함 (정차 칸 전체를 SPEED_APPROACH 로)
//   한 칸 통과 시간을 주행 중에 직접 재서 쓰므로 배터리 전압이 달라도 같은 거리에서 감속한다.
//   ★ 올리는 순서: 0 으로 완주 확인 → 40 → 55 → 70.  정차 위치가 교차점을 지나치기 시작하면 한 단계 낮춤
#ifndef LATE_BRAKE_PCT                     // (PC 시뮬레이션에서 바꿔 시험할 수 있게)
#define LATE_BRAKE_PCT    0     // [현장]
#endif

// ---------------------------------------------------------------------
// [5] 교차점 · 정차
// ---------------------------------------------------------------------
#ifndef XING_CONFIRM_SAMPLES
#define XING_CONFIRM_SAMPLES 2   // 양쪽 검정이 연속 n회일 때 교차점
#endif
#ifndef XING_SKIP_MS
#define XING_SKIP_MS       150   // [현장] 카운트 직후 이 시간 동안 재카운트 금지 (1회만 카운트)
#endif
#ifndef XING_CLEAR_SAMPLES
#define XING_CLEAR_SAMPLES  8    // [현장] 선을 완전히 벗어났다고 볼 '양쪽 흰색' 연속 횟수 (교차점 두 번 세기 방지)
#endif
#ifndef ALIGN_MS
#define ALIGN_MS           130   // [현장] 11-11: SPEED_APPROACH 70 으로 되돌려 130 (속도 70에서 완주 확인한 값). 교차점 감지 후 바퀴축 중점이 교차점에 올 때까지 더 가는 시간
#endif
                                 // (센서→바퀴축 거리 / SPEED_APPROACH 속도). tune_drive 'f'로 맞춤
#ifndef BRAKE_PWM
#define BRAKE_PWM           80   // 정지 시 역방향 펄스 세기 (0 = 브레이크 안 씀)
#endif
#ifndef BRAKE_MS
#define BRAKE_MS            25
#endif
#ifndef HOP_TIMEOUT_MS
#define HOP_TIMEOUT_MS    4000   // [현장] 한 칸에서 이 시간 안에 교차점을 못 찾으면 정지(선 이탈·맵 밖 주행 방지)
#endif
                                 //  가장 느린 칸 시간의 2~3배. 0 = 사용 안 함
#ifndef STOP_HOLD_MS
#define STOP_HOLD_MS      2300   // '2초 정차' + 여유
#endif

// 후진 (course2_drive: 장애물 앞에서 G·B행으로 돌아가기, 팔레트 들기·내리기 후 빠져나오기)   코드: Nav::backTo
//   교차점을 세며 똑바로 후진 → REV_PAST_MS 더 후진 → 정지 → 앞으로 똑바로 천천히 그 교차점을 다시 찾아 REV_ALIGN_MS 로 정렬
//   ┌──────────────────────────────────────┬─────────────────────────────┐
//   │ 증상 (tune_drive 'b' 로 확인)          │ 바꿀 값                       │
//   ├──────────────────────────────────────┼─────────────────────────────┤
//   │ 후진하며 한 칸 덜/더 감 (교차점 셈 오류) │ REV_SKIP_MS ↑ (같은 선 두 번 셈) │
//   │ 다시 찾을 때 교차점을 못 찾고 다음 칸까지 │ REV_PAST_MS ↑ (선 뒤로 덜 빠짐)   │
//   │ 후진하면서 비뚤어짐 (tune_drive 'B')    │ 오른쪽으로 휘면 REV_L_GAIN ↓, 왼쪽이면 REV_R_GAIN ↓ │
//   │ 다시 찾은 뒤 바퀴축이 교차점 앞/뒤       │ REV_ALIGN_MS ↑(덜 감) / ↓(지나침)  │
//   └──────────────────────────────────────┴─────────────────────────────┘
// course2_drive: T자(선 끝의 짧은 가로선) 위치에서 멈추기 — 하역 위치 H2·H6 은 H행 교차점을 지나 T자에서 정지
#ifndef DROP_AT_BAR
#define DROP_AT_BAR         1    // [현장] 1 = 하역은 H 교차점 정렬 후 T자까지 더 가서 정지,  0 = H 교차점 위에서 하역
#endif
#ifndef PICK_AT_BAR
#define PICK_AT_BAR         0    // [현장] 1 = 적재물이 A행 바깥 T자 위치에 있음 (T자까지 가서 들기),  0 = A 교차점 위에서 들기
#endif
#ifndef BAR_MAX_MS
#define BAR_MAX_MS        900    // [현장] 교차점에서 T자를 찾는 최대 시간. 못 찾으면 그 자리에 정지 (맵 밖으로 안 나가게)
#endif

// course2_drive: 어떤 동작에 후진을 쓸지 (1 = 교차점을 세며 후진, 0 = U턴). 대회 당일 바꿀 수 있게 config 에 둠
#ifndef OBST_BACK
#define OBST_BACK           1    // [현장] 장애물 → 1: F행까지 교차점을 세며 후진 (큰 장애물 앞에서 돌지 않음)  0: 그 자리에서 U턴 후 F행으로
#endif
#ifndef PICK_BACK
#define PICK_BACK           0    // [현장] 적재물 든 뒤 → 0: 그 교차점 위에서 U턴 (11-13: 후진은 경로가 틀어져서 U턴으로)  1: 한 칸 후진
#endif
#ifndef DROP_BACK
#define DROP_BACK           1    // [현장] ① 내린 뒤 → 1: 교차점을 세며 한 칸 위(G행)까지 후진으로 빠져나옴 (11-13 규칙)  0: 그 자리에서 U턴  (② 는 내리면 종료)
#endif
#ifndef REV_SETTLE
#define REV_SETTLE          0    // [현장] 1 = 목표 교차점보다 한 칸 더 후진한 뒤 선 따라 한 칸 전진해 정렬 (옆 어긋남을 바로잡음, 약 1.5초 더 걸림)
#endif                           //        0 = 목표 교차점을 지나자마자 똑바로 앞으로 다시 찾아 정렬 (빠르지만 후진이 비뚤면 위험)
#ifndef REV_PWM
#define REV_PWM            70    // [현장] 후진 속도
#endif
#ifndef REV_PAST_MS
#define REV_PAST_MS       150    // [현장] 돌아갈 교차점 선을 센 뒤 더 후진하는 시간 (그 뒤 앞으로 다시 찾아 정렬)
#endif
#ifndef REV_SKIP_MS
#define REV_SKIP_MS       150    // [현장] 후진 중 한 선을 센 뒤 다음 선을 셀 수 있을 때까지 최소 시간 (T자와 H행 선 사이는 50mm 뿐이라 짧게)
#endif
#ifndef REV_ALIGN_MS
#define REV_ALIGN_MS      ALIGN_MS // [현장] 다시 찾은 가로선에서 바퀴축까지 더 가는 시간 (멈춘 상태에서 출발하므로 ALIGN_MS 보다 조금 길 수 있음)
#endif
#ifndef REV_L_GAIN                       // (PC 시뮬레이션에서 바꿔 시험할 수 있게)
#define REV_L_GAIN        100    // [현장] 후진 때만 추가로 곱하는 좌우 보정 % (MOTOR_L/R_GAIN 위에 곱함). 후진 직진은 tune_drive 'B'
#endif
#ifndef REV_R_GAIN
#define REV_R_GAIN        100    // [현장]
#endif

// ---------------------------------------------------------------------
// [6] 회전 (제자리 피벗: 바퀴축 중심 회전 → 정차 위치 유지)    코드: 04_turn/Turn.cpp
// ---------------------------------------------------------------------
//
//  회전을 멈추는 기준
//   이 로봇에는 바퀴 회전량 센서(엔코더)·자이로가 없어서 '모터가 돈 각도'를 직접 잴 수 없다.
//   그래서 가장 정확한 기준인 '바닥 IR이 새 선에 닿는 순간'으로 멈춘다 (배터리 전압이 바뀌어도 같은 위치).
//   아래 값들은 모두 '각도(°)'로 적는다. 프로그램이 TURN90_MS(90° 도는 시간)로 시간으로 바꿔 쓴다.
//
//  ★★ 회전 각도 직접 조정하는 곳 ★★  (숫자만 바꾸고 저장 → 다시 업로드)
//
//   ┌──────────────────────┬────────────────────────────────────────────┐
//   │ 증상                  │ 바꿀 값 (단위 °)                             │
//   ├──────────────────────┼────────────────────────────────────────────┤
//   │ 좌회전만 덜 돎/더 돎    │ TURN_AFTER_LEFT_DEG   덜 돈 각도만큼 +, 더 돈 만큼 - │
//   │ 우회전만 덜 돎/더 돎    │ TURN_AFTER_RIGHT_DEG  덜 돈 각도만큼 +, 더 돈 만큼 - │
//   │ U턴                   │ 아래 [6-U] U턴 전용 설정                     │
//   │ 0으로 해도 더 돎        │ TURN_MIN_DEG 를 낮춤                         │
//   │ 모든 각도가 같은 비율로  │ TURN90_MS 다시 측정 (tune_drive 'L' = 정확히 한 바퀴) │
//   │  덜/더 돎 (배터리 등)   │                                            │
//   └──────────────────────┴────────────────────────────────────────────┘
//   예) 우회전이 약 10° 덜 돎 → TURN_AFTER_RIGHT_DEG 8 → 18
//   음수도 가능: 멈춘 뒤 그 각도만큼 반대로 되돌림.  예) 좌회전이 10° 더 돎 → TURN_AFTER_LEFT_DEG 8 → -2
//
//   회전 순서: ① 제자리 회전 시작
//            ② 무시 구간: TURN_BLIND_DEG 까지는 센서를 보지 않음
//            ③ 볼 센서(TURN_STOP_SENSOR)가 흰 바닥으로 나온 뒤 새 검은 선에 닿음
//            ④ 닿은 뒤 AFTER 각도만큼 더 돎 (음수면 ⑤ 정지 뒤 그만큼 되돌림)
//            ④-2 그래도 TURN_MIN_DEG 가 안 됐으면 될 때까지 돎 (덜 도는 것 방지)
//            ⑤ 정지
//
#ifndef TURN_AFTER_LEFT_DEG
#define TURN_AFTER_LEFT_DEG  47  // [현장] ★ 좌회전: 선에 닿은 뒤 더 도는 각도 (°). 11-11: 8→30→45→47 (TURN90_MS 변경분 보정, 시간은 그대로 → 정확 유지)
#endif
#ifndef TURN_AFTER_RIGHT_DEG
#define TURN_AFTER_RIGHT_DEG  0  // [현장] ★ 우회전: 선에 닿은 뒤 더 도는 각도 (°). 11-11: 8→30→45→0 (회전 대칭 후 45° 과함 4/5회)
#endif
#ifndef TURN_MIN_DEG
#define TURN_MIN_DEG         80  // [현장] ★ 90° 회전의 최소 각도 (센서가 일찍 걸려도 이만큼은 돎). 11-11: 77→80 (TURN90_MS 변경분 보정)
#endif

#ifndef TURN90_MS
#define TURN90_MS        442     // [현장] 90° 도는 데 걸리는 시간(ms) = 각도↔시간 환산 기준. 11-11: 460→442 ('L' 약 15° 과함: 460×360/375)
#endif
                                 //  tune_drive 'L'(90°×4)이 정확히 한 바퀴가 되게 맞춤. 11-11: 420×4가 약 330° → 460
#define TURN_DEG_TO_MS(deg) ((unsigned long)(deg) * TURN90_MS / 90)   // 각도 → 시간 (수정하지 않음)
#ifndef TURN_PWM
#define TURN_PWM          90     // 회전 속도. 바꾸면 TURN90_MS 다시 측정해야 함
#endif
#ifndef TURN_USE_MOTOR_GAIN
#define TURN_USE_MOTOR_GAIN 0    // 0 = 제자리 회전 때 MOTOR_x_GAIN(직진용 좌우 보정)을 쓰지 않음 → 좌우 대칭 회전
#endif
                                 //  11-11: 좌회전만 과하게 도는 문제로 0. 1 = 예전처럼 보정 적용
#ifndef TURN_BLIND_DEG
#define TURN_BLIND_DEG    37     // [현장] ② 90° 회전: 이 각도까지는 센서 무시. 11-11: 36→37 (TURN90_MS 변경분 보정)
#endif
#ifndef TURN_SEEK_MAX_MS
#define TURN_SEEK_MAX_MS 500     // ③ 목표 각도를 지나서도 선을 못 찾을 때 더 찾아보는 시간 (넘으면 그냥 정지)
#endif
#ifndef TURN_EXIT_MAX_MS
#define TURN_EXIT_MAX_MS   0     // ④ 검정→흰색 기다리는 최대 시간 (0 = 사용 안 함. 11-11: 300이면 왼회전 120°까지 넘어감)
#endif
#ifndef TURN_SETTLE_MS
#define TURN_SETTLE_MS    60     // 정지 후 흔들림 가라앉는 시간
#endif
#ifndef TURN_USE_SENSOR
#define TURN_USE_SENSOR    1     // [현장] 1 = 센서로 선 찾아 정지(권장), 0 = 시간만으로 회전 (TURN90_MS × 횟수)
#endif
#ifndef TURN_STOP_SENSOR
#define TURN_STOP_SENSOR   1     // [현장] 회전을 멈출 때 볼 센서: 1 = 반대쪽(우회전→왼쪽 IR, 좌회전→오른쪽 IR)
#endif
                                 //  0 = 진행쪽. 11-11: 진행쪽이면 우회전이 조금만 돌아서 1

// ---------------------------------------------------------------------
// [6-U] U턴 (180°)    코드: 04_turn/UTurn.cpp     — 강의안 6차시 '180도 뒤돌아 가기' 방식
// ---------------------------------------------------------------------
//
//  U턴 순서:
//   ① 교차점 위에 정지한 상태에서 UTURN_BACK_MS 동안 후진
//      → 바닥 IR이 교차점 가로선 뒤로 빠짐 (회전 중 가로선을 '돌아갈 선'으로 착각하지 않게)
//   ② 제자리 회전 (UTURN_LEFT 방향)
//   ③ 무시 구간: UTURN_BLIND_DEG 까지는 센서를 보지 않음
//   ④ 진행쪽 IR(왼쪽 U턴 → 왼쪽 IR)이 돌아갈 선(검정)을 볼 때까지 회전
//   ⑤ 그 IR이 다시 흰색이 될 때까지 회전 → 선이 두 센서 사이(가운데)에 옴
//   ⑥ 정지 (+ UTURN_AFTER_DEG 만큼 더 돌거나 되돌림)
//   ※ 선을 못 찾으면 180° + UTURN_SEEK_EXTRA_DEG 에서 멈춤 (무한 회전 없음)
//
//  ★ U턴 조정 ★
//   ┌──────────────────────────────┬──────────────────────────────────────┐
//   │ 증상                          │ 바꿀 값                                │
//   ├──────────────────────────────┼──────────────────────────────────────┤
//   │ U턴이 덜 돎 / 더 돎             │ UTURN_AFTER_DEG  덜 돈 만큼 +, 더 돈 만큼 - │
//   │ U턴이 90° 근처에서 멈춤          │ UTURN_BACK_MS ↑ (더 후진) 또는 BLIND ↑   │
//   │ U턴 뒤 C7을 세지 않고 지나침      │ UTURN_BACK_MS ↓ (너무 많이 후진)        │
//   │ 선을 못 찾고 계속 돎             │ UTURN_LEFT 반대로(0) 시험               │
//   └──────────────────────────────┴──────────────────────────────────────┘
//
#ifndef UTURN_LEFT
#define UTURN_LEFT           1   // U턴 회전 방향 1 = 왼쪽, 0 = 오른쪽
#endif
#ifndef UTURN_BACK_MS
#define UTURN_BACK_MS      150   // [현장] ★ ① 회전 전 후진 시간 (ms). 0 = 후진 안 함
#endif
#ifndef UTURN_BACK_PWM
#define UTURN_BACK_PWM      70   // ① 후진 속도
#endif
#ifndef UTURN_BLIND_DEG
#define UTURN_BLIND_DEG     90   // [현장] ③ 이 각도까지 센서 무시
#endif
#ifndef UTURN_AFTER_DEG
#define UTURN_AFTER_DEG     45   // [현장] ★ ⑥ 선을 찾은 뒤 더 도는 각도 (°). 음수 = 정지 후 그만큼 되돌림. 11-11: 0→30→45 (대칭 회전 후 15° 덜 돎)
#endif
#ifndef UTURN_EXIT_MAX_MS
#define UTURN_EXIT_MAX_MS  300   // ⑤ 검정 → 흰색을 기다리는 최대 시간 (0 = ⑤ 생략, 검정 보자마자 정지)
#endif
#ifndef UTURN_SEEK_EXTRA_DEG
#define UTURN_SEEK_EXTRA_DEG 40  // 선을 못 찾을 때 180° 보다 더 돌아볼 각도 (안전장치)
#endif

// ---------------------------------------------------------------------
// [7] 출발 · 소리
// ---------------------------------------------------------------------
// 출발 신호: 0 = 버튼만, 1 = RFID(IC 태그)만, 2 = 버튼 또는 태그 중 먼저 들어온 것
// RFID 설정은 [11]. MFRC522 라이브러리는 AGV2 안에 포함되어 있어 따로 설치하지 않는다
#ifndef START_MODE
#define START_MODE         2     // [현장] 대회 출발 규정에 맞춤. 11-11: 0→2 (버튼 또는 카드 태그, MFRC522 라이브러리 필요)
#endif
#ifndef SOUND_ON
#define SOUND_ON           0     // 1 = 부저 소리 사용, 0 = 무음 (상태는 보드 LED D13 깜빡임으로 표시)
#endif
#ifndef SOUND_CROSS
#define SOUND_CROSS        1     // 1 = 교차점을 인식할 때마다 '삑' (SOUND_ON 0 이어도 남 — 카운트 확인용), 0 = 끔
#endif
#ifndef CROSS_BEEP_HZ
#define CROSS_BEEP_HZ   2000     // 교차점 삑 소리 높이
#endif
#ifndef CROSS_BEEP_MS
#define CROSS_BEEP_MS     60     // 교차점 삑 소리 길이 (주행은 멈추지 않음)
#endif
#ifndef START_DELAY_MS
#define START_DELAY_MS   200     // 버튼 뗀 뒤 출발까지 (신호 후 5초 내 출발선 통과)
#endif
// 출발 위치(교차점 바깥 50mm)에 놓았을 때 바닥 센서가 이미 첫 교차점 선을 '지나' 있으면 1.
// 센서가 선 앞(아직 안 닿음)이거나 선 위면 0.  calib_ir 연속출력으로 확인: 출발 위치에서 앞으로 밀 때
// 두 센서가 같이 검게 변하면 0, 변하지 않으면(이미 지남) 1
#ifndef START_XING_PASSED
#define START_XING_PASSED  0     // [현장]
#endif

// ---------------------------------------------------------------------
// [8] 리프트 서보 (D9)
// ---------------------------------------------------------------------
#ifndef LIFT_UP_ANGLE
#define LIFT_UP_ANGLE    165     // [현장] 11-12 실측 (tune_drive p 후 , 키). 교본값 170
#endif
#ifndef LIFT_DOWN_ANGLE
#define LIFT_DOWN_ANGLE   58     // [현장] 11-12 실측 (tune_drive , 키): 60 = 살짝 떠 있음, 55 = 서보가 버티는 소리 → 58
#endif
#ifndef LIFT_INIT_WAIT_MS
#define LIFT_INIT_WAIT_MS 300     // [현장] 전원 켠 뒤 리프터를 내리기 전에 기다리는 시간 (그래도 올라와 있으면 300→600)
#endif
#ifndef LIFT_INIT_HOLD_MS
#define LIFT_INIT_HOLD_MS 400     // [현장] 내린 각도를 유지하는 시간 (교본 300). 다 내려가기 전에 풀리면 ↑
#endif
#ifndef LIFT_STEP_DEG
#define LIFT_STEP_DEG      3
#endif
#ifndef LIFT_STEP_MS
#define LIFT_STEP_MS      15
#endif
#ifndef LIFT_SETTLE_MS
#define LIFT_SETTLE_MS   250
#endif
#ifndef LIFT_HOLD_WHILE_CARRY
#define LIFT_HOLD_WHILE_CARRY 1  // 1 = 들고 있는 동안 서보 attach 유지(처짐 방지), 내린 뒤 detach
#endif

// ---------------------------------------------------------------------
// [9] 팔레트 (종목②)
// ---------------------------------------------------------------------
#ifndef PICK_ADJ_MS
#define PICK_ADJ_MS        0     // 교차점 정렬 후 들기 전 추가 전진(+)/후진(-) ms
#endif
#ifndef DROP_ADJ_MS
#define DROP_ADJ_MS        0     // 교차점 정렬 후 내리기 전 추가 전진(+)/후진(-) ms
#endif
#ifndef EXIT_BACK_MS
#define EXIT_BACK_MS     700     // 내린 뒤 후진해서 팔레트 밑을 빠져나오는 시간
#endif
#ifndef EXIT_BACK_PWM
#define EXIT_BACK_PWM     80     // (후진거리 + 센서거리 < 200mm 이어야 다음 교차점을 놓치지 않음)
#endif

// ---------------------------------------------------------------------
// [10] 경로 계획 비용 (종목②) — 시간 비율. 보통 수정하지 않음
// ---------------------------------------------------------------------
#ifndef PLAN_COST_HOP
#define PLAN_COST_HOP      10    // 한 칸
#endif
#ifndef PLAN_COST_TURN
#define PLAN_COST_TURN      6    // 90° 회전
#endif
#ifndef PLAN_COST_UTURN
#define PLAN_COST_UTURN    10    // U턴
#endif
#ifndef PLAN_COST_UNKNOWN
#define PLAN_COST_UNKNOWN  15    // 막혔는지 모르는 통로를 지나갈 때 벌점
#endif

// ---------------------------------------------------------------------
// [11] RFID (MFRC522, SS=D2, RST=D4, SPI D11~D13)    코드: 13_rfid/Rfid.cpp
//   MFRC522 라이브러리는 13_rfid/mfrc522 에 포함 (IDE 에 설치할 필요 없음).
//   모두 0 이면 RFID 코드는 아예 들어가지 않는다.
//   카드 번호(UID)는 rfid_test 스케치로 읽는다.
// ---------------------------------------------------------------------
// ① 출발: START_MODE ([7] 출발) — 1 = 태그로 출발, 2 = 버튼 또는 태그
// ② 팔레트 확인: 팔레트 밑에 정차한 뒤 팔레트 카드를 읽어 '제대로 밑에 들어왔는지' 확인
//      0 = 안 씀
//      1 = 확인만 (카드 번호를 로그에 남기고, 못 읽어도 그냥 들기)
//      2 = 확인 + 위치 찾기 (못 읽으면 앞뒤로 천천히 움직이며 카드를 찾은 뒤 들기)
#ifndef PICK_RFID_CHECK
#define PICK_RFID_CHECK      0   // [현장]
#endif
#ifndef PICK_RFID_WAIT_MS
#define PICK_RFID_WAIT_MS  300   // 정차 후 카드를 기다리는 시간
#endif
#ifndef PICK_RFID_SEARCH_MS
#define PICK_RFID_SEARCH_MS 250  // (모드 2) 한 방향으로 찾아보는 시간
#endif
#ifndef PICK_RFID_SEARCH_PWM
#define PICK_RFID_SEARCH_PWM 50  // (모드 2) 찾을 때 속도 (아주 느리게)
#endif
// ③ 카드로 배송지 정하기: 팔레트 카드 번호 → 배송 교차점 (course2_pallet.ino 의 RFID_DEST[] 표)
//      0 = 안 씀 (배송지 고정: A2→H6, A6→H2)
//      1 = 표에 있는 카드면 그 배송지로, 없으면 고정 배송지
#ifndef RFID_DEST_MODE
#define RFID_DEST_MODE       0   // [현장] 대회 규정에 '카드로 배송지 지정'이 있으면 1
#endif

#ifndef RFID_USED
#define RFID_USED ((START_MODE != 0) || (PICK_RFID_CHECK != 0) || (RFID_DEST_MODE != 0))
#endif

// ---------------------------------------------------------------------
// [12] 주행 기록 (종목②)    코드: 14_runlog/RunLog.cpp
//   주행 중 교차점·회전·장애물·오류와 시각을 로봇 안(EEPROM)에 저장한다. 전원을 꺼도 남는다.
//   주행 뒤 USB 를 꽂고 시리얼 모니터(115200)를 열면 course2_pallet 이 '지난 주행 기록'을 출력한다.
// ---------------------------------------------------------------------
#ifndef RUNLOG_ON
#define RUNLOG_ON  1             // 1 = 기록, 0 = 기록 안 함 (코드에서 빠짐)
#endif

#endif
