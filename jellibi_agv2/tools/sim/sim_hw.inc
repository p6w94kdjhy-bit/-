// 가짜 하드웨어 + 물리 격자 세계. 각 시뮬레이션 TU에 한 번 include.
// 실제 Nav.cpp / Grid.cpp 는 그대로 링크된다.
#include <vector>
#include <string>
#include <stdexcept>
#include "AGV2.h"

bool g_serialEcho = false;
SerialStub Serial;
static unsigned long g_ms = 0;
unsigned long millis() { return g_ms++; }
void delay(unsigned long ms) { g_ms += ms; }

struct World {
    uint8_t cur = NODE_NONE, tgt = NODE_NONE, hdg = HN;
    bool midEdge = false;            // 장애물 앞에서 멈춤 (칸 중간)
    bool blocked[64] = {};
    bool carrying = false;
    bool atBar = false;              // 센서가 선 끝 T자 바깥에 있음
    bool (*edgeAllowed)(uint8_t, uint8_t) = nullptr;
    int hops = 0, turns = 0, uturns = 0;
    std::vector<std::string> log;
    std::string fail;
} W;

static std::string nodeStr(uint8_t n)
{
    if (n == NODE_NONE) return "--";
    return std::string(1, 'A' + rowOf(n)) + std::to_string(colOf(n) + 1);
}
static void failNow(const std::string& m)
{
    if (W.fail.empty()) W.fail = m;
    throw std::runtime_error(m);
}
void worldStart(uint8_t start, uint8_t h)
{
    W.cur = stepNode(start, reverseH(h)); W.tgt = start; W.hdg = h;
    W.midEdge = false; W.carrying = false; W.hops = W.turns = W.uturns = 0;
    W.atBar = false;
    memset(W.blocked, 0, sizeof(W.blocked));
    W.log.clear(); W.fail.clear();
}

// ---- Drive ----
void driveInit() {}
void drive(int, int) {}
void Forward(int) {}
void Backward(int) {}
void Stop() {}
void spinDrive(int, int) {}
void Brake() {}
void spinTurn(bool left, uint8_t q)
{
    if (q == 2) {
        W.uturns++; W.log.push_back("U");
        if (W.midEdge) {                        // 칸 중간 U턴: 방금 떠난 교차점으로
            uint8_t back = W.cur;
            W.cur = W.tgt; W.tgt = back; W.hdg = reverseH(W.hdg); W.midEdge = false;
            return;
        }
        W.hdg = reverseH(W.hdg);
    } else {
        W.turns++; W.log.push_back(left ? "L" : "R");
        W.hdg = (W.hdg + (left ? 3 : 1)) & 3;
    }
    W.tgt = stepNode(W.cur, W.hdg);
}
void TurnLeft90()  { spinTurn(true, 1); }
void TurnRight90() { spinTurn(false, 1); }
void UTurn()       { spinTurn(UTURN_LEFT, 2); }
void UTurnOnEdge() { spinTurn(UTURN_LEFT, 2); }
void TurnAdjust(bool, int) {}
IrSample readIr() { return IrSample{0, 0}; }
int irFront() { return (W.tgt != NODE_NONE && W.blocked[W.tgt]) ? 650 : 70; }   // 바로 앞 교차점에 장애물이 있으면 큰 값
bool irCalibrated() { return true; }
void irUseCarry(bool) {}
bool irCarryCalibrated() { return false; }
int irXingThL() { return 0; }
int irXingThR() { return 0; }
int irCenterOffset() { return 0; }
bool frontCalibrated() { return true; }
bool isBlackL(int) { return false; }
bool isBlackR(int) { return false; }
int irNormL(int) { return 0; }
int irNormR(int) { return 0; }
int lineError(const IrSample&) { return 0; }
void lineTraceReset() {}
void lineTraceSetGains(int, int, int, int) {}
void lineTraceSmooth(int, int, int, int) {}
void lineTraceDisarm() {}
void lineTraceBoost(int, unsigned long) {}
uint16_t lineTraceTakeMaxErr() { return 0; }
void runlogBegin() {}
void runlogAdd(uint8_t, uint8_t, uint8_t) {}
void runlogPump() {}
void runlogFlush() {}
void runlogDump() {}
void lineTraceStep(const IrSample&, int) {}

void AlignOnCross() { lineTraceReset(); }

// 후진: 마지막으로 센 교차점(센서가 그 선 앞에 있음)부터 뒤쪽으로 count 개를 셈.
// 끝나면 센서는 마지막으로 센 교차점 바로 뒤 → 다음 전진(LineTraceToCross)에서 그 교차점을 다시 셈
uint8_t LineTraceStartLines(int, unsigned long) { LineTraceToCross(0, false); return 1; }
bool LineTraceToBar(int, unsigned long) { W.atBar = true; W.log.push_back("T"); return true; }
void ClearBarForward(unsigned long) {}

bool LineTraceBackCross(uint8_t count, int)
{
    if (W.atBar) {                               // T자를 먼저 한 번 셈
        if (count == 0) failNow("후진 수 부족");
        W.atBar = false; count--; W.log.push_back("<T");
    }
    uint8_t back = reverseH(W.hdg);
    uint8_t n = W.cur;
    for (uint8_t i = 0; i < count; i++) {
        if (n == NODE_NONE) failNow("후진 중 맵 밖");
        W.log.push_back("<" + nodeStr(n));
        W.tgt = n;
        uint8_t m = stepNode(n, back);
        if (i + 1 < count && W.edgeAllowed && !W.edgeAllowed(m, n)) failNow("후진 금지 통로 " + nodeStr(n) + "-" + nodeStr(m));
        if (i + 1 < count && W.blocked[m]) failNow("후진 중 장애물 " + nodeStr(m));
        n = m;
    }
    W.cur = n; W.midEdge = false; W.hops += count;
    return true;
}
void RecatchCrossStraight()
{
    if (!LineTraceToCross(0, false)) failNow("recatch");
}

// 라인트레이싱: 다음 교차점으로 한 칸 이동. 장애물 감시 중이면 앞 칸에 장애물이 있을 때 멈춤
bool LineTraceToCross(int, bool checkObstacle)
{
    W.atBar = false;
    bool canSee = !(W.carrying && !FRONT_CHECK_WHILE_CARRY);   // 팔레트에 가려 못 봄
    if (checkObstacle && canSee && W.tgt != NODE_NONE && W.blocked[W.tgt]) {
        W.midEdge = true;
        W.log.push_back("!obs");
        return false;
    }
    if (W.tgt == NODE_NONE) failNow("맵 밖으로 이탈 (from " + nodeStr(W.cur) + ")");
    if (W.blocked[W.tgt]) failNow("장애물 충돌/밀기 at " + nodeStr(W.tgt));
    if (W.cur != NODE_NONE && W.edgeAllowed && !W.edgeAllowed(W.cur, W.tgt))
        failNow("금지 통로 " + nodeStr(W.cur) + "-" + nodeStr(W.tgt));
    W.cur = W.tgt; W.tgt = stepNode(W.cur, W.hdg); W.midEdge = false;
    W.hops++; W.log.push_back(nodeStr(W.cur));
    return true;
}

// ---- Lift ----
void liftInit() {}
void LiftUp()   { W.carrying = true;  W.log.push_back("PICK@" + nodeStr(W.cur) + "/" + "NESW"[W.hdg]); }
void LiftDown() { W.carrying = false; W.log.push_back("DROP@" + nodeStr(W.cur) + "/" + "NESW"[W.hdg]); }
bool liftIsUp() { return W.carrying; }
void LiftGoto(int) {}

// ---- Ui ----
void uiInit() {}
bool buttonDown() { return false; }
void uiWaitStart() {}
void beep(unsigned int, unsigned int) {}
void beepReady() {}
void beepNode() {}
void beepDone() {}
void uiFatal(const __FlashStringHelper* m, uint8_t) { failNow(std::string("uiFatal: ") + reinterpret_cast<const char*>(m)); }
void uiArmAbort() {}
void showConfig() {}
void uiCheckAbort() {}
void waitMs(unsigned long ms)
{
    if (ms >= 1500) W.log.push_back("HOLD@" + nodeStr(W.cur) + "/" + "NESW"[W.hdg]);
    g_ms += ms;
}

static std::string joinLog()
{
    std::string s;
    for (auto& t : W.log) { if (!s.empty()) s += ' '; s += t; }
    return s;
}
