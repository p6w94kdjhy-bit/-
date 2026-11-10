#include "Nav.h"
#include "Ui.h"

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
    if (d == 1) TurnRight90();
    else if (d == 3) TurnLeft90();
    else if (d == 2) UTurn();
    hdg = h;
}

void Nav::uturnOnEdge()
{
    Brake();
    UTurn();
    setApproach(obsFrom, reverseH(hdg));
}

void Nav::hold(unsigned long ms)
{
    Stop();
    beep(1319, 80);
    waitMs(ms > 80 ? ms - 80 : 0);
}

NavResult Nav::leg(uint8_t target, bool stopAtEnd)
{
    if (approach) {
        if (target == node) return hops(1, stopAtEnd);
        int8_t h = headingTo(node, target);
        if (h < 0) uiFatal(F("leg: not straight"));
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
    if (h < 0) uiFatal(F("leg: not straight"));
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

        if (!LineTraceToCross(spd, chk)) {          // 라인트레이싱 (LineTrace.cpp)
            obsFrom = from;
            obsTo = to;
            moving = false;
            Serial.print(F("[nav] OBSTACLE "));
            printNode(from); Serial.print(F("->")); printNode(to); Serial.println();
            return NAV_OBSTACLE;
        }
        node = to;
        approach = false;
        moving = true;
        beepNode();
        printNode(node); Serial.print(' '); Serial.println(headingChar(hdg));
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
}
