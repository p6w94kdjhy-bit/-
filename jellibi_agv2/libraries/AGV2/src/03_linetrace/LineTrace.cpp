#include "03_linetrace/LineTrace.h"
#include "05_obstacle/Obstacle.h"
#include "01_motor/Motor.h"
#include "12_signal/Signal.h"

// 교차점 판정 레벨: 흰 + (검-흰) * XING_LEVEL_PCT%   (50% = 흑백 기준값과 같음)
//   (적재함을 들면 그림자용 측정값으로 바뀌므로 02_sensor 에서 받아 온다)
#define XING_TH_L (irXingThL())
#define XING_TH_R (irXingThR())

static int s_speed = 0;          // 현재 속도 (출발 시 서서히 올림)

// 비례 제어 강도 (기본 = config.h LT_KP / LT_KD, 스케치가 lineTraceSetGains 로 바꿀 수 있음)
static int s_kp = LT_KP;
static int s_kd = LT_KD;
static int s_maxPct = 100;       // 안쪽 바퀴를 최대 몇 % 까지 줄이나
static int s_dead = 0;           // 이 값보다 작은 치우침(err)은 무시 (0~1000)

void lineTraceSetGains(int kp, int kd, int maxPct, int deadband)
{
    s_kp = kp; s_kd = kd; s_maxPct = maxPct; s_dead = deadband;
}
static int s_prevErr = 0;        // 방식 1 미분항

// ---- 부드러운 PD 조향 (lineTraceSmooth 로 켬, 종목②) ----
//  기존 비례 제어와 다른 점
//   1) 5ms 마다 한 번 조향 (그 사이 읽은 값은 평균 → 센서 잡음 제거)
//   2) 미분(D): 선이 센서 쪽으로 '다가오는 속도'만큼 미리 꺾고, 선이 가운데로 '돌아가는 중'이면 조향을 풀어
//      반대쪽으로 넘어가는 것(좌우 흔들림)을 막는다
//   3) 많이 치우쳤을 때만 잠깐 감속 (선 이탈 방지), 가운데면 최고 속도
//   4) 가로선(교차점)을 지나는 순간에는 조향하지 않음
#define SMOOTH_PERIOD_US  5000UL
#define SMOOTH_XLINE_NORM  300   // 두 센서가 모두 이만큼(0~1000) 어두우면 가로선 위
static bool s_smooth = false;
static int  s_smKp = 0, s_smTdMs = 0, s_smMaxPct = 100, s_smSlowPct = 0;
static long s_smAcc = 0;         // 주기 동안 err 합
static uint16_t s_smN = 0;       // 합한 횟수
static int  s_smErrF = 0;        // 걸러낸 err
static unsigned long s_smLastUs = 0;
static bool s_smFirst = true;

static uint16_t s_maxAbsErr = 0; // 이번 칸에서 가장 크게 치우친 정도 (주행 기록용)

void lineTraceSmooth(int kp, int tdMs, int maxPct, int slowPct)
{
    s_smooth = (kp > 0);
    s_smKp = kp; s_smTdMs = tdMs; s_smMaxPct = maxPct; s_smSlowPct = slowPct;
    s_smFirst = true;
}

uint16_t lineTraceTakeMaxErr()
{
    uint16_t m = s_maxAbsErr;
    s_maxAbsErr = 0;
    return m;
}

static void noteErr(int err)
{
    uint16_t a = (uint16_t)(err < 0 ? -err : err);
    if (a > s_maxAbsErr) s_maxAbsErr = a;
}

static void smoothStep(const IrSample& s, int speed)
{
    int nl = irNormL(s.l), nr = irNormR(s.r);
    int e = nl - nr - irCenterOffset();
    if (nl > SMOOTH_XLINE_NORM && nr > SMOOTH_XLINE_NORM) e = 0;   // 가로선 위: 조향하지 않음
    s_smAcc += e;
    s_smN++;

    unsigned long nowUs = micros();
    unsigned long dt = nowUs - s_smLastUs;
    if (!s_smFirst && dt < SMOOTH_PERIOD_US) return;        // 다음 조향 시각까지 값만 모음

    int avg = (int)(s_smAcc / s_smN);
    s_smAcc = 0;
    s_smN = 0;
    s_smLastUs = nowUs;

    long d = 0;                                             // 미분항 (err 단위로 환산)
    if (s_smFirst || dt > 25000UL) {
        s_smErrF = avg;                                     // 처음·오래 쉰 뒤: 미분 없이 시작
        s_smFirst = false;
    } else {
        int prev = s_smErrF;
        s_smErrF = prev + (avg - prev) / 2;                 // 저역 필터
        d = (long)(s_smErrF - prev) * s_smTdMs * 1000L / (long)dt;
    }
    noteErr(s_smErrF);

    long u = (long)s_smErrF + d;                            // P + D  (-1000 ~ +1000 이 한쪽 센서 완전 검정)
    long pct = u * s_smKp / 1000;                           // 안쪽 바퀴를 줄일 %
    if (pct > s_smMaxPct)  pct = s_smMaxPct;
    if (pct < -s_smMaxPct) pct = -s_smMaxPct;

    int spd = speed;                                        // 많이 치우치면 감속 (SPEED_APPROACH 아래로는 안 내림)
    if (s_smSlowPct > 0 && speed > SPEED_APPROACH) {
        long a = s_smErrF < 0 ? -(long)s_smErrF : (long)s_smErrF;
        if (a > 1000) a = 1000;
        spd = speed - (int)((long)(speed - SPEED_APPROACH) * a / 1000 * s_smSlowPct / 100);
    }
    int corr = (int)((long)spd * pct / 100);
    if (corr >= 0) drive(spd - corr, spd);                  // 선이 왼쪽 → 왼바퀴 감속 → 왼쪽으로
    else           drive(spd, spd + corr);                  // 선이 오른쪽 → 오른바퀴 감속 → 오른쪽으로
}

// ---- 늦은 감속 (lineTraceBoost): 다음 LineTraceToCross 한 번만, 처음 ms 동안 fastSpeed 로 달린다 ----
static int s_boostSpeed = 0;
static unsigned long s_boostMs = 0;
void lineTraceBoost(int fastSpeed, unsigned long ms)
{
    s_boostSpeed = fastSpeed;
    s_boostMs = ms;
}

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
    // 재카운트 금지 시간: 느리게 달릴수록 같은 선 위에 오래 머무르므로 속도에 반비례해서 늘림 (거리로는 같음)
    //   장애물 감시 칸(SPEED_ZONE)처럼 느리게 '통과'할 때 같은 교차점을 두 번 세는 것을 막는다
    unsigned long skip = XING_SKIP_MS;
    if (s_speed > 0 && s_speed < SPEED_CRUISE) {
        skip = (unsigned long)XING_SKIP_MS * SPEED_CRUISE / s_speed;
        if (skip > XING_SKIP_MAX_MS) skip = XING_SKIP_MAX_MS;
    }
    if (s_crossArmed && s_crossCnt >= XING_CONFIRM_SAMPLES && millis() - s_crossLast >= skip) {
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
    s_crossLast = millis() - XING_SKIP_MAX_MS;         // 초기화 직후의 첫 교차점은 시간 제한 없이 셈
    s_smAcc = 0;
    s_smN = 0;
    s_smErrF = 0;
    s_smFirst = true;
}

// 다음 교차점은 '양쪽 흰색'을 한 번 지난 뒤에만 셈 (후진 뒤 회전하면 센서가 옛 선 위에 있을 수 있어서)
void lineTraceDisarm()
{
    s_crossArmed = false;
    s_clearCnt = 0;
    s_crossCnt = 0;
}

// ---- 한 번의 조향 ----
void lineTraceStep(const IrSample& s, int speed)
{
    if (s_smooth) { smoothStep(s, speed); return; }      // 부드러운 PD 조향 (lineTraceSmooth 로 켠 경우만)
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
    //   보정량 = 속도 × KP% × (err/1000)   → 속도가 바뀌어도 꺾는 비율이 같음
//   KP·KD·최대조향·무시구간은 기본 config.h LT_KP/LT_KD, 스케치에서 lineTraceSetGains()로 변경 가능
    //   안쪽 바퀴만 줄인다 (바깥 바퀴는 speed 그대로 → 최고 속도 넘지 않음)
    int err = lineError(s);
    noteErr(err);
    if (err > -s_dead && err < s_dead) err = 0;  // 아주 작은 치우침은 무시 (불필요한 조향 방지)
    long corr = (long)err * speed / 1000 * s_kp / 100
              + (long)(err - s_prevErr) * speed / 1000 * s_kd / 100;
    s_prevErr = err;
    long cmax = (long)speed * s_maxPct / 100;   // 최대 조향량 (안쪽 바퀴 최소 = speed - cmax)
    if (corr > cmax)  corr = cmax;
    if (corr < -cmax) corr = -cmax;
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
    const int slowSpeed = speed;                          // 늦은 감속: 처음 boostMs 동안만 boostSpeed
    const int boostSpeed = s_boostSpeed;
    const unsigned long boostMs = s_boostMs;
    s_boostSpeed = 0;
    s_boostMs = 0;                                        // 한 번만 적용
    for (;;) {
        IrSample s = readIr();
        if (crossDetected(s)) return true;               // 교차점 도착

        unsigned long now = millis();
#if HOP_TIMEOUT_MS > 0
        if (now - startMs > HOP_TIMEOUT_MS) uiFatal(F("교차점을 찾지 못함 (HOP_TIMEOUT_MS)"), 1);
#endif
        speed = (boostMs && now - startMs < boostMs && boostSpeed > slowSpeed) ? boostSpeed : slowSpeed;
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

// ---- 후진하며 교차점 세기 ----
//  전진과 다른 점: 후진 중엔 한쪽 센서가 세로선 위에 계속 있을 수 있어서
//  '양쪽 흰색'이 아니라 '양쪽 검정이 아님'이 연속 XING_CLEAR_SAMPLES 회 + REV_SKIP_MS 가 지나면 다음 교차점을 셀 수 있음
bool LineTraceBackCross(uint8_t count, int speed)
{
    if (count == 0) return true;
    uint8_t both = 0, notBoth = 0;
    bool armed = true;
    unsigned long last = millis() - REV_SKIP_MS;
    unsigned long cellStart = millis();
    unsigned long lastMs = millis();
    int spd = 0;
    for (;;) {
        IrSample s = readIr();
        bool bl = s.l > XING_TH_L, br = s.r > XING_TH_R;
        unsigned long now = millis();
        if (bl && br) {
            notBoth = 0;
            if (both < 255) both++;
            if (armed && both >= XING_CONFIRM_SAMPLES && now - last >= REV_SKIP_MS) {
                armed = false;
                last = now;
                cellStart = now;
                beepNode();
                if (--count == 0) return true;            // 다 셈 (후진은 계속 중 — 호출한 쪽이 정지)
            }
        } else {
            both = 0;
            if (notBoth < 255) notBoth++;
            if (notBoth >= REV_CLEAR_SAMPLES) armed = true;    // 선을 '확실히' 벗어난 뒤에만 다음 선을 셈
        }
#if HOP_TIMEOUT_MS > 0
        if (now - cellStart > HOP_TIMEOUT_MS) uiFatal(F("후진 중 교차점을 찾지 못함 (HOP_TIMEOUT_MS)"), 4);
#endif
        if (spd < speed) {                                 // 부드럽게 출발 (팔레트 미끄럼 방지)
            spd += (int)(now - lastMs) * ACCEL_PER_MS;
            if (spd > speed) spd = speed;
        }
        lastMs = now;
        drive(-(int)((long)spd * REV_L_GAIN / 100), -(int)((long)spd * REV_R_GAIN / 100));
        uiCheckAbort();
    }
}

// ---- 출발: T자 뒤에서 출발했는지, T자와 첫 교차점 사이에서 출발했는지 스스로 구분 ----
//  출발 위치의 선: [T자] ─ 50mm ─ [바깥 행 교차점(H5)] ─ 200mm ─ [G5] ...
//  천천히 가며 첫 가로선을 본 뒤, gapMs 안에 가로선이 또 나오면 → 처음 것은 T자였고 지금 것이 교차점 (2 반환, 센서는 교차점 선 위)
//  gapMs 동안 안 나오면 → 처음 것이 교차점이었음 (1 반환, 센서는 교차점을 조금 지난 곳)
//  어느 쪽이든 돌아올 때 로봇은 '첫 교차점을 막 지난 상태로 달리는 중'이다. 멈추지 않는다.
uint8_t LineTraceStartLines(int speed, unsigned long gapMs)
{
    lineTraceReset();
    uint8_t both = 0, notBoth = 0, lines = 0;
    bool armed = true;
    unsigned long t0 = millis(), lastMs = t0, tLine = 0;
    int spd = 0;
    for (;;) {
        IrSample s = readIr();
        unsigned long now = millis();
        if (s.l > XING_TH_L && s.r > XING_TH_R) {
            notBoth = 0;
            if (both < 255) both++;
            if (armed && both >= XING_CONFIRM_SAMPLES) {
                armed = false;
                lines++;
                tLine = now;
                beepNode();
                if (lines == 2) break;                       // T자 + 교차점
            }
        } else {
            both = 0;
            if (notBoth < 255) notBoth++;
            if (notBoth >= XING_CLEAR_SAMPLES) armed = true;
        }
        if (lines == 1 && now - tLine > gapMs) break;        // 교차점 하나만 (T자 없이)
#if HOP_TIMEOUT_MS > 0
        if (lines == 0 && now - t0 > HOP_TIMEOUT_MS) uiFatal(F("출발 후 첫 가로선을 찾지 못함"), 6);
#endif
        if (spd < speed) {
            spd += (int)(now - lastMs) * ACCEL_PER_MS;
            if (spd > speed) spd = speed;
        }
        lastMs = now;
        lineTraceStep(s, spd);
        uiCheckAbort();
    }
    s_speed = speed;                                         // 이어서 달리는 LineTraceToCross 가 이 속도에서 시작
    lineTraceDisarm();                                       // 지금 밟고 있는 선을 다음 교차점으로 다시 세지 않게
    s_crossLast = millis();
    return lines;
}

// ---- T자(선 끝의 짧은 가로선)까지 전진해서 정지 ----
//  맵 바깥쪽: 각 열의 선이 H행(A행) 밖으로 약 50mm 더 나와 있고 끝에 짧은 가로선(T자)이 있다 = 출발·하역 위치.
//  바깥 행 교차점에 정렬·정지한 상태에서 호출: 선을 따라 천천히 가다 두 센서가 동시에 검정(T자)이면 그 순간 정지.
//  (센서가 아직 교차점 가로선 위에 있을 수 있으므로, 먼저 '양쪽 검정이 아님'을 본 뒤부터 T자를 찾는다)
//  maxMs 안에 못 찾으면 그 자리에 정지하고 false (맵 밖으로 나가지 않게)
bool LineTraceToBar(int speed, unsigned long maxMs)
{
    lineTraceReset();
    uint8_t both = 0, notBoth = 0;
    bool armed = false, found = false;
    unsigned long t0 = millis(), lastMs = t0;
    int spd = 0;
    for (;;) {
        IrSample s = readIr();
        if (s.l > XING_TH_L && s.r > XING_TH_R) {
            notBoth = 0;
            if (armed && ++both >= XING_CONFIRM_SAMPLES) { found = true; break; }
        } else {
            both = 0;
            if (notBoth < 255) notBoth++;
            if (notBoth >= XING_CLEAR_SAMPLES) armed = true;
        }
        unsigned long now = millis();
        if (now - t0 > maxMs) break;
        if (spd < speed) {
            spd += (int)(now - lastMs) * ACCEL_PER_MS;
            if (spd > speed) spd = speed;
        }
        lastMs = now;
        lineTraceStep(s, spd);
        uiCheckAbort();
    }
    Brake();
    lineTraceReset();
    if (found) beepNode();
    return found;
}

// T자 위에 센서가 걸쳐 있으면 조금 더 전진해서 완전히 지나간다 (이미 지나 있으면 움직이지 않음).
//  → 그 뒤 후진할 때 T자를 '반드시 한 번' 세게 되어 교차점 수가 틀리지 않는다
void ClearBarForward(unsigned long maxMs)
{
    unsigned long t0 = millis();
    uint8_t clear = 0;
    for (;;) {
        IrSample s = readIr();
        if (s.l > XING_TH_L && s.r > XING_TH_R) {
            clear = 0;
            Forward(SPEED_APPROACH);
        } else if (++clear >= XING_CLEAR_SAMPLES) {
            break;
        }
        if (millis() - t0 > maxMs) break;
        uiCheckAbort();
    }
    Brake();
}

// ---- 후진 뒤 교차점 다시 찾아 정렬 (조향 없이 똑바로) ----
//  후진하면 조금 비뚤어질 수 있다. 이때 선 따라 조향하면 짧은 거리에서 크게 꺾여 더 비뚤어지므로
//  똑바로 천천히 앞으로 가서 가로선(양쪽 검정)을 찾고 REV_ALIGN_MS 더 가서 정지.
//  남은 방향 오차는 바로 뒤에 오는 회전(센서로 새 선을 찾아 멈춤)이 바로잡는다.
void RecatchCrossStraight()
{
    uint8_t both = 0;
    unsigned long t0 = millis(), lastMs = t0;
    int spd = 0;
    for (;;) {
        IrSample s = readIr();
        if (s.l > XING_TH_L && s.r > XING_TH_R) {
            if (++both >= XING_CONFIRM_SAMPLES) break;
        } else {
            both = 0;
        }
        unsigned long now = millis();
#if HOP_TIMEOUT_MS > 0
        if (now - t0 > HOP_TIMEOUT_MS) uiFatal(F("후진 뒤 교차점을 다시 찾지 못함"), 5);
#endif
        if (spd < SPEED_APPROACH) {
            spd += (int)(now - lastMs) * ACCEL_PER_MS;
            if (spd > SPEED_APPROACH) spd = SPEED_APPROACH;
        }
        lastMs = now;
        Forward(spd);
        uiCheckAbort();
    }
    beepNode();
    Forward(SPEED_APPROACH);
    waitMs(REV_ALIGN_MS);
    Brake();
    lineTraceReset();
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
