// 격자 주행: 현재 교차점·방향을 추적하며 직선 구간(leg) 단위로 이동
//
//  상태
//   node    : 마지막으로 도착(카운트)한 교차점
//   hdg     : 현재 진행 방향
//   approach: true면 로봇이 node '앞'에 있고 다음에 세는 교차점이 node (출발·중간 U턴 후)
//   moving  : 교차점을 정지 없이 통과 중
#ifndef AGV2_NAV_H
#define AGV2_NAV_H

#include <Arduino.h>
#include "Grid.h"
#include "Drive.h"

enum NavResult : uint8_t { NAV_OK = 0, NAV_OBSTACLE = 1 };

typedef bool (*HopCheckFn)(uint8_t from, uint8_t to);   // true → 이 칸에서 전방 장애물 감시
typedef void (*NodeFn)(uint8_t node);                   // 교차점을 셀 때마다 호출

class Nav {
public:
    uint8_t node = NODE_NONE;
    uint8_t hdg = HN;
    bool approach = false;
    bool moving = false;

    int cruise = SPEED_CRUISE;          // 통과 구간 속도 (운반 중엔 낮춤)
    HopCheckFn hopCheck = nullptr;
    NodeFn onNode = nullptr;

    uint8_t obsFrom = NODE_NONE, obsTo = NODE_NONE;   // 장애물을 만난 칸

    void setApproach(uint8_t n, uint8_t h);   // n 앞에서 h 방향으로 대기/진입 중
    void setAt(uint8_t n, uint8_t h);         // n 위에 정렬·정지

    // 현재 위치에서 같은 행/열의 target까지 직진. 방향이 다르면 먼저 정지·회전.
    // stopAtEnd=false면 target을 정지 없이 통과(다음 leg가 같은 방향일 때).
    NavResult leg(uint8_t target, bool stopAtEnd);

    void turnTo(uint8_t h);                   // 교차점 위(정지 상태)에서 회전
    void uturnOnEdge();                       // 장애물 앞(칸 중간)에서 U턴 → obsFrom으로 접근 상태
    void hold(unsigned long ms);              // 정차 유지 (+부저)

    // a→b로 들어와서 b→c로 나갈 때 b에서 멈춰야 하나 (방향 변경)
    static bool mustStopAt(uint8_t a, uint8_t b, uint8_t c);

private:
    NavResult hops(uint8_t count, bool stopAtEnd);
    void alignAndStop();
};

#endif
