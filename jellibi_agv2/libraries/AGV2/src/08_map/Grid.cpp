#include "08_map/Grid.h"

uint8_t stepNode(uint8_t n, uint8_t h)
{
    if (n == NODE_NONE) return NODE_NONE;
    int8_t r = rowOf(n), c = colOf(n);
    switch (h & 3) {
        case HN: r--; break;
        case HE: c++; break;
        case HS: r++; break;
        case HW: c--; break;
    }
    if (r < 0 || r > 7 || c < 0 || c > 7) return NODE_NONE;
    return (uint8_t)(r * 8 + c);
}

int8_t headingTo(uint8_t a, uint8_t b)
{
    if (a == b) return -1;
    if (rowOf(a) == rowOf(b)) return colOf(b) > colOf(a) ? HE : HW;
    if (colOf(a) == colOf(b)) return rowOf(b) > rowOf(a) ? HS : HN;
    return -1;
}

uint8_t straightDist(uint8_t a, uint8_t b)
{
    int dr = (int)rowOf(a) - rowOf(b);
    int dc = (int)colOf(a) - colOf(b);
    return (uint8_t)(abs(dr) + abs(dc));
}

void printNode(uint8_t n)
{
    if (n == NODE_NONE) { Serial.print(F("--")); return; }
    Serial.print((char)('A' + rowOf(n)));
    Serial.print(colOf(n) + 1);
}

char headingChar(uint8_t h) { return "NESW"[h & 3]; }
