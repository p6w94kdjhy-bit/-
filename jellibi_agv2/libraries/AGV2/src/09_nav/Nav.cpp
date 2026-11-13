#include "09_nav/Nav.h"
#include "12_signal/Signal.h"
#include "14_runlog/RunLog.h"

void Nav::setApproach(uint8_t n, uint8_t h)
{
    node = n;
    hdg = h;
    approach = true;
    moving = false;
    lineTraceReset();
}

void Nav::setAt(uint8_t n, uint8_t h)
{
    node = n;
    hdg = h;
    approach = false;
    moving = false;
    lineTraceReset();
}

bool Nav::mustStopAt(uint8_t a, uint8_t b, uint8_t c)
{
    if (c == NODE_NONE) return true;
    return headingTo(a, b) != headingTo(b, c);
}

void Nav::turnTo(uint8_t h)
{
    uint8_t d = (h - hdg) & 3;
    if (d) runlogAdd(RL_TURN, d == 1 ? 'R' : (d == 3 ? 'L' : 'U'));
    if (d == 1) TurnRight90();
    else if (d == 3) TurnLeft90();
    else if (d == 2) UTurn();
    hdg = h;
    if (afterBack && d) lineTraceDisarm();     // 후진 뒤 회전: 센서가 옛 선 위일 수 있음 → 흰색을 지난 뒤부터 셈
    afterBack = false;
}

void Nav::backTo(uint8_t target, uint8_t extra)
{
    // 세어야 할 교차점 수 = (마지막으로 센 교차점 ~ target 칸 수) + 1
    //   센서가 바퀴축보다 앞에 있으므로, 후진하면 '마지막으로 센 교차점'의 선을 센서가 먼저 다시 지난다
    if (approach) uiFatal(F("backTo: approach"), 3);
    if (target != node && headingTo(node, target) != (int8_t)reverseH(hdg)) uiFatal(F("backTo: target not behind"), 3);
    uint8_t behind = stepNode(target, reverseH(hdg));
    bool settle = REV_SETTLE && behind != NODE_NONE;       // 한 칸 더 후진 → 선 따라 한 칸 전진하며 중앙으로
    uint8_t count = straightDist(node, target) + 1 + (settle ? 1 : 0) + extra;
    runlogAdd(RL_BACK, target, count);
    Serial.print(F("[nav] BACK to ")); printNode(target); Serial.print(F(" count=")); Serial.println(count);

    LineTraceBackCross(count, REV_PWM);    // 후진하며 교차점 세기
    waitMs(REV_PAST_MS);                   // 마지막 선을 조금 더 지나서
    Brake();
    delay(TURN_SETTLE_MS);

    if (settle) {
        // 뒤 교차점(behind) → target 한 칸을 종목① 과 같이 선 따라 전진 → 후진 중 생긴 옆 어긋남·비뚤어짐을 바로잡고 정렬
        lineTraceReset();
        LineTraceToCross(SPEED_APPROACH);  // behind 선
        LineTraceToCross(SPEED_APPROACH);  // target 선
        AlignOnCross();
    } else {
        RecatchCrossStraight();            // 앞으로 똑바로 천천히 → target 선을 다시 찾아 바퀴축 정렬
    }
    afterBack = true;
    node = target;
    approach = false;
    moving = false;
    runlogAdd(RL_STOP, node);
}

void Nav::uturnOnEdge()
{
    Brake();
    runlogAdd(RL_TURN, 'E');
    UTurnOnEdge();
    setApproach(obsFrom, reverseH(hdg));
}

void Nav::hold(unsigned long ms)
{
    Stop();
    runlogAdd(RL_HOLD, node);
    beep(1319, 80);
    waitMs(ms > 80 ? ms - 80 : 0);
}

NavResult Nav::leg(uint8_t target, bool stopAtEnd)
{
    if (approach) {
        if (target == node) return hops(1, stopAtEnd);
        int8_t h = headingTo(node, target);
        if (h < 0) uiFatal(F("leg: not straight"), 2);
        if (h == hdg) return hops(straightDist(node, target) + 1, stopAtEnd);
        // 방향이 다르면 일단 node에 도착·정렬 후 회전
        NavResult r = hops(1, true);
        if (r != NAV_OK) return r;
    }
    if (target == node) {
        if (moving && stopAtEnd) alignAndStop();
        return NAV_OK;
    }
    int8_t h = headingTo(node, target);
    if (h < 0) uiFatal(F("leg: not straight"), 2);
    if (h != hdg) {
        if (moving) {                // 계획 오류 방어: 지나쳤을 수 있음
            Serial.println(F("[nav] turn while moving"));
            alignAndStop();
        }
        turnTo(h);
    }
    return hops(straightDist(node, target), stopAtEnd);
}

NavResult Nav::hops(uint8_t count, bool stopAtEnd)
{
    for (uint8_t i = 0; i < count; i++) {
        uint8_t from, to;
        if (approach) { to = node; from = stepNode(node, reverseH(hdg)); }
        else          { from = node; to = stepNode(node, hdg); }

        bool last = (i == count - 1);
        int spd = (last && stopAtEnd) ? SPEED_APPROACH : cruise;
        bool chk = hopCheck && from != NODE_NONE && to != NODE_NONE && hopCheck(from, to);
        if (chk && spd > SPEED_ZONE) spd = SPEED_ZONE;

        if (hopMsSpeed != cruise) { hopMsSpeed = cruise; hopMs = 0; }   // 통과 속도가 바뀌면 다시 잼
#if LATE_BRAKE_PCT > 0
        // 늦은 감속: 정차할 칸의 앞부분은 통과 속도로 달리고, 남은 구간만 SPEED_APPROACH 로 접근
        //   달리는 시간 = (이번 주행에서 잰 한 칸 통과 시간) × LATE_BRAKE_PCT %  → 배터리가 바뀌어도 같은 거리
        if (last && stopAtEnd && !chk && hopMs > 0 && cruise > spd)
            lineTraceBoost(cruise, (unsigned long)hopMs * LATE_BRAKE_PCT / 100);
#endif
        bool wasMoving = moving;
        unsigned long tHop = millis();

        if (!LineTraceToCross(spd, chk)) {          // 라인트레이싱 (LineTrace.cpp)
            obsFrom = from;
            obsTo = to;
            moving = false;
            Serial.print(F("[nav] OBSTACLE "));
            printNode(from); Serial.print(F("->")); printNode(to); Serial.println();
            runlogAdd(RL_OBST, to);
            return NAV_OBSTACLE;
        }
        // 정지 없이 통과 속도로 달린 칸이면 걸린 시간을 기억 (가장 짧은 값)
        if (wasMoving && spd == cruise) {
            unsigned long dt = millis() - tHop;
            if (dt < 60000UL && (hopMs == 0 || dt < hopMs)) hopMs = (uint16_t)dt;
        }
        uint16_t e = lineTraceTakeMaxErr() / 10;                           // 이 칸에서 가장 크게 치우친 정도 0~100 %
        runlogAdd(RL_NODE, (uint8_t)(to | (hdg << 6)), (uint8_t)(e > 255 ? 255 : e));
        node = to;
        approach = false;
        moving = true;
        beepNode();
        printNode(node); Serial.print(' '); Serial.print(headingChar(hdg));
        Serial.print(F(" err=")); Serial.println(e);
        if (onNode) onNode(node);
    }
    if (stopAtEnd) alignAndStop();
    return NAV_OK;
}

// 교차점 위에 바퀴축을 맞추고 정지 (LineTrace.cpp)
void Nav::alignAndStop()
{
    AlignOnCross();
    moving = false;
    runlogAdd(RL_STOP, node);
}
