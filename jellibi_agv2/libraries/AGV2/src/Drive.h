// 모터 · 바닥/전방 IR · 라인트레이스 · 교차점 감지 · 회전
// 함수 구조는 강의 예제(drive, Forward, TurnLeft90 ...)를 따른다.
#ifndef AGV2_DRIVE_H
#define AGV2_DRIVE_H

#include <Arduino.h>
#include "config.h"

// ---- 모터 ----
void driveInit();
void drive(int left, int right);     // -255 ~ +255, + = 전진. MOTOR_x_GAIN 적용
void Forward(int speed);
void Backward(int speed);
void Stop();
void Brake();                         // 마지막 명령의 반대 방향 짧은 펄스 후 정지

// ---- 회전 (교차점 위 제자리 피벗) ----
void pivotTurn(bool left, uint8_t quarters);   // quarters: 1=90°, 2=180°
void TurnLeft90();
void TurnRight90();
void UTurn();

// ---- 센서 ----
struct IrSample { int l; int r; };
IrSample readIr();
int  irFront();
bool irCalibrated();                  // config.h 바닥 IR 측정값이 채워졌는지
bool frontCalibrated();
bool isBlackL(int v);                 // v > IR_L_TH
bool isBlackR(int v);
int  irNormL(int v);                  // 흰=0 ~ 검=1000
int  irNormR(int v);
int  lineError(const IrSample& s);    // 선이 왼쪽에 있으면 + (=왼쪽으로 꺾어야 함)

// ---- 라인트레이스 ----
void lineTraceReset();
void lineTraceStep(const IrSample& s, int speed);

// ---- 교차점: 한 교차점은 반드시 1회만 카운트 ----
class XingDetector {
public:
    void reset();                     // 무장 상태로 초기화 (출발·회전 직후)
    bool update(const IrSample& s);   // 새 교차점을 센 순간에만 true
private:
    uint8_t _cnt = 0;
    bool _armed = true;
    unsigned long _last = 0;
};

// ---- 전방 장애물 (연속 FRONT_CONFIRM_SAMPLES 회) ----
class ObstacleDetector {
public:
    void reset() { _cnt = 0; }
    bool update();
private:
    uint8_t _cnt = 0;
};

#endif
