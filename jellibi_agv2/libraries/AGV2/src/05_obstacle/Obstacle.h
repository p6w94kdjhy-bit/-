// ===================================================================
//  [05 장애물 감지] 전방 IR(A0)로 앞의 장애물을 감지 (종목②)
//   - 값이 FRONT_OBSTACLE_TH 를 넘는(또는 밑도는) 상태가 연속 FRONT_CONFIRM_SAMPLES 회면 장애물
//   - 방향은 FRONT_NEAR_IS_HIGH 로 선택 (가까울수록 큼 / 작음)
// ===================================================================
#ifndef AGV2_OBSTACLE_H
#define AGV2_OBSTACLE_H

#include <Arduino.h>
#include "config.h"

bool frontCalibrated();          // config.h에 전방 IR 현장값이 들어있나
bool obstacleNow(int v);         // 이 값 하나가 '장애물 있음' 쪽인가

class ObstacleDetector {
public:
    void reset() { _cnt = 0; }
    bool update();               // 전방 IR을 읽어 연속 n회면 true
private:
    uint8_t _cnt = 0;
};

#endif
