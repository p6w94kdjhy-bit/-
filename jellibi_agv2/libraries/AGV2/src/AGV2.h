// =====================================================================
//  젤리비 AGV2 공용 라이브러리 — 스케치는 #include <AGV2.h> 하나만 쓰면 된다.
//
//  기능별 폴더 (번호 = 아래 기능이 위 기능을 사용하는 순서)
//   config.h        보정값 전부 (현장 측정값은 [현장] 표시)
//   pins.h          핀 번호
//   01_motor        모터: 바퀴 속도·방향 (drive, Forward, Backward, Stop, Brake)
//   02_sensor       센서: 바닥 IR 흑/백 판정, 전방 IR 값 (readIr, isBlackL/R, irFront)
//   03_linetrace    라인트레이싱: 선 따라 다음 교차점까지, 교차점 위 정지 (LineTraceToCross, AlignOnCross)
//   04_turn         회전: 교차점 위 제자리 회전 (TurnLeft90, TurnRight90, UTurn)
//   05_obstacle     장애물 감지: 전방 IR로 앞 장애물 판단 (ObstacleDetector)
//   06_lift         리프트: 서보로 올리기/내리기 (LiftUp, LiftDown)
//   07_logistics    물류: 팔레트 들기·내리기·빠져나오기 (PickPallet, DropPallet)
//   08_map          맵 좌표: 교차점 이름↔번호, 방향 계산 (P('C',5), stepNode)
//   09_nav          격자 주행: 현재 위치·방향 기억하며 구간 이동 (Nav)
//   10_planner      경로 계획: 최단 경로, 막힌 통로 기억·재탐색 (gotoNode)
//   11_start        출발: 버튼 / RFID 태그 신호 대기 (uiWaitStart)
//   12_signal       알림·안전: LED·부저, 오류 정지, 비상정지 (beep, uiFatal, uiCheckAbort)
//   13_rfid         RFID: 카드 읽기 — 출발 태그, 팔레트 확인, 카드로 배송지 (rfidRead, rfidWait)
//   14_runlog       주행 기록: 교차점·회전·장애물·오류와 시각을 로봇 안에 저장 → USB 로 꺼내 보기 (runlogDump)
// =====================================================================
#ifndef AGV2_H
#define AGV2_H

#include <Arduino.h>
#include "config.h"
#include "pins.h"
#include "01_motor/Motor.h"
#include "02_sensor/Sensor.h"
#include "03_linetrace/LineTrace.h"
#include "04_turn/Turn.h"
#include "05_obstacle/Obstacle.h"
#include "06_lift/Lift.h"
#include "07_logistics/Pallet.h"
#include "08_map/Grid.h"
#include "09_nav/Nav.h"
#include "10_planner/Planner.h"
#include "11_start/Start.h"
#include "12_signal/Signal.h"
#include "13_rfid/Rfid.h"
#include "14_runlog/RunLog.h"

#endif
