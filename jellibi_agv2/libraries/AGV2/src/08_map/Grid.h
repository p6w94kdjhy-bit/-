// ===================================================================
//  [08 맵 좌표] A~H행(위=북쪽=A) × 1~8열(왼쪽=1). 교차점 번호 = 행*8 + (열-1)
//   P('C',5) 처럼 교차점 이름을 번호로 바꾸고, 방향(N/E/S/W)·이웃 계산
// ===================================================================
#ifndef AGV2_GRID_H
#define AGV2_GRID_H

#include <Arduino.h>

enum Heading : uint8_t { HN = 0, HE = 1, HS = 2, HW = 3 };   // 시계방향

#define NODE_NONE 0xFF

// P('C',5) → C5 교차점 번호
constexpr uint8_t P(char row, uint8_t col) { return (uint8_t)((row - 'A') * 8 + (col - 1)); }

inline uint8_t rowOf(uint8_t n) { return n >> 3; }   // 0=A ... 7=H
inline uint8_t colOf(uint8_t n) { return n & 7; }    // 0=1열 ... 7=8열
inline uint8_t reverseH(uint8_t h) { return (h + 2) & 3; }

uint8_t stepNode(uint8_t n, uint8_t h);           // 이웃 교차점, 맵 밖이면 NODE_NONE
int8_t  headingTo(uint8_t a, uint8_t b);          // 같은 행/열이면 방향, 아니면 -1
uint8_t straightDist(uint8_t a, uint8_t b);       // 같은 행/열 칸 수
void    printNode(uint8_t n);                     // "C5"
char    headingChar(uint8_t h);                   // 'N' 'E' 'S' 'W'

#endif
