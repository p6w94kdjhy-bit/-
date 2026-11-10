#include "05_obstacle/Obstacle.h"
#include "02_sensor/Sensor.h"

bool frontCalibrated() { return FRONT_OBSTACLE_TH > 0; }

bool obstacleNow(int v)
{
#if FRONT_NEAR_IS_HIGH
    return v > FRONT_OBSTACLE_TH;
#else
    return v < FRONT_OBSTACLE_TH;
#endif
}

bool ObstacleDetector::update()
{
    if (!frontCalibrated()) return false;
    if (obstacleNow(irFront())) {
        if (_cnt < 255) _cnt++;
    } else {
        _cnt = 0;
    }
    return _cnt >= FRONT_CONFIRM_SAMPLES;
}
