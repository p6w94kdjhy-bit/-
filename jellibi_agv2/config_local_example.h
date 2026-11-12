// =====================================================================
//  노트북 전용 보정값 — config_local.h 예시
//
//  쓰는 법
//   1) 이 파일을 복사해서 libraries/AGV2/src/ 폴더에 넣고 이름을 config_local.h 로 바꾼다
//      (최종 위치: jellibi_agv2/libraries/AGV2/src/config_local.h)
//   2) 바꾸고 싶은 값만 아래처럼 #define 으로 적는다 (앞의 // 를 지우면 적용)
//   3) 저장 → Arduino IDE 에서 스케치 업로드
//
//  - 이 파일(config_local.h)은 GitHub 에 올라가지 않는다 → Pull 해도 "would be overwritten" 충돌이 나지 않는다
//  - 여기 적은 값이 config.h 의 값보다 먼저 적용된다. 여기 없는 값은 config.h 값 그대로
//  - 업로드하면 시리얼 모니터 맨 위에 "★ config_local.h 사용 중" 이 나온다
//  - 값이 확정되면 Claude 에게 알려서 config.h 에 넣고, 이 파일에서는 지운다 (안 지우면 계속 이 값이 우선)
// =====================================================================

// 예) 속도
// #define SPEED_CRUISE     190
// #define SPEED_APPROACH    70
// #define C1F_CRUISE       210      // course1_fast 통과 속도
// #define C1F_LATE_BRAKE_PCT 55     // course1_fast 늦은 감속

// 예) 바닥 IR (calib_ir p 출력을 그대로 붙여넣어도 됨)
// #define IR_L_WHITE  475
// #define IR_L_BLACK  957

// 예) 회전
// #define TURN_AFTER_LEFT_DEG  47
// #define TURN_AFTER_RIGHT_DEG  0
