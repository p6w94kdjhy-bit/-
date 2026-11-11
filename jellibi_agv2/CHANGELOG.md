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
| 11-11 | `MOTOR_L_GAIN` 97→94, `TURN90_MS` 420→460, `TURN_USE_SENSOR` 1→0(임시) | | tune_drive 2차: 여전히 오른쪽으로 휨, L 약 330°, r 조금만 돎, F 정상(3칸) |
| 11-11 | `TURN_USE_SENSOR` 0→1, `TURN_STOP_SENSOR` 1(신규) | | 우회전은 왼쪽 IR, 좌회전은 오른쪽 IR이 새 선에 닿으면 정지 (사용자 실측 요청) |
| 11-11 | `SPEED_CRUISE` 90→120, `SPEED_APPROACH` 60→70, `ALIGN_MS` 150→130, `TURN_AFTER_MS` 40(신규), `TURN_MIN_PCT` 85(신규) | | 교차점 좌우 회전이 덜 됨 → 최소 회전 보장 + 감지 후 추가 회전. 속도 상향 |
| 11-11 | `SPEED_CRUISE` 120→168, `SPEED_APPROACH` 70→98, `SPEED_CARRY` 75→98, `ALIGN_MS` 130→93 | | 종목① 완주 확인(커밋 89f16ba) 후 주행 속도 ×1.4. 회전 설정은 그대로. 되돌리기: 120/70/75/130 |
| 11-11 | `LT_MODE` 0→1, `LT_KP` 8→70(의미 변경: %), `LT_KD` 20→0 | | 직진하면서 계속 조향(비례 제어). 한쪽 IR이 선 위에 완전히 올라가면 안쪽 바퀴 30%로 = 방식 0과 같은 최대 조향. 되돌리기: LT_MODE 0 |
| 11-11 | `TURN_AFTER_MS` → `TURN_AFTER_LEFT_MS` 40 / `TURN_AFTER_RIGHT_MS` 40 / `UTURN_AFTER_MS` 40 | | 좌·우·U턴 각도를 따로 조정할 수 있게 분리. config.h [6]에 조정 안내표 추가 |
| 11-11 | 회전 값을 각도(°) 단위로 변경: `TURN_AFTER_LEFT_DEG` 8, `TURN_AFTER_RIGHT_DEG` 8, `UTURN_AFTER_DEG` 8, `TURN_MIN_DEG` 77, `UTURN_MIN_DEG` 153, `TURN_BLIND_DEG` 36, `UTURN_BLIND_DEG` 126 / `SOUND_CROSS` 1(신규) | | 동작은 이전과 같음(40ms=8°, 85%=77°, 40%=36°). 교차점 인식 시 삑 소리 추가 |
| 11-11 | `UTURN_AFTER_DEG` 8→0, `UTURN_MIN_DEG` 153→143 | | U턴 약 10° 더 돎 |
| 11-11 | (코드) AFTER 각도 음수 지원 | | 음수 = 멈춘 뒤 그 각도만큼 반대로 되돌림. 이전에는 음수를 넣으면 회전이 멈추지 않았음 |
| 11-11 | `TURN_AFTER_LEFT_DEG` 8→30, `TURN_AFTER_RIGHT_DEG` 8→30, `UTURN_AFTER_DEG` 0→-30, `UTURN_MIN_DEG` 143→105, `UTURN_BLIND_DEG` 126→100 | | 사용자 노트북에서 현장 조정한 값 반영 |
| 11-11 | U턴 새로 작성 (04_turn/UTurn.cpp, config [6-U]): `UTURN_BACK_MS` 150, `UTURN_BACK_PWM` 70, `UTURN_BLIND_DEG` 90, `UTURN_AFTER_DEG` 0, `UTURN_EXIT_MAX_MS` 300, `UTURN_SEEK_EXTRA_DEG` 40 / `UTURN_MIN_DEG` 삭제 | 이전 현장값 AFTER -30, MIN 105, BLIND 100 은 방식이 달라 초기화 | 강의안 180도 방식: 후진 → 회전 → 진행쪽 IR 검정 → 흰색 → 정지 |
| 11-11 | 직전 U턴 후진 변경(a4c8d71) 되돌림 / `TURN_AFTER_LEFT_DEG` 30→45, `TURN_AFTER_RIGHT_DEG` 30→45, `UTURN_AFTER_DEG` 0→30 | | 사용자 요청: 좌·우 15° 더, U턴 30° 더 (9613b62 기준) |
| 11-11 | course1 경로표에 교차점별 `adjDeg` 칸 추가, F6 = -30 | | F6 좌회전만 약 30° 더 돎. 다른 회전 값은 그대로 |
| 11-11 | `SPEED_APPROACH` 98→70, `ALIGN_MS` 93→130, `TURN_USE_MOTOR_GAIN` 0(신규) | | 좌회전만 과함 → 회전 때 좌우 보정 빼서 대칭, 정차 직전 속도 낮춰 정차 위치 안정 |
