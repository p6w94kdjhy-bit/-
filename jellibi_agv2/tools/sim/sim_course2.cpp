#include "config.h"
#include "sim_hw.inc"
#define setup sketch_setup
#define loop sketch_loop
#include "../../course2_pallet/course2_pallet.ino"
#undef setup
#undef loop

static bool blueEdge(uint8_t a, uint8_t b) { return plannerHasEdge(a, b); }

int main()
{
    // 대략 시간: 한 칸 0.9s, 90° 0.7s, U턴 1.2s, 리프트 1.5s, 정차 2.3s (추정치)
    const uint8_t open[3] = { P('D', 2), P('D', 4), P('D', 6) };
    int fails = 0;
    printf("FRONT_CHECK_WHILE_CARRY=%d\n", FRONT_CHECK_WHILE_CARRY);
    for (int k = 0; k < 3; k++) {
        worldStart(START_NODE, START_HDG);
        W.edgeAllowed = blueEdge;
        for (int j = 0; j < 3; j++) if (j != k) W.blocked[open[j]] = true;
        try { sketch_setup(); } catch (std::exception&) {}
        std::string got = joinLog();
        // 검증: 픽업·하역 순서와 위치, 마지막 H4 남향 정차
        std::vector<std::string> ev;
        for (auto& t : W.log) if (t.rfind("PICK", 0) == 0 || t.rfind("DROP", 0) == 0 || t.rfind("HOLD", 0) == 0) ev.push_back(t);
        std::string evs; for (auto& t : ev) evs += t + " ";
        bool ok = W.fail.empty() &&
                  (evs == "PICK@A2/N DROP@H6/S PICK@A6/N DROP@H2/S HOLD@H4/S " ||
                   evs == "PICK@A6/N DROP@H2/S PICK@A2/N DROP@H6/S HOLD@H4/S ");
        double est = W.hops * 0.9 + W.turns * 0.7 + W.uturns * 1.2 + 4 * 1.5 + 2.3 + 2 * 0.8;
        printf("open=%s  hops=%d turns=%d uturns=%d  est~%.0fs  %s %s\n",
               nodeStr(open[k]).c_str(), W.hops, W.turns, W.uturns, est,
               ok ? "OK" : "FAIL", W.fail.c_str());
        printf("  %s\n", got.c_str());
        if (!ok) { fails++; printf("  events: %s\n", evs.c_str()); }
    }
    return fails ? 1 : 0;
}
