# 코드 지도 — 기능별 분류

공용 코드는 `libraries/AGV2/src/` 아래 **기능별 폴더**로 나뉘어 있다.
번호는 쌓는 순서다. 아래 번호의 기능은 위 번호의 기능을 가져다 쓴다(예: 03 라인트레이싱은 01 모터, 02 센서를 사용).

| 폴더 | 항목 | 하는 일 | 주로 쓰는 함수 | 관련 보정값 (config.h) |
|---|---|---|---|---|
| `config.h` | 보정값 | 현장 측정값·속도·시간 등 숫자 전부. `[현장]` = 대회장에서 측정 | — | — |
| `pins.h` | 핀 번호 | 어떤 부품이 몇 번 핀에 연결됐는지 | — | — |
| `01_motor` | **모터** | 바퀴 2개의 속도·방향. 좌우 모터 성능 차이 보정 | `drive(왼,오)` `Forward` `Backward` `Stop` `Brake` | `LEFT/RIGHT_FWD_LEVEL` `MOTOR_L/R_GAIN` `BRAKE_*` |
| `02_sensor` | **센서** | 바닥 IR 2개 읽기, 흑/백 판정 (기준 = (흰+검)/2), 전방 IR 값 읽기 | `readIr` `isBlackL` `isBlackR` `irFront` | `IR_L/R_WHITE/BLACK` |
| `03_linetrace` | **라인트레이싱** | 선을 따라 달리다 교차점(양쪽 검정)에서 반환. 교차점은 1번만 셈. 교차점 위에 바퀴축을 맞춰 정지. 기본은 비례 제어(달리면서 계속 조금씩 조향) | `LineTraceToCross(속도)` `AlignOnCross()` | `LT_MODE` `LT_KP` `LT_KD` `LT_INNER_PCT` `XING_*` `ALIGN_MS` `SPEED_*` |
| `04_turn` | **회전** | 교차점 위 제자리 회전: 무시 구간 → 진행쪽 IR 검정 → 흰색 → 정지 | `TurnLeft90` `TurnRight90` `UTurn` | `TURN_*` `UTURN_LEFT` |
| `05_obstacle` | **장애물 감지** | 전방 IR이 기준값을 넘는 상태가 연속 n회면 장애물 | `ObstacleDetector` | `FRONT_OBSTACLE_TH` `FRONT_NEAR_IS_HIGH` |
| `06_lift` | **리프트** | 서보로 천천히 올리기/내리기, 내린 뒤 서보 끄기 | `LiftUp` `LiftDown` | `LIFT_*` |
| `07_logistics` | **물류(팔레트)** | 팔레트 들기 / 내리기 + 후진으로 빠져나오기 + U턴 | `PickPallet` `DropPallet` | `PICK_ADJ_MS` `DROP_ADJ_MS` `EXIT_BACK_*` `SPEED_CARRY` |
| `08_map` | **맵 좌표** | 교차점 이름 ↔ 번호, 방향(N/E/S/W), 이웃 교차점 계산 | `P('C',5)` `stepNode` `headingTo` | — |
| `09_nav` | **격자 주행** | 현재 교차점·방향을 기억하며 "C5까지 직진" 단위로 이동 (03·04·05 사용) | `Nav::leg` `Nav::turnTo` | — |
| `10_planner` | **경로 계획** | 파란 통로 위 최단 경로 계산, 막힌 통로 기억, 장애물 만나면 다른 길로 | `gotoNode` `ensureOpenLane` `plan` | `PLAN_COST_*` `FRONT_CHECK_WHILE_CARRY` |
| `11_start` | **출발** | 버튼 또는 RFID 태그 신호를 기다렸다가 출발 | `uiWaitStart` `buttonDown` | `START_MODE` `START_DELAY_MS` `START_XING_PASSED` |
| `12_signal` | **알림·안전** | LED·부저 상태 표시, 오류 정지, 주행 중 버튼 비상정지 | `beep` `uiFatal` `uiCheckAbort` `waitMs` | `SOUND_ON` |

## 스케치 (실제로 업로드하는 프로그램)

| 스케치 | 용도 | 사용하는 기능 |
|---|---|---|
| `calib_ir` | 명도 측정 → config.h 줄 출력 | 02 센서 |
| `tune_drive` | 직진·회전·정차·리프트 보정 테스트 | 01~04, 06, 09 |
| `course1_drive` | **종목①** 경로표(교차점마다 GO/LEFT/RIGHT/UTURN/END) | 03 라인트레이싱, 04 회전, 11, 12 |
| `course2_pallet` | **종목②** 맵·임무·순서만 정의 | 07 물류, 10 경로 계획 (→ 09, 03, 04, 05) |
| `rfid_test` | RFID 모듈 동작 확인 | (독립) |
| `hello_test` | 업로드 연결 확인 | (독립) |

## 한 교차점에서 일어나는 일 (종목① 기준)

```
03 LineTraceToCross()  선 따라 달리기 ──▶ 양쪽 검정 = 교차점
        │
        ├─ 통과(GO) ─────────────────▶ 다음 교차점으로
        │
        └─ 03 AlignOnCross()  바퀴축을 교차점 위에 두고 정지
               └─ (2초 정차) ─▶ 04 TurnLeft90 / TurnRight90 / UTurn
```

## 종목② 배송 1건

```
10 ensureOpenLane   (팔레트 들기 전에) 열린 통로를 빈 차로 확인
10 gotoNode(A2)     최단 경로로 이동, 장애물 → 05 감지 → U턴 → 다른 길
07 PickPallet       06 리프트 올림
10 gotoNode(H6)     확인된 통로로만 이동
07 DropPallet       06 리프트 내림 → 후진 탈출 → U턴
```
