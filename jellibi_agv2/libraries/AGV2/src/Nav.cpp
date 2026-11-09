#include "Nav.h"
#include "Ui.h"

void Nav::setApproach(uint8_t n, uint8_t h)
{
    node = n;
    hdg = h;
    approach = true;
    moving = false;
    _xing.reset();
}

void Nav::setAt(uint8_t n, uint8_t h)
{
    node = n;
    hdg = h;
    approach = false;
    moving = false;
    _xing.reset();
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
    _spd = 0;
    _xing.reset();
}

void Nav::uturnOnEdge()
{
    Brake();
    UTurn();
    setApproach(obsFrom, reverseH(hdg));
    _spd = 0;
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

        if (!traceToXing(spd, chk)) {
            Brake();
            obsFrom = from;
            obsTo = to;
            moving = false;
            _spd = 0;
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

bool Nav::traceToXing(int target, bool checkObstacle)
{
    _obs.reset();
    unsigned long lastMs = millis();
    for (;;) {
        IrSample s = readIr();
        if (_xing.update(s)) return true;

        unsigned long now = millis();
        if (_spd < target) {
            _spd += (int)(now - lastMs) * ACCEL_PER_MS;
            if (_spd > target) _spd = target;
        } else {
            _spd = target;
        }
        lastMs = now;
        lineTraceStep(s, _spd);

        if (checkObstacle && _obs.update()) return false;
        uiCheckAbort();
    }
}

// 센서가 교차점을 본 뒤 바퀴축 중점이 교차점 위에 오도록 ALIGN_MS 더 간다
void Nav::alignAndStop()
{
    unsigned long t0 = millis();
    while (millis() - t0 < ALIGN_MS) {
        IrSample s = readIr();
        lineTraceStep(s, SPEED_APPROACH);
        uiCheckAbort();
    }
    Brake();
    moving = false;
    _spd = 0;
    _xing.reset();
}
