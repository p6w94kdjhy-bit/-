# 인수인계 — 젤리비 AGV2 대회 (2026-11-11 저녁 기준)

## 기본
- 대회 2026-11-13(금) 대구 EXCO. 로봇 Jellibi AGV2 (Arduino Nano 호환)
- 저장소 https://github.com/p6w94kdjhy-bit/-  브랜치 `claude/jellibi-agv2-contest-rnqx70` (main 아님), 폴더 `jellibi_agv2/`
- 먼저 읽을 것: `CODE_MAP.md`(기능별 폴더 지도), `libraries/AGV2/src/config.h`(보정값 전부), `CHANGELOG.md`(값 변경 이력)
- 사용자: 경북대 전기공학과. 한국어로 간결하게. 단계 안내는 번호로, 줄 번호까지
- 클라우드 환경 `jellibi-agv2`: arduino-cli + arduino:avr + Servo + MFRC522. 프록시 포트가 바뀌면
  `arduino-cli config set network.proxy "$HTTPS_PROXY"`, `SSL_CERT_FILE=/root/.ccr/ca-bundle.crt`
- 확인 도구: `tools/build_check.sh`(전 스케치 컴파일), `tools/sim/run.sh`(PC 경로 시뮬레이션: 종목① + 종목② 차단 3가지)

## 규칙
- IR 명도는 예제 기본값 금지, 현장 측정값만. 기준값 = (흰+검)/2 좌우 각각
- 보정값은 config.h 한 곳, `[현장]` 표시. 바꾸면 CHANGELOG.md 한 줄
- 운반(종목②) 프로그램 13:20 제출 후 코드 고정, config.h 보정값만
- 커밋 메시지·코드에 모델명 금지

## 작업 흐름 (사용자와 합의됨)
Claude 수정 → build_check + sim → push → 사용자가 GitHub Desktop Fetch→Pull → Arduino IDE에서 스케치 닫았다 다시 열고 업로드 → 결과 전달
- 사용자가 노트북에서 config.h를 직접 고치면 Pull 때 "would be overwritten" 창 → 값 메모 → Stash → View stash → Discard → 값 재입력. 고친 값은 Claude 쪽 config.h에도 반영해 줄 것
- 사용자에게 Commit/Push 누르지 말라고 안내함
- 시리얼 모니터 115200, 시작 시 `showConfig()`가 로봇 안 config 값 출력 (반영 확인용)
- 결과는 "5번 중 몇 번, 몇 도" 형식으로 받기로 함. 한 번에 한 값만 바꾸기

## 종목① (완료, 완주 확인)
- `course1_drive`(원본) 완주 OK. `course1_drive_v2` = 직진 조향만 부드럽게(V2_LT_* 값), 비교 테스트 전
- 경로표(교차점별 GO/LEFT/RIGHT/UTURN/END + 2초정차 + adjDeg 각도보정 칸)
- 현재 확정값: IR L 486/963, R 426/957 / CRUISE 168, APPROACH 70, ALIGN_MS 130 / MOTOR_L_GAIN 94 /
  LT_MODE 1(비례), LT_KP 70 / TURN90_MS 442, TURN_AFTER 좌47 우0, TURN_USE_MOTOR_GAIN 0(회전은 좌우 같은 힘),
  TURN_STOP_SENSOR 1(반대쪽 IR로 정지) / U턴(UTurn.cpp): 0.15초 후진 → 회전 → 진행쪽 IR 검정→흰 → UTURN_AFTER 45
- 교차점마다 삑(SOUND_CROSS 1), 그 외 무음(SOUND_ON 0)
- 출발 START_MODE 2 (버튼 또는 RFID 태그). RFID 인식 안 된다는 보고 후 감도 최대·재초기화 넣음 → 결과 미확인

## 종목② (코드 있음 — 새로 짜지 말고 기존 코드를 이어서 다듬을 것)
- `course2_pallet.ino`: 맵 `BLUE[]`, 장애물 후보 `CAND[]`(D2 D4 D6, 열린 통로 1), 임무 `JOBS[]`(A2→H6, A6→H2), 복귀 H4, 출발 H4 북향(가정)
- 라이브러리: 10_planner(Dijkstra, 막힌 통로 기억·재탐색, ensureOpenLane), 07_logistics(PickPallet/DropPallet, 후진 탈출 + UTurnOnEdge),
  09_nav(Nav), 05_obstacle(FRONT_NEAR_IS_HIGH 0 = 가까우면 값 작아짐), 13_rfid(팔레트 카드 확인 PICK_RFID_CHECK, 카드로 배송지 RFID_DEST_MODE + RFID_DEST[] 표, 기본 0)
- 시뮬레이션: 차단 3경우 × FRONT_CHECK_WHILE_CARRY 0/1 모두 성공. **실물 테스트는 한 번도 안 함**
- 실물 테스트 전 확인할 것: 리프트 각도(LIFT_UP 170/DOWN 80, tune_drive p/d), 팔레트 밑 진입·하역 후 후진 탈출(EXIT_BACK_MS 700, tune_drive x),
  PICK/DROP_ADJ_MS, 전방 IR 장애물 값(FRONT_OBSTACLE_TH 660 연습값, 정지 상태에서 거리 확인), 종목②도 START_XING_PASSED

## 공식 예제에서 확인한 것 (RobotNMore/JellibiAGV2, src/01.Applications/Jellibi-AGV-Final-Mission)
- 공식 최종 미션도 **출발 전 RFID 카드 태깅 → 출발** (RunState 0)
- 장애물 확인: 교차점에서 **정지한 뒤** 전방 IR 측정, `analogRead < 990` 이면 장애물 (가까우면 값이 작음 → FRONT_NEAR_IS_HIGH 0과 일치)
  ※ 예제는 전방 중앙 IR을 **A1**(pinLT4)로 씀. 우리 코드는 A0 (OT 핀맵 기준). 실제 어느 핀이 중앙 센서인지 확인 필요
- 경로 선택: 중앙 → 막히면 좌측 확인 → 막히면 우측 (3 경유로 중 장애물 없는 길)
- 팔레트: 목표 교차점에서 정지 → LifterUp → 2초 → 정지선에서 살짝 후진 후 TurnLeft180

## 11-11 밤 추가 (다른 세션에서 작업 → 사용자 요청으로 `claude/jellibi-agv2-contest-rnqx70` 에도 같은 내용 반영)
- 브랜치 `claude/agent-reach-course2-optimization-yha1tn` 과 rnqx70 은 같은 커밋. 사용자는 rnqx70 만 Pull. 두 세션이 모두 rnqx70 에 올리므로 작업 전 반드시 fetch
- 사용자 목표: 종목② 완주 + 시간 단축. 내일(11-12) 실물 테스트 가능. `COURSE2_TEST.md` 가 테스트 순서·보고 형식
- 종목② 라인트레이싱 = 부드러운 PD (`lineTraceSmooth`, config `C2_LT_*`, 기본 KP 40 / TD 75). **실물 미검증**, `C2_LT_SMOOTH 0` 이면 종목① 방식
- 늦은 감속 `LATE_BRAKE_PCT` (기본 0): Nav 가 통과 칸 시간을 재서 정차 칸 앞부분을 통과 속도로
- 주행 기록 14_runlog (EEPROM, 비차단). course2 시작 시 `runlogDump()` 로 지난 주행 출력. uiFatal 오류 번호 표는 COURSE2_TEST.md
- 출발: START_MODE 2 유지 (사용자 확인: rfid_test 에서 카드 번호 잘 읽힘)
- 검증 도구 추가: `tools/sim/run_phys.sh` (실제 LineTrace/Nav/RunLog 코드 + 가짜 로봇). 로봇 치수는 추정값 → 방식 비교·코드 오류 확인용
- 이 세션 환경에는 arduino-cli 없음 → `CORE=.. SERVO=.. RFID=.. tools/build_check.sh` (avr-gcc 직접 컴파일)로 확인함
- 종목①(course1_drive)은 조향 로직 변경 없음 (공용 함수에 기록용 코드만 추가). 내일 종목①도 1회 다시 완주 확인할 것

## 미해결
1. 종목② 실물 테스트 전부 (위 목록)
2. OT 자료 원본 미확인 → 맵(파란 통로), 출발 위치, 팔레트 크기·진입 방향, 출발 방식 확인
3. 제출 후 보정 방식(재업로드 허용 여부) 미확인
4. 대회 노트북 교체 예정 → SETUP_GUIDE.pdf
5. 13:20 제출 직전 커밋 기록 (태그 push는 이 환경에서 막힘 → GitHub Desktop에서 태그 생성 안내)
