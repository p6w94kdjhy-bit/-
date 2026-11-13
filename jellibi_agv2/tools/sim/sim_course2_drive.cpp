// course2_drive 경로 시뮬레이션 (11-13 맵): D1~D3 중 1개 × D7~D8 중 1개 = 6가지 + 서쪽 2개가 막힌 경우
#include "config.h"
#include "sim_hw.inc"
#define setup sketch_setup
#define loop sketch_loop
#include "../../course2_drive/course2_drive.ino"
#undef setup
#undef loop

// 모든 격자 선 사용 가능. 단 고정 가림막(D4~D6)에 닿는 칸은 없음
static bool isBarrier(uint8_t n) { return rowOf(n) == 3 && colOf(n) >= 3 && colOf(n) <= 5; }
static bool gridEdge(uint8_t a, uint8_t b)
{
    if (a == NODE_NONE || b == NODE_NONE) return false;
    return !isBarrier(a) && !isBarrier(b);
}

// 기록을 따라가며 적재물과 부딪히지 않았는지 확인: 적재물이 놓인 교차점은 '들러 갈 때'만 들어갈 수 있다
static std::string checkPallets()
{
    std::vector<std::string> pal = { "A6", "A4" };
    bool carrying = false;
    std::string here;                                                    // 방금 하역한 교차점 (떠나기 전까지)
    auto has = [&](const std::string& n) { return std::find(pal.begin(), pal.end(), n) != pal.end(); };
    for (size_t i = 0; i < W.log.size(); i++) {
        const std::string& t = W.log[i];
        if (t.rfind("PICK@", 0) == 0) { std::string n = t.substr(5, 2); pal.erase(std::remove(pal.begin(), pal.end(), n), pal.end()); carrying = true; continue; }
        if (t.rfind("DROP@", 0) == 0) { pal.push_back(t.substr(5, 2)); carrying = false; here = t.substr(5, 2); continue; }
        if (t[0] == '<') continue;                                        // 후진으로 빠져나오는 중
        const std::string& n = t;
        if (n.size() != 2 || n[0] < 'A' || n[0] > 'H') continue;         // 교차점 이름이 아님 (L R U !obs)
        if (n == here) continue;                                          // 방금 내려놓은 자리에서 다시 정렬하는 것은 통과가 아님
        here.clear();
        if (!has(n)) continue;
        if (carrying) return "운반 중 다른 적재물과 충돌 " + n;
        size_t j = i + 1;
        while (j < W.log.size() && W.log[j] == "T") j++;                 // T자까지 더 가는 것은 같은 위치
        if (!(j < W.log.size() && W.log[j].rfind("PICK@" + n, 0) == 0)) return "적재물 위치 통과 " + n;
    }
    return "";
}

static int runCase(int westBlocked1, int westBlocked2, int eastBlocked, const char* label)
{
    worldStart(START_NODE, START_HDG);
    W.edgeAllowed = gridEdge;
    for (int c = 3; c <= 5; c++) W.blocked[3 * 8 + c] = true;            // 가림막
    W.blocked[3 * 8 + westBlocked1] = true;
    if (westBlocked2 >= 0) W.blocked[3 * 8 + westBlocked2] = true;
    W.blocked[3 * 8 + eastBlocked] = true;
    for (int i = 0; i < 3; i++) lane[i] = L_UNKNOWN;
    openLane = -1;
    nav = Nav();
    try { sketch_setup(); } catch (std::exception&) {}
    std::string ev;
    for (auto& t : W.log) if (t.rfind("PICK", 0) == 0 || t.rfind("DROP", 0) == 0) ev += t.substr(0, 7) + " ";
    std::string pal = W.fail.empty() ? checkPallets() : "";
    bool ok = W.fail.empty() && pal.empty() && ev == "PICK@A6 DROP@H2 PICK@A4 DROP@H6 ";
    double est = W.hops * 0.9 + W.turns * 0.7 + W.uturns * 1.2 + 4 * 1.5;
    printf("%s  hops=%d turns=%d uturns=%d  est~%.0fs  %s %s%s\n", label, W.hops, W.turns, W.uturns, est,
           ok ? "OK" : "FAIL", W.fail.c_str(), pal.c_str());
    printf("  %s\n", joinLog().c_str());
    if (!ok) printf("  events: %s\n", ev.c_str());
    return ok ? 0 : 1;
}

int main()
{
    int fails = 0;
    char label[64];
    for (int w = 0; w < 3; w++)
        for (int e = 6; e < 8; e++) {
            snprintf(label, sizeof label, "막힘 D%d,D%d", w + 1, e + 1);
            fails += runCase(w, -1, e, label);
        }
    // 규칙 해석이 반대(3곳 중 1곳만 열림)여도 장애물을 밀지 않는지
    fails += runCase(2, 1, 6, "막힘 D3,D2 (D1 만 열림)");
    fails += runCase(2, 0, 6, "막힘 D3,D1 (D2 만 열림)");
    fails += runCase(1, 0, 6, "막힘 D2,D1 (D3 만 열림)");
    return fails ? 1 : 0;
}
