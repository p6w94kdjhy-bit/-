// course2_drive 경로 시뮬레이션: 막힌 통로 3가지 경우 × 출발 위치 2가지
#include "config.h"
#include "sim_hw.inc"
#define setup sketch_setup
#define loop sketch_loop
#include "../../course2_drive/course2_drive.ino"
#undef setup
#undef loop

// 파란 통로 (스케치 주석의 가정과 같음): 2열 A~H, 6열 A~H, 4열 B~H, B행 2~6, G행 2~6
static bool blueEdge(uint8_t a, uint8_t b)
{
    if (a == NODE_NONE || b == NODE_NONE) return false;
    uint8_t ra = rowOf(a), rb = rowOf(b), ca = colOf(a), cb = colOf(b);
    if (ca == cb) {
        if (ca == 1 || ca == 5) return true;
        if (ca == 3) return ra >= 1 && rb >= 1;
        return false;
    }
    if (ra == rb && (ra == 1 || ra == 6)) return ca >= 1 && ca <= 5 && cb >= 1 && cb <= 5;
    return false;
}

int main()
{
    const uint8_t cand[3] = { P('D', 2), P('D', 4), P('D', 6) };
    int fails = 0;
    for (int k = 0; k < 3; k++) {
        worldStart(START_NODE, START_HDG);
        W.edgeAllowed = blueEdge;
        for (int j = 0; j < 3; j++) if (j != k) W.blocked[cand[j]] = true;
        for (int i = 0; i < 3; i++) lane[i] = L_UNKNOWN;
        openLane = -1;
        nav = Nav();
        try { sketch_setup(); } catch (std::exception&) {}
        std::string ev;
        for (auto& t : W.log) if (t.rfind("PICK", 0) == 0 || t.rfind("DROP", 0) == 0 || t.rfind("HOLD", 0) == 0) ev += t + " ";
        bool ok = W.fail.empty() &&
                  (ev == "PICK@A2/N DROP@H6/S PICK@A6/N DROP@H2/S HOLD@H4/S " ||
                   ev == "PICK@A6/N DROP@H2/S PICK@A2/N DROP@H6/S HOLD@H4/S ");
        double est = W.hops * 0.9 + W.turns * 0.7 + W.uturns * 1.2 + 4 * 1.5 + 2.3;
        printf("open=%s  hops=%d turns=%d uturns=%d  est~%.0fs  %s %s\n", nodeStr(cand[k]).c_str(), W.hops, W.turns,
               W.uturns, est, ok ? "OK" : "FAIL", W.fail.c_str());
        printf("  %s\n", joinLog().c_str());
        if (!ok) { fails++; printf("  events: %s\n", ev.c_str()); }
    }
    return fails ? 1 : 0;
}
