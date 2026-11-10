// 핀맵 (OT자료 기준) — 보정값 아님, 수정하지 않는다
#ifndef AGV2_PINS_H
#define AGV2_PINS_H

#define PIN_IR_L      A6   // 바닥 IR 왼쪽  (흰=작음)
#define PIN_IR_R      A7   // 바닥 IR 오른쪽
#define PIN_IR_FRONT  A0   // 전방 IR (가까울수록 큼)
#define PIN_L_PWM     5
#define PIN_R_PWM     6
#define PIN_L_DIR     7
#define PIN_R_DIR     8
#define PIN_SERVO     9    // 리프트
#define PIN_BUTTON    A3   // 눌림 = LOW (JellibiButton과 동일)
#define PIN_BUZZER    3
#define PIN_LED       13   // 보드 LED (무음일 때 상태 표시)

#endif
