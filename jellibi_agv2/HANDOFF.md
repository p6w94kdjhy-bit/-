# 인수인계 — 젤리비 AGV2 대회 프로젝트 (2026-11-10 기준)

## 1. 기본 정보
- 대회: 스마트물류 로봇 챌린지, 2026-11-13 대구 EXCO. 로봇: Jellibi AGV2 (Arduino Nano 호환)
- 저장소: https://github.com/p6w94kdjhy-bit/-  · 브랜치 **`claude/jellibi-agv2-contest-rnqx70`** (main 아님)
- 프로젝트 폴더: `jellibi_agv2/` (저장소 루트의 logicsim 등은 무관한 예전 작업)
- 사용자: 경북대 전기공학과, C++·아두이노 경험. 설명은 한국어로 간결하게
- 클라우드 환경 `jellibi-agv2`: downloads.arduino.cc 허용, 설치 스크립트로 arduino-cli + arduino:avr + Servo + MFRC522 자동 설치
  - 프록시 포트가 세션마다 바뀜 → 라이브러리 설치 실패 시 `arduino-cli config set network.proxy "$HTTPS_PROXY"`, `SSL_CERT_FILE=/root/.ccr/ca-bundle.crt`

## 2. 최우선 규칙 (처음 지시)
- IR 명도값은 예제 기본값(410/560/710, LT_ADJUST) 금지. 현장 측정값을 config.h에, 기준값 = (흰+검)/2 좌우 각각. 미측정이면 -1(TODO), 임의값 금지
- 보정값은 전부 `libraries/AGV2/src/config.h` 한 곳 (`[현장]` 표시 = 대회장 측정 항목)
- 운반 프로그램 13:20 제출·고정 → 이후 경로·로직 수정 금지, 승인된 보정값만
- 커밋 메시지·코드에 모델명 금지. 변경한 상수는 CHANGELOG.md에 기록

## 3. 작업 흐름
Claude가 수정 → 컴파일(`./tools/build_check.sh`) + 시뮬레이션(`./tools/sim/run.sh`) → push →
사용자가 GitHub Desktop **Fetch → Pull** → Arduino IDE에서 스케치 **다시 열고 Upload** → 테스트 → 결과 전달.
업로드는 사용자만 가능(로봇은 사용자 노트북 USB). 수정 후 "어느 스케치를 열어 업로드할지" 항상 알려줄 것.

## 4. 사용자 노트북 상태 (현재 노트북, 대회 땐 다른 노트북 사용 예정)
- Arduino IDE 2.x, CH341 드라이버, GitHub Desktop(계정 p6w94kdjhy-bit 로그인) 완료
- 저장소 clone: `C:\robot\-`, 스케치북 위치: `C:\robot\-\jellibi_agv2`
- hello_test 업로드·시리얼 모니터(115200) 성공. "Arduino PLC IDE"와 혼동한 적 있음(다른 프로그램)
- 새 노트북 세팅 절차: `SETUP_GUIDE.md/.pdf`, 점검 스크립트 `tools/check_setup.ps1`

## 5. 코드 구조 (자세히: CODE_MAP.md)
- 라이브러리 `libraries/AGV2/src/` 기능별 폴더:
  01_motor · 02_sensor · 03_linetrace · 04_turn · 05_obstacle · 06_lift · 07_logistics(팔레트) · 08_map ·
  09_nav · 10_planner(Dijkstra 경로계획) · 11_start(버튼/RFID) · 12_signal(LED·부저·비상정지)
- 라인트레이싱: LT_MODE 0 = 강의안 4가지 경우(둘 다 흰→직진, 한쪽 검정→그쪽으로, 둘 다 검정→교차점). 1 = 비례제어
- 회전: 제자리 피벗 → 무시구간(TURN_BLIND_PCT) → 진행쪽 IR 검정 → 흰색 → 정지
- 스케치: `course1_drive`(경로표 GO/LEFT/RIGHT/UTURN/END), `course2_pallet`(맵·임무·순서만), `calib_ir`, `tune_drive`, `rfid_test`, `hello_test`
- 소리: SOUND_ON 0(무음, 상태는 D13 LED). 출발: START_MODE 0(버튼), 1(RFID), 2(둘 다)
- 시뮬레이션: 종목① 경로 일치(START_XING_PASSED 0/1), 종목② 차단 3가지 × FRONT_CHECK_WHILE_CARRY 0/1 모두 성공(추정 68~80초)

## 6. 현재 config.h 상태
- IR_L/R_WHITE/BLACK = **-1 (미측정)** → 주행 스케치는 출발 거부(LED 오류 깜빡임)
- FRONT_OBSTACLE_TH = -1. 나머지(속도, TURN90_MS, ALIGN_MS 등)는 초기 추정값
- 첫 측정 시도(무효): L 흰504 검1018, R 흰479 검616, c 측정 시 두 센서 모두 ~950
  → R의 b 측정이 선 밖이었고, 테이프가 대회 선(10mm)보다 넓었던 것으로 판단. **10mm 선에서 재측정 필요**

## 7. 미해결 · 해야 할 일 (우선순위)
1. **제출 후 보정 방식 확인 → 결정 필요**: 재업로드 허용인가, 업로드 금지(로봇에서만 보정)인가.
   업로드 금지 대비안 = EEPROM 저장 + 전원 ON 시 버튼 보정 모드(강의안 7차시 방식). 사용자 결정 대기 중
2. 10mm 선에서 calib_ir 재측정(w/b/c/f → p) → config.h 반영
3. 전방 IR 방향 확인: 강의안 5차시 "장애물 없으면 ~970 이상" → 가까울수록 작아질 가능성. `FRONT_NEAR_IS_HIGH` 0/1 결정
4. tune_drive 보정 순서: s(방향·직진) → L(TURN90_MS) → l/r/u → F(LT_INNER_PCT, 1회 카운트) → f(ALIGN_MS) → q
5. `START_XING_PASSED`(출발 위치에서 센서가 H2 선을 이미 지났나) 현장 확인
6. **OT 자료 원본 미확인**: 지금까지 사용자의 텍스트 요약 기준. 경로·맵 방향·출발 방식(RFID 여부)·정차 판정 대조 필요.
   드라이브 공유 폴더 '2026_로봇부트_대회(공유중)'에는 강의안 PDF만 있음(80쪽까지만 읽힘, 8차시 미션·부록 미확인)
7. 종목② 미확인: 파란 통로 배치(`BLUE[]` 가정), 출발 위치(H4 북향 가정), 팔레트 크기·진입 방향·교차점 위 여부,
   팔레트 든 채 전방 IR 가시성(`FRONT_CHECK_WHILE_CARRY`)
8. RFID 출발이 필요하면 노트북 라이브러리 매니저에서 MFRC522 설치, rfid_test로 모듈 확인
9. 대회 전 체크: 새 노트북 세팅(SETUP_GUIDE), 13:20 직전 `git tag submit-course2`
