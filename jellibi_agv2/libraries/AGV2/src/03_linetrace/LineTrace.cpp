#include "03_linetrace/LineTrace.h"
#include "05_obstacle/Obstacle.h"
#include "01_motor/Motor.h"
#include "12_signal/Signal.h"

// 교차점 판정 레벨: 흰 + (검-흰) * XING_LEVEL_PCT%   (50% = 흑백 기준값과 같음)
#define XING_TH_L (IR_L_WHITE + (long)(IR_L_BLACK - IR_L_WHITE) * XING_LEVEL_PCT / 100)
#define XING_TH_R (IR_R_WHITE + (long)(IR_R_BLACK - IR_R_WHITE) * XING_LEVEL_PCT / 100)

static int s_speed = 0;          // 현재 속도 (출발 시 서서히 올림)
static int s_prevErr = 0;        // 방식 1 미분항

// ---- 교차점 1회만 세기 ----
//  양쪽 검정이 연속 XING_CONFIRM_SAMPLES 회 → 1번 셈
//  그 뒤 '양쪽 흰색'이 연속 XING_CLEAR_SAMPLES 회 나오고(선을 완전히 벗어남)
//  XING_SKIP_MS 가 지나야 다음 교차점을 셀 수 있음
//  (선 가장자리에서 한쪽 센서만 잠깐 흰색이 되는 깜빡임으로 같은 교차점을 두 번 세지 않게)
static uint8_t s_crossCnt = 0;
static uint8_t s_clearCnt = 0;
static bool s_crossArmed = true;
static unsigned long s_crossLast = 0;

static bool crossDetected(const IrSample& s)
{
    bool blackL = s.l > XING_TH_L;
    bool blackR = s.r > XING_TH_R;
    if (!(blackL && blackR)) {
        s_crossCnt = 0;
        if (!blackL && !blackR) {                       // 양쪽 흰색
            if (s_clearCnt < 255) s_clearCnt++;
            if (s_clearCnt >= XING_CLEAR_SAMPLES) s_crossArmed = true;
        } else {
            s_clearCnt = 0;
        }
        return false;
    }
    s_clearCnt = 0;
    if (s_crossCnt < 255) s_crossCnt++;
    if (s_crossArmed && s_crossCnt >= XING_CONFIRM_SAMPLES && millis() - s_crossLast >= XING_SKIP_MS) {
        s_crossArmed = false;
        s_crossLast = millis();
        return true;
    }
    return false;
}

void lineTraceReset()
{
    s_speed = 0;
    s_prevErr = 0;
    s_crossCnt = 0;
    s_clearCnt = 0;
    s_crossArmed = true;
    s_crossLast = millis() - XING_SKIP_MS;
}

// ---- 한 번의 조향 ----
void lineTraceStep(const IrSample& s, int speed)
{
#if LT_MODE == 0
    // 강의안 방식: 4가지 경우
    bool L = isBlackL(s.l);
    bool R = isBlackR(s.r);
    int inner = (int)((long)speed * LT_INNER_PCT / 100);
    if (L && !R)      drive(inner, speed);     // 왼쪽만 검정 → 왼쪽으로 꺾기
    else if (!L && R) drive(speed, inner);     // 오른쪽만 검정 → 오른쪽으로 꺾기
    else              drive(speed, speed);     // 둘 다 흰색(가운데) / 둘 다 검정(교차점) → 직진
#else
    // 비례 제어: 달리면서 계속 조금씩 조향 (매끄러운 직진)
    //   err = 왼쪽 IR 어두운 정도 - 오른쪽 IR 어두운 정도  (-1000 ~ +1000, + = 선이 왼쪽으로 치우침)
    //   선이 가운데면 err ≈ 0 → 직진, 선이 한쪽 센서에 걸리기 시작하면 그 정도만큼 살짝 꺾음
    //   보정량 = 속도 × LT_KP% × (err/1000)   → 속도가 바뀌어도 꺾는 비율이 같음
    //   안쪽 바퀴만 줄인다 (바깥 바퀴는 speed 그대로 → 최고 속도 넘지 않음)
    int err = lineError(s);
    long corr = (long)err * speed / 1000 * LT_KP / 100
              + (long)(err - s_prevErr) * speed / 1000 * LT_KD / 100;
    s_prevErr = err;
    if (corr > speed)  corr = speed;            // 안쪽 바퀴 최소 0 (역회전 안 함)
    if (corr < -speed) corr = -speed;
    if (corr >= 0) drive(speed - corr, speed);  // 선이 왼쪽 → 왼바퀴 감속 → 왼쪽으로
    else           drive(speed, speed + corr);  // 선이 오른쪽 → 오른바퀴 감속 → 오른쪽으로
#endif
}

// ---- 다음 교차점까지 달리기 ----
bool LineTraceToCross(int speed, bool checkObstacle)
{
    ObstacleDetector obs;
    unsigned long startMs = millis();
    unsigned long lastMs = startMs;
    for (;;) {
        IrSample s = readIr();
        if (crossDetected(s)) return true;               // 교차점 도착

        unsigned long now = millis();
#if HOP_TIMEOUT_MS > 0
        if (now - startMs > HOP_TIMEOUT_MS) uiFatal(F("교차점을 찾지 못함 (HOP_TIMEOUT_MS)"));
#endif
        if (s_speed < speed) {                            // 출발 가속 (바퀴 미끄러짐 방지)
            s_speed += (int)(now - lastMs) * ACCEL_PER_MS;
            if (s_speed > speed) s_speed = speed;
        } else {
            s_speed = speed;
        }
        lastMs = now;

        lineTraceStep(s, s_speed);                        // 선 따라 조향

        if (checkObstacle && obs.update()) {              // 전방 장애물 → 급정지
            Brake();
            s_speed = 0;
            return false;
        }
        uiCheckAbort();                                   // 버튼 = 비상정지
    }
}

// ---- 교차점 위에 정지 ----
// 바닥 IR은 바퀴축보다 앞에 있으므로, 교차점을 본 뒤 ALIGN_MS 만큼 더 가야 바퀴축이 교차점 위에 온다
void AlignOnCross()
{
    unsigned long t0 = millis();
    while (millis() - t0 < ALIGN_MS) {
        lineTraceStep(readIr(), SPEED_APPROACH);
        uiCheckAbort();
    }
    Brake();
    lineTraceReset();
}
