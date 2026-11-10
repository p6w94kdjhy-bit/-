#include "config.h"
#ifdef SIM_SXP
#undef START_XING_PASSED
#define START_XING_PASSED SIM_SXP
#endif
#include "sim_hw.inc"
#define setup sketch_setup
#define loop sketch_loop
#include "../../course1_drive/course1_drive.ino"
#undef setup
#undef loop

int main()
{
    const char* expect = START_XING_PASSED ?
        "G2 F2 R F3 F4 F5 L E5 D5 C5 HOLD@C5/N R C6 C7 L B7 U C7 D7 E7 F7 R F6 L G6 H6 HOLD@H6/S" :
        "H2 G2 F2 R F3 F4 F5 L E5 D5 C5 HOLD@C5/N R C6 C7 L B7 U C7 D7 E7 F7 R F6 L G6 H6 HOLD@H6/S";
    worldStart(P('H', 2), HN);
    if (START_XING_PASSED) { W.cur = P('H', 2); W.tgt = P('G', 2); }   // 센서가 이미 H2 선을 지남
    printf("START_XING_PASSED=%d\n", START_XING_PASSED);
    try { sketch_setup(); } catch (std::exception&) {}
    std::string got = joinLog();
    printf("course1: %s\n", got.c_str());
    printf("  hops=%d turns=%d uturns=%d\n", W.hops, W.turns, W.uturns);
    bool ok = W.fail.empty() && got == expect;
    if (!ok) printf("  FAIL %s\n  expect: %s\n", W.fail.c_str(), expect);
    else printf("  OK\n");
    return ok ? 0 : 1;
}
