# 젤리비 AGV2 — 스마트물류 로봇 챌린지 (2026.11.13 대구 EXCO)

## 구성
```
jellibi_agv2/                  ← Arduino IDE '스케치북 위치'로 지정 (libraries/ 자동 인식)
├─ libraries/AGV2/src/
│   ├─ config.h               ★ 보정값은 전부 여기 (IR·전방·모터비율·속도·회전·리프트·팔레트)
│   ├─ pins.h                 핀맵 (A6/A7 바닥IR, A0 전방, D5/D6 PWM, D7/D8 방향, D9 서보, A3 버튼, D3 부저)
│   ├─ Motor.*                바퀴 모터: drive / Forward / Backward / Stop / Brake
│   ├─ Sensor.*               센서: readIr / isBlackL·R (흑백 판정) / irFront
│   ├─ LineTrace.*            ★ 라인트레이싱: LineTraceToCross(다음 교차점까지) / AlignOnCross(교차점 위 정지)
│   │                         강의안 방식(좌우 IR 4가지 경우), 교차점 1회 카운트
│   ├─ Turn.*                 ★ 회전: TurnLeft90 / TurnRight90 / UTurn (강의안 방식: 검정→흰색까지)
│   ├─ Nav.*                  (종목②) 격자 위치·방향 추적, 직선 구간 주행
│   ├─ Grid.*  Lift.*  Ui.*
├─ calib_ir/                  ① 현장 IR 측정 → config.h 줄 출력
├─ tune_drive/                ② 직진·회전·정렬·리프트 보정 테스트
├─ course1_drive/             종목① 지정경로 순회정차
├─ course2_pallet/            종목② 팔레트 운반 + 장애물 (제출본)
├─ tools/build_check.sh       4개 스케치 컴파일 확인
├─ tools/sim/run.sh           PC에서 경로 로직 시뮬레이션 (종목① 순서, 종목② 차단 3가지)
└─ CHANGELOG.md               보정값 변경 기록
```

### Arduino IDE 설정
1. 파일 → 기본 설정 → **스케치북 위치 = 이 `jellibi_agv2` 폴더** → IDE 재시작 (AGV2 라이브러리 인식)
2. 보드 Arduino Nano / 프로세서 ATmega328P (업로드 실패 시 Old Bootloader), CH341 드라이버, 로봇 전원 ON
3. 시리얼 모니터 **115200**

arduino-cli: `arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328 --libraries libraries course1_drive`
(또는 `./tools/build_check.sh`)

## 현장 절차 (순서대로)
1. **calib_ir** 업로드 → 대회 조명 아래 맵 위에서
   - `w` 흰 바닥 / `b` 로봇을 선과 직각으로 놓아 두 센서가 선 중앙 위 / `c` 선을 따라 정중앙 / `f` 장애물을 멈출 거리에 두고
   - `p` → 출력된 `#define` 줄을 `config.h` [1][2]에 붙여넣기. 기준값 = (흰+검)/2 는 자동 계산
   - 경고가 나오면 그대로 따름 (`XING_LEVEL_PCT`, `LT_CENTER_OFFSET`)
   - IR 값이 `-1`(미입력)이면 주행 스케치는 출발하지 않고 경고음만 낸다
2. **tune_drive** 업로드 → 순서대로 맞추기
   | 명령 | 확인 | 조정 상수 |
   |---|---|---|
   | `s` | 앞으로 가는가 / 똑바로 가는가 | `*_FWD_LEVEL`, `MOTOR_L/R_GAIN` |
   | `L` | 정확히 한 바퀴 | `TURN90_MS` |
   | `l` `r` `u` | 선 위에서 멈추는가 | `TURN_BLIND_PCT`, `TURN_EXIT_MAX_MS` |
   | `f` | 바퀴축 중점이 교차점 ±30mm | `ALIGN_MS`, `BRAKE_*` |
   | `F` | 교차점을 한 번씩만 세는가(부저 틱) | `XING_SKIP_MS`, `XING_LEVEL_PCT` |
   | `q` | 사각 주행 후 제자리 | 종합 |
   | `p` `d` `x` | 리프트 높이, 팔레트 밑 탈출 | `LIFT_*`, `EXIT_BACK_*` |
   명령을 한 번 입력한 뒤에는 버튼으로 반복 실행 가능
3. course1 / course2 업로드 → 버튼 누르고 떼면 `START_DELAY_MS` 후 출발. 주행 중 버튼 = 비상정지
4. 안정화되면 `SPEED_CRUISE` → `SPEED_APPROACH` 순으로 올린다 (APPROACH를 바꾸면 `ALIGN_MS` 재조정)

## 동작 원리
- **교차점 카운트**: 두 바닥 센서가 동시에 검정(연속 `XING_CONFIRM_SAMPLES`회) → 1회 카운트 후, 선을 벗어나고 `XING_SKIP_MS`가 지나야 다음 카운트
- **정차 위치**: 센서가 교차점을 본 뒤 `ALIGN_MS` 만큼 라인트레이스로 더 가서 정지 → 바퀴축 중점이 교차점 위. 정차·회전할 교차점 직전 1칸은 항상 `SPEED_APPROACH`라서 정렬 시간이 일정
- **회전**: 제자리 피벗(바퀴축 중심) → 회전해도 정차 위치 유지. 예상시간 `TURN_BLIND_PCT`%까지는 센서 무시, 그 뒤 진행쪽 센서가 선을 만나면 정지 (막다른 끝·칸 중간 U턴도 동일)
- **라인트레이싱(LT_MODE 0, 강의안 방식)**: 둘 다 흰색 → 직진 / 왼쪽만 검정 → 왼쪽으로(안쪽 바퀴 `LT_INNER_PCT`%) / 오른쪽만 검정 → 오른쪽으로 / 둘 다 검정 → 교차점. `LT_MODE 1`은 비례 제어
- **종목①**: 경로표 `ROUTE[]`에 교차점마다 동작(GO/LEFT/RIGHT/UTURN/END)과 2초 정차를 한 줄씩 적음 → 교차점마다 ①라인트레이싱 ②통과 또는 ③정지 ④정차 ⑤회전
  `H2 G2 F2(R) F3 F4 F5(L) E5 D5 C5(정차→R) C6 C7(L) B7(U) C7 D7 E7 F7(R) F6(L) G6 H6(정차, 남향)`
- **종목②**: 파란 통로 목록 `BLUE[]` + 배송 `JOBS[]` → 매번 최단경로(Dijkstra, 칸·회전 비용)를 계산
  - 미확인 후보 통로(D2/D4/D6 열, B~F행) 칸에서만 전방 IR 감시 → 감지 시 정지·U턴·그 통로 막힘 처리·재계획 (밀지 않음)
  - 통로 하나를 통과하면 나머지는 막힘으로 추론(`OPEN_LANES=1`), 2곳 막힘을 찾으면 남은 곳은 열림
  - `FRONT_CHECK_WHILE_CARRY=0`(기본): 팔레트가 전방 IR을 가릴 수 있으므로 **빈 차로 열린 통로를 먼저 확인**한 뒤 팔레트를 든다. 들고 있을 땐 확인된 통로만 사용
  - `AUTO_ORDER=1`: 열린 통로를 안 뒤 ①/② 중 빠른 순서를 자동 선택
  - 하역: 정렬 → 내림 → `EXIT_BACK_MS` 후진으로 팔레트 밑 탈출 → U턴 → 다음 목적지
- 시뮬레이션(`tools/sim/run.sh`) 결과: 차단 조합 3가지 모두 성공, 47~61칸 이동(추정 68~80초 < 180초)

## 제출(13:20) 규칙
- 제출 직전 `git tag submit-course2` 로 고정. 이후 `course2_pallet.ino`·라이브러리 코드는 수정 금지, `config.h`의 승인된 보정값만 수정 → `CHANGELOG.md`에 기록
- 종목① 개선은 `course1_drive`에서 자유롭게 (공용 라이브러리를 바꾸면 종목②에도 영향 → 제출 후에는 금지)

## 미확인 사항 (현장/공지로 확인 후 반영)
| 항목 | 현재 가정 | 바꿀 곳 |
|---|---|---|
| 파란 통로 배치 | 2열·6열 A~H, 4열 B~H, B행·G행 2~6열 | `course2_pallet.ino` `BLUE[]` |
| 종목② 출발 위치·방향 | H4 바깥 50mm, 북쪽 | `START_NODE`, `START_HDG` |
| 팔레트 크기·다리 간격 | 로봇이 팔레트 밑으로 들어가 교차점 위에서 들어올림 | `PICK_ADJ_MS`, `DROP_ADJ_MS`, `EXIT_BACK_*` |
| 팔레트 진입 방향 | A2·A6 모두 남쪽(B행)에서 북향 진입 | `BLUE[]` |
| 팔레트가 교차점 위에 있는지 | 교차점 중심 | `PICK_ADJ_MS` |
| 팔레트를 든 채 전방 IR이 보이는지 | 안 보임 | `FRONT_CHECK_WHILE_CARRY` |
| 파란 선의 IR 값 | 검은 선과 같이 측정 | calib_ir `b` 를 파란 선에서 |
| 정차 허용오차 | ±30mm | `ALIGN_MS` |
