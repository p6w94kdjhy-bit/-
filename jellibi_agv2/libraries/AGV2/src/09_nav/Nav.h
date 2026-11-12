// [09 격자 주행] 현재 교차점·방향을 기억하며 "C5까지 직진" 같은 구간 단위로 이동 (종목②)
//   내부에서 03 라인트레이싱, 04 회전, 05 장애물 감지를 사용
//
//  상태
//   node    : 마지막으로 도착(카운트)한 교차점
//   hdg     : 현재 진행 방향
//   approach: true면 로봇이 node '앞'에 있고 다음에 세는 교차점이 node (출발·중간 U턴 후)
//   moving  : 교차점을 정지 없이 통과 중
#ifndef AGV2_NAV_H
#define AGV2_NAV_H

#include <Arduino.h>
#include "08_map/Grid.h"
#include "01_motor/Motor.h"
#include "03_linetrace/LineTrace.h"
#include "04_turn/Turn.h"

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

    // 늦은 감속용: 이번 주행에서 잰 '통과 속도로 한 칸 가는 시간'의 최솟값 (0 = 아직 못 잼)
    uint16_t hopMs = 0;
    bool afterBack = false;             // 방금 후진했나 (다음 회전 뒤 교차점 세기를 잠시 막음)
    int hopMsSpeed = 0;                 // 그 시간을 잰 속도 (속도가 바뀌면 다시 잼)

    void setApproach(uint8_t n, uint8_t h);   // n 앞에서 h 방향으로 대기/진입 중
    void setAt(uint8_t n, uint8_t h);         // n 위에 정렬·정지

    // 현재 위치에서 같은 행/열의 target까지 직진. 방향이 다르면 먼저 정지·회전.
    // stopAtEnd=false면 target을 정지 없이 통과(다음 leg가 같은 방향일 때).
    NavResult leg(uint8_t target, bool stopAtEnd);

    void turnTo(uint8_t h);                   // 교차점 위(정지 상태)에서 회전

    // 지금 방향을 유지한 채 뒤쪽(같은 줄)의 target 교차점까지 후진 → target 위에 바퀴축을 맞춰 정지 (방향 그대로)
    //   장애물 앞(칸 중간)에서 G·B행으로 돌아갈 때, 팔레트를 든 뒤/내린 뒤 빠져나올 때 사용
    //   순서: 교차점을 세며 후진 → REV_PAST_MS 더 후진 → 정지 → 앞으로 똑바로 천천히 가며 target 을 다시 찾아 정렬
    //   후진 뒤에는 항상 회전이 오므로(센서로 새 선을 찾아 멈춤) 후진 중 생긴 작은 방향 오차는 그 회전에서 바로잡힌다
    void backTo(uint8_t target);
    void uturnOnEdge();                       // 장애물 앞(칸 중간)에서 U턴 → obsFrom으로 접근 상태
    void hold(unsigned long ms);              // 정차 유지 (+부저)

    // a→b로 들어와서 b→c로 나갈 때 b에서 멈춰야 하나 (방향 변경)
    static bool mustStopAt(uint8_t a, uint8_t b, uint8_t c);

private:
    NavResult hops(uint8_t count, bool stopAtEnd);
    void alignAndStop();
};

#endif
