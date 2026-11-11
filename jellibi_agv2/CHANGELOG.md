# 보정값 변경 기록

형식: `날짜 시각 | 상수 | 이전 → 이후 | 이유` (config.h 변경 시 한 줄씩)

| 일시 | 상수 | 변경 | 이유 |
|---|---|---|---|
| 11-09 | (초기) | IR·전방 = -1(미측정), 나머지 초기값 | 첫 작성. 현장 측정 전 |
| 11-09 | `HOP_TIMEOUT_MS`, `START_XING_PASSED` 추가 | 4000 / 0 | 교차점 놓침 시 정지, 출발 위치에서 센서가 첫 교차점을 지난 경우 대응. 현장 측정 항목에 `[현장]` 표시 |
| 11-10 | `SOUND_ON` 추가 | 0 (무음) | 부저 끔. 상태는 보드 LED(D13)로 표시 |
| 11-10 | `START_MODE` 추가 | 0 (버튼) | RFID 태그 출발 옵션(1=태그, 2=버튼 또는 태그). rfid_test 스케치 추가 |
| 11-10 | `LT_MODE`, `LT_INNER_PCT`, `TURN_EXIT_MAX_MS` 추가 / `TURN_CENTER_MS` 삭제 | 0 / 30 / 300 | 강의안 방식 라인트레이싱(4가지 경우)·회전(검정→흰색)으로 변경, 코드를 Motor/Sensor/LineTrace/Turn 파일로 분리 |
| 11-10 | `FRONT_NEAR_IS_HIGH`, `PLAN_COST_*` 추가 | 1 / 10,6,10,15 | 라이브러리를 기능별 폴더(01~12)로 분류, 팔레트·경로계획을 라이브러리로 이동, CODE_MAP.md 추가 |
| 11-11 | `IR_R_WHITE` `IR_R_BLACK` `FRONT_OBSTACLE_TH` `FRONT_NEAR_IS_HIGH` `LT_CENTER_OFFSET` | -1→428, -1→959, -1→29, 1→0, 0→18 | 연습 측정값(10mm 선). 왼쪽(L) 값은 전달받지 못해 아직 -1 |
| 11-11 | `FRONT_OBSTACLE_TH` `LT_CENTER_OFFSET` | 29→660, 18→-8 | 2차 연습 측정. IR 흰/검 4줄은 아직 미전달(L -1, R 1차값 유지) |
| 11-11 | `IR_L_WHITE` `IR_L_BLACK` `IR_R_WHITE` `IR_R_BLACK` | -1→486, -1→963, 428→426, 959→957 | 2차 연습 측정(10mm 선) 전체 반영. 기준값 L=724 R=691. 주행 가능 상태 |
| 11-11 | `MOTOR_L_GAIN` 100→97, `TURN_BLIND_PCT` 65→40, `UTURN_BLIND_PCT` 70(신규), `TURN_EXIT_MAX_MS` 300→0, `XING_CLEAR_SAMPLES` 8(신규) | | tune_drive 1차: 직진 약간 오른쪽, l 약 120°, F 한 칸에서 정지(교차점 중복 카운트로 추정) |
