// =====================================================================
//  물리 시뮬레이션 — 실제 주행 코드(Motor / Sensor / LineTrace / Nav / RunLog)를 그대로 컴파일해서
//  가짜 로봇(2륜 차동구동 + 바닥 IR 2개)과 10mm 격자 선 위에서 달려 본다.
//
//  목적: 라인트레이싱 방식끼리 비교(흔들림·정차 위치·시간), 늦은 감속·주행 기록 동작 확인.
//  한계: 로봇 치수·모터 반응은 추정값이다. 숫자 자체가 아니라 '방식 간 차이'와 '코드가 깨지지 않는지'를 본다.
//        그래서 로봇 특성을 여러 가지로 바꿔 가며(PLANTS) 전부에서 문제없는지 확인한다.
//  회전은 실제 코드를 쓰지 않고 '제자리에서 돌고 ±오차' 로 대신한다 (회전 보정값은 실물에서만 맞출 수 있음).
// =====================================================================
#include <vector>
#include <random>
#include <stdexcept>
#include "AGV2.h"
#include "EEPROM.h"
#include "avr/eeprom.h"

bool g_serialEcho = false;
SerialStub Serial;
EEPROMStub EEPROM;

// ------------------------------------------------------------------ 로봇 특성 (추정)
struct Plant {
    const char* name;
    double W;      // 바퀴 사이 거리 mm
    double L;      // 바퀴축 → 바닥 IR 거리 mm
    double S;      // 좌우 IR 사이 거리 mm
    double R;      // IR 이 보는 원의 반지름 mm
    double K;      // 속도 mm/s per PWM
    double DB;     // 이 PWM 이하는 안 움직임
    double tau;    // 모터 반응 지연 s
};
static Plant PL;

// ------------------------------------------------------------------ 세계 상태
static uint64_t g_us = 0;
static double X, Y, PHI;             // 바퀴축 중점 mm, 방향 rad (0=북, +시계방향). y 는 남쪽이 +
static double VL, VR;                // 바퀴 속도 mm/s
static int g_dirL = LEFT_FWD_LEVEL, g_dirR = RIGHT_FWD_LEVEL, g_magL = 0, g_magR = 0;
static std::mt19937 g_rng(1);
static uint64_t g_eeBusyUntil = 0;
static uint8_t g_ee[1024];

// 주행 품질 측정
static double g_nomPhi = 0;          // 지금 따라가야 할 선의 방향
static struct {
    double dist, sumHerr2, maxHerr, maxLat, sumYaw2, t;
    int reversals, lastSign;
    void clear() { *this = {}; }
} M;

static double wrapPi(double a) { while (a > M_PI) a -= 2 * M_PI; while (a < -M_PI) a += 2 * M_PI; return a; }

static bool onLine(double px, double py)
{
    const double lo = -150, hi = 1550, hw = 5.0;      // 선 폭 10mm, 격자 200mm × 8
    if (py >= lo && py <= hi) {
        double c = std::round(px / 200.0);
        if (c >= 0 && c <= 7 && std::fabs(px - c * 200.0) <= hw) return true;
    }
    if (px >= lo && px <= hi) {
        double r = std::round(py / 200.0);
        if (r >= 0 && r <= 7 && std::fabs(py - r * 200.0) <= hw) return true;
    }
    return false;
}

// 바닥 IR 한 개가 보는 원 안의 검정 비율
static double blackFrac(bool left)
{
    double fx = std::sin(PHI), fy = -std::cos(PHI);          // 앞
    double lx = -std::cos(PHI), ly = -std::sin(PHI);         // 왼쪽
    double side = left ? PL.S / 2 : -PL.S / 2;
    double cx = X + fx * PL.L + lx * side, cy = Y + fy * PL.L + ly * side;
    int n = 0, b = 0;
    static const double rr[3] = { 0.0, 0.5, 0.9 };
    static const int cnt[3] = { 1, 6, 10 };
    for (int k = 0; k < 3; k++)
        for (int i = 0; i < cnt[k]; i++) {
            double a = 2 * M_PI * i / cnt[k] + k;
            n++;
            if (onLine(cx + PL.R * rr[k] * std::cos(a), cy + PL.R * rr[k] * std::sin(a))) b++;
        }
    return (double)b / n;
}

static void physStep(double dt)
{
    int cl = (g_dirL == LEFT_FWD_LEVEL ? 1 : -1) * g_magL;
    int cr = (g_dirR == RIGHT_FWD_LEVEL ? 1 : -1) * g_magR;
    auto target = [](int pwm, double boost) {
        double a = std::abs(pwm) - PL.DB;
        if (a <= 0) return 0.0;
        return (pwm > 0 ? 1 : -1) * a * PL.K * boost;
    };
    double tl = target(cl, 1.0 / 0.94);                       // 왼쪽 모터가 조금 셈 (MOTOR_L_GAIN 94 의 원인)
    double tr = target(cr, 1.0);
    VL += (tl - VL) * dt / PL.tau;
    VR += (tr - VR) * dt / PL.tau;
    double v = (VL + VR) / 2, w = (VL - VR) / PL.W;
    PHI += w * dt;
    X += v * std::sin(PHI) * dt;
    Y -= v * std::cos(PHI) * dt;

    if (std::fabs(v) > 40) {                                  // 달리는 동안만 측정
        double herr = wrapPi(PHI - g_nomPhi) * 180 / M_PI;
        int axis = (int)std::lround(g_nomPhi / (M_PI / 2)) & 1;   // 0 = 남북, 1 = 동서
        double p = axis ? Y : X;
        double lat = p - std::round(p / 200.0) * 200.0;
        M.dist += std::fabs(v) * dt;
        M.t += dt;
        M.sumHerr2 += herr * herr * dt;
        M.maxHerr = std::max(M.maxHerr, std::fabs(herr));
        M.maxLat = std::max(M.maxLat, std::fabs(lat));
        double yaw = w * 180 / M_PI;
        M.sumYaw2 += yaw * yaw * dt;
        int sg = yaw > 4 ? 1 : (yaw < -4 ? -1 : 0);
        if (sg && sg != M.lastSign) { if (M.lastSign) M.reversals++; M.lastSign = sg; }
    }
}

static void advance(uint64_t us)
{
    while (us) {
        uint64_t s = us > 100 ? 100 : us;
        physStep(s * 1e-6);
        g_us += s;
        us -= s;
    }
}

// ------------------------------------------------------------------ Arduino 함수
unsigned long millis() { advance(2); return (unsigned long)(g_us / 1000); }
unsigned long micros() { advance(2); return (unsigned long)g_us; }
void delay(unsigned long ms) { advance((uint64_t)ms * 1000); }
void digitalWrite(int pin, int v)
{
    if (pin == PIN_L_DIR) g_dirL = v;
    if (pin == PIN_R_DIR) g_dirR = v;
}
int digitalRead(int) { advance(2); return HIGH; }             // 버튼 안 눌림
void analogWrite(int pin, int v)
{
    if (pin == PIN_L_PWM) g_magL = v;
    if (pin == PIN_R_PWM) g_magR = v;
}
int analogRead(int pin)
{
    advance(112);                                             // ADC 변환 시간
    std::uniform_int_distribution<int> nz(-3, 3);
    if (pin == PIN_IR_L) return (int)(IR_L_WHITE + blackFrac(true) * (IR_L_BLACK - IR_L_WHITE)) + nz(g_rng);
    if (pin == PIN_IR_R) return (int)(IR_R_WHITE + blackFrac(false) * (IR_R_BLACK - IR_R_WHITE)) + nz(g_rng);
    return 1000;                                              // 전방 IR: 장애물 없음
}
uint8_t EEPROMStub::read(int a) { while (g_us < g_eeBusyUntil) advance(50); return g_ee[a]; }
void EEPROMStub::write(int a, uint8_t v) { while (g_us < g_eeBusyUntil) advance(50); g_ee[a] = v; g_eeBusyUntil = g_us + 3400; }
bool eeprom_is_ready() { advance(2); return g_us >= g_eeBusyUntil; }

// ------------------------------------------------------------------ 회전·알림 (대역)
static double g_turnErrDeg = 0;
static void idealTurn(double deg)
{
    Stop();
    advance(60000);
    g_nomPhi = wrapPi(g_nomPhi + deg * M_PI / 180);
    std::uniform_real_distribution<double> e(-g_turnErrDeg, g_turnErrDeg);
    PHI = g_nomPhi + e(g_rng) * M_PI / 180;
    VL = VR = 0;
    advance((uint64_t)(std::fabs(deg) / 90.0 * TURN90_MS * 1000));
    lineTraceReset();
}
void TurnLeft90()  { idealTurn(-90); }
void TurnRight90() { idealTurn(90); }
void UTurn()       { idealTurn(180); }
void UTurnOnEdge() { idealTurn(180); }
void TurnAdjust(bool, int) {}
void spinTurn(bool, uint8_t) {}

struct Fatal : std::runtime_error { using std::runtime_error::runtime_error; };
void uiInit() {}
void beep(unsigned int, unsigned int ms) { delay(ms); }
void beepReady() {}
void beepNode() { if (getenv("TRACE2")) printf("    count at %.0f,%.0f hdg %.1f  v=%.0f\n", X, Y, PHI * 180 / M_PI, (VL + VR) / 2); }
void beepDone() {}
void uiFatal(const __FlashStringHelper* m, uint8_t code)
{
    Stop();
    runlogAdd(RL_FATAL, code);
    runlogFlush();
    throw Fatal(reinterpret_cast<const char*>(m));
}
void uiArmAbort() {}
void uiCheckAbort() { runlogPump(); advance(2); }
void waitMs(unsigned long ms) { unsigned long t0 = millis(); while (millis() - t0 < ms) uiCheckAbort(); }
void showConfig() {}

// ------------------------------------------------------------------ 시험
struct Ctl { const char* name; int smooth; int a, b, c, d; };

static void applyCtl(const Ctl& c)
{
    if (c.smooth) lineTraceSmooth(c.a, c.b, c.c, c.d);
    else { lineTraceSmooth(0, 0, 100, 0); lineTraceSetGains(c.a, c.b, c.c, c.d); }
}

static void resetWorld(double x, double y, double phiDeg, unsigned seed)
{
    g_us = 1000000; g_eeBusyUntil = 0;
    X = x; Y = y; PHI = phiDeg * M_PI / 180; VL = VR = 0;
    g_magL = g_magR = 0;
    g_nomPhi = 0;
    g_rng.seed(seed);
    M.clear();
}

struct StopStat { int n = 0, bad = 0; double sumLon = 0, sumLon2 = 0, maxLon = 0, maxLat = 0, maxH = 0; };

// 정지한 뒤: 목표 교차점 기준 앞뒤(lon)·좌우(lat) 오차 mm
static void noteStop(StopStat& st, uint8_t node)
{
    advance(300000);
    double tx = colOf(node) * 200.0, ty = rowOf(node) * 200.0;
    double fx = std::sin(g_nomPhi), fy = -std::cos(g_nomPhi);
    double lon = (X - tx) * fx + (Y - ty) * fy;
    double lat = -(X - tx) * fy + (Y - ty) * fx;
    double h = wrapPi(PHI - g_nomPhi) * 180 / M_PI;
    st.n++;
    st.sumLon += lon; st.sumLon2 += lon * lon;
    st.maxLon = std::max(st.maxLon, std::fabs(lon));
    st.maxLat = std::max(st.maxLat, std::fabs(lat));
    st.maxH = std::max(st.maxH, std::fabs(h));
    if (std::fabs(lon) > 30 || std::fabs(lat) > 30) st.bad++;
    if (std::fabs(lon) > 80 || std::fabs(lat) > 80) throw Fatal("교차점을 잘못 셈 / 선 이탈");
}

// ALIGN_MS 는 실물에서 '바퀴축이 교차점 위에 오게' 맞춘 값이다. 가짜 로봇은 치수가 달라 같은 값이면
// 일정하게 앞/뒤에 선다 → 그 일정한 차이(g_stopBias)를 미리 재서 정지할 때마다 빼 준다 (= 현장 보정을 흉내).
static double g_stopBias = 0;
static void applyStopBias()
{
    advance(200000);
    X -= g_stopBias * std::sin(g_nomPhi);
    Y += g_stopBias * std::cos(g_nomPhi);
}

struct Row {
    int runs = 0, fails = 0;
    double time = 0, rmsH = 0, maxLat = 0, rev = 0;
    StopStat st;
    std::string firstFail;
};

// 격자 순회: 직선 길이·좌우 회전·U턴이 섞인 경로. 회전마다 ±turnErr° 의 방향 오차
static const uint8_t TOUR[] = {
    P('E', 4), P('E', 7), P('B', 7), P('B', 2), P('G', 2), P('G', 6), P('C', 6), P('C', 3),
    P('F', 3), P('F', 5), P('E', 5), P('E', 4), P('H', 4), P('G', 4), P('G', 3), P('A', 3),
    P('A', 8), P('H', 8), P('H', 1), P('A', 1), P('B', 1), P('B', 2), P('C', 2), P('H', 2),
};

static void runTour(const Ctl& c, int cruise, double turnErr, double startPhi, double startDx, unsigned seed, Row& row,
                    bool log = false)
{
    resetWorld(600 + startDx, 1400 + 80, startPhi, seed);
    g_turnErrDeg = turnErr;
    applyCtl(c);
    Nav nav;
    nav.cruise = cruise;
    nav.setApproach(P('H', 4), HN);
    if (log) runlogBegin();
    row.runs++;
    uint64_t t0 = g_us;
    try {
        for (uint8_t t : TOUR) {
            nav.leg(t, true);
            applyStopBias();
            noteStop(row.st, t);
            if (getenv("TRACE")) {
                printf("  leg -> "); g_serialEcho = true; printNode(t); g_serialEcho = false;
                printf("  pos %.0f,%.0f  hdg %.1f (nom %.0f)\n", X, Y, PHI * 180 / M_PI, g_nomPhi * 180 / M_PI);
            }
        }
        if (log) { runlogAdd(RL_DONE); runlogFlush(); }
    } catch (Fatal& f) {
        row.fails++;
        if (row.firstFail.empty()) {
            char buf[160];
            snprintf(buf, sizeof buf, "%s (seed %u, 위치 %.0f,%.0f)", f.what(), seed, X, Y);
            row.firstFail = buf;
        }
        return;
    }
    row.time += (g_us - t0) * 1e-6;
    row.rmsH += std::sqrt(M.sumHerr2 / M.t);
    row.maxLat = std::max(row.maxLat, M.maxLat);
    row.rev += M.reversals / (M.dist / 1000.0);
}

static void calibrateStopBias(const Ctl& c)
{
    g_stopBias = 0;
    resetWorld(600, 1400 + 80, 0, 99);
    g_turnErrDeg = 0;
    applyCtl(c);
    Nav nav;
    nav.setApproach(P('H', 4), HN);
    try { nav.leg(P('F', 4), true); } catch (Fatal&) { return; }
    advance(300000);
    g_stopBias = -(Y - 1000.0);          // 북쪽으로 달림: 앞으로 지나친 만큼 +
}

static const Plant PLANTS[] = {
    //  이름           W    L   S    R    K    DB   tau
    { "기준",        105, 22, 22, 3.5, 3.0, 25, 0.07 },
    { "반응 느림",    105, 22, 22, 3.5, 3.0, 25, 0.13 },
    { "반응 빠름",    105, 22, 22, 3.5, 3.0, 25, 0.04 },
    { "센서 앞쪽",    105, 35, 22, 3.5, 3.0, 25, 0.07 },
    { "센서 뒤쪽",    105, 15, 22, 3.5, 3.0, 25, 0.07 },
    { "센서 넓음",    105, 22, 28, 3.5, 3.0, 25, 0.07 },
    { "센서 좁음",    105, 22, 19, 3.0, 3.0, 25, 0.07 },
    { "빠른 로봇",    105, 22, 22, 3.5, 3.9, 20, 0.09 },
    { "차폭 넓음",    130, 25, 22, 3.5, 3.0, 25, 0.07 },
};

int main(int argc, char** argv)
{
    if (getenv("ONE")) {                                   // 디버그: ONE="로봇번호 방식번호 seed" 한 번만
        int pi, ci; unsigned seed;
        sscanf(getenv("ONE"), "%d %d %u", &pi, &ci, &seed);
        const Ctl C[] = { { "p", 0, LT_KP, LT_KD, 100, 0 }, { "v2", 0, V2_LT_KP, V2_LT_KD, V2_LT_MAX_PCT, V2_LT_DEADBAND },
                          { "pd", 1, C2_LT_KP, C2_LT_TD_MS, C2_LT_MAX_PCT, C2_LT_SLOW_PCT } };
        PL = PLANTS[pi];
        Row row;
        g_serialEcho = getenv("ECHO") != nullptr;
        runTour(C[ci], argc > 1 ? atoi(argv[1]) : SPEED_CRUISE, argc > 2 ? atof(argv[2]) : 8,
                ((int)seed % 5 - 2) * 4.0, ((int)seed % 3 - 1) * 4.0, seed, row);
        printf("fail=%d %s\n", row.fails, row.firstFail.c_str());
        return 0;
    }
    int cruise = argc > 1 ? atoi(argv[1]) : SPEED_CRUISE;
    double turnErr = argc > 2 ? atof(argv[2]) : 8;
    if (argc > 3) {                                         // 주행 기록(EEPROM) 확인: 한 번 달리고 저장된 내용 출력
        const Ctl c = { "pd", 1, C2_LT_KP, C2_LT_TD_MS, C2_LT_MAX_PCT, C2_LT_SLOW_PCT };
        PL = PLANTS[0];
        memset(g_ee, 0xFF, sizeof g_ee);
        calibrateStopBias(c);
        Row row;
        runTour(c, cruise, turnErr, 4, 0, 3, row, true);
        g_serialEcho = true;
        runlogDump();
        return row.fails;
    }
    int pd[4] = { C2_LT_KP, C2_LT_TD_MS, C2_LT_MAX_PCT, C2_LT_SLOW_PCT };
    if (getenv("PD")) sscanf(getenv("PD"), "%d,%d,%d,%d", &pd[0], &pd[1], &pd[2], &pd[3]);   // 값 바꿔 보기
    const Ctl CTLS[] = {
        { "종목① 비례 KP70",        0, LT_KP, LT_KD, 100, 0 },
        { "v2 KP35/KD15",          0, V2_LT_KP, V2_LT_KD, V2_LT_MAX_PCT, V2_LT_DEADBAND },
        { "부드러운 PD (종목②)",    1, pd[0], pd[1], pd[2], pd[3] },
    };
    if (getenv("SWEEP")) {                                  // 한 줄 요약 (값 탐색용)
        Row all; double worstH = 0, worstRev = 0, worstStop = 0; int worstBad = 0;
        for (const Plant& p : PLANTS) {
            PL = p;
            Row row;
            calibrateStopBias(CTLS[2]);
            for (unsigned seed = 1; seed <= 8; seed++)
                runTour(CTLS[2], cruise, turnErr, ((int)seed % 5 - 2) * 4.0, ((int)seed % 3 - 1) * 4.0, seed, row);
            int ok = row.runs - row.fails;
            all.fails += row.fails; all.time += ok ? row.time / ok : 0; all.maxLat = std::max(all.maxLat, row.maxLat);
            worstH = std::max(worstH, ok ? row.rmsH / ok : 99); worstRev = std::max(worstRev, ok ? row.rev / ok : 99);
            worstStop = std::max(worstStop, row.st.maxLon); worstBad += row.st.bad;
        }
        printf("PD %3d,%3d,%3d,%3d  실패 %2d  평균시간 %.1f  흔들림(최악) %.2f°  꺾임/m(최악) %.1f  최대벗어남 %.1f  정차 최대오차 %.1f  ±30밖 %d\n",
               pd[0], pd[1], pd[2], pd[3], all.fails, all.time / (sizeof(PLANTS) / sizeof(PLANTS[0])), worstH, worstRev,
               all.maxLat, worstStop, worstBad);
        return 0;
    }
    printf("통과 속도 PWM %d, 회전 오차 ±%.0f°, LATE_BRAKE_PCT %d — 격자 순회 %d구간 × 8회\n",
           cruise, turnErr, LATE_BRAKE_PCT, (int)(sizeof(TOUR)));
    printf("%-12s %-22s %5s %7s %8s %8s %8s %14s %8s\n", "로봇", "방식", "실패", "시간s", "흔들림°", "최대벗어남", "꺾임/m",
           "정차오차mm", "±30밖");
    int totalFail[3] = { 0, 0, 0 };
    for (const Plant& p : PLANTS) {
        PL = p;
        for (int ci = 0; ci < 3; ci++) {
            Row row;
            calibrateStopBias(CTLS[ci]);
            for (unsigned seed = 1; seed <= 8; seed++) {
                double sphi = ((int)seed % 5 - 2) * 4.0;          // 출발 방향 오차 -8 ~ +8°
                double sdx = ((int)seed % 3 - 1) * 4.0;           // 출발 좌우 오차 -4 ~ +4 mm
                runTour(CTLS[ci], cruise, turnErr, sphi, sdx, seed, row);
            }
            int ok = row.runs - row.fails;
            double mean = row.st.n ? row.st.sumLon / row.st.n : 0;
            double sd = row.st.n ? std::sqrt(std::max(0.0, row.st.sumLon2 / row.st.n - mean * mean)) : 0;
            printf("%-12s %-22s %2d/%-2d %7.1f %8.2f %8.1f %8.1f %+6.1f ±%-5.1f %4d/%-3d %s\n", p.name, CTLS[ci].name,
                   row.fails, row.runs, ok ? row.time / ok : 0, ok ? row.rmsH / ok : 0, row.maxLat, ok ? row.rev / ok : 0,
                   mean, sd, row.st.bad, row.st.n, row.firstFail.c_str());
            totalFail[ci] += row.fails;
        }
    }
    printf("실패 합계: 종목① 방식 %d, v2 %d, 부드러운 PD %d\n", totalFail[0], totalFail[1], totalFail[2]);

    return totalFail[2] ? 1 : 0;
}
