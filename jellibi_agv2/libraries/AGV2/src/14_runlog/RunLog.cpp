#include "14_runlog/RunLog.h"
#include "config.h"
#include "08_map/Grid.h"

#if RUNLOG_ON
#include <EEPROM.h>
#include <avr/eeprom.h>

// EEPROM 배치:  [0] = 표식(RL_MAGIC)   [1] = 예비   [2...] = 기록 5바이트씩
//   기록 = 종류(1) a(1) b(1) 시각(2, 0.01초 단위)      종류 0xFF = 끝
//   쓰는 순서: ① 다음 칸에 '끝' 표시 → ② a, b, 시각 → ③ 종류.  종류를 맨 나중에 쓰므로
//             쓰는 도중 전원이 꺼져도 반쯤 쓰인 줄은 보이지 않는다.
#define RL_MAGIC     0xA7
#define RL_BASE      2
#define RL_REC       5
#define RL_CAPACITY  ((E2END + 1 - RL_BASE) / RL_REC)     // ATmega328P: 204줄
#define RL_QUEUE     8                                    // 저장을 기다리는 줄 수 (RAM 40바이트)

static bool s_active = false;
static unsigned long s_t0 = 0;
static uint8_t s_q[RL_QUEUE][RL_REC];
static uint8_t s_qHead = 0, s_qCount = 0;                 // s_qHead = 지금 쓰는 줄
static uint8_t s_step = 0;                                // 지금 쓰는 줄의 진행 단계 0~5
static uint8_t s_nRec = 0;                                // 이미 저장한 줄 수
static bool s_magicDone = false;

void runlogBegin()
{
    s_active = true;
    s_t0 = millis();
    s_qHead = s_qCount = 0;
    s_step = 0;
    s_nRec = 0;
    s_magicDone = false;
    runlogAdd(RL_START);
}

void runlogAdd(uint8_t type, uint8_t a, uint8_t b)
{
    if (!s_active || s_qCount >= RL_QUEUE) return;        // 꽉 차면 버림 (주행이 우선)
    if ((uint16_t)s_nRec + s_qCount >= RL_CAPACITY) return;
    unsigned long cs = (millis() - s_t0) / 10;            // 0.01초 단위
    if (cs > 65535UL) cs = 65535UL;
    uint8_t* r = s_q[(uint8_t)(s_qHead + s_qCount) % RL_QUEUE];
    r[0] = type; r[1] = a; r[2] = b;
    r[3] = (uint8_t)(cs & 0xFF); r[4] = (uint8_t)(cs >> 8);
    s_qCount++;
}

// 값이 다를 때만 쓰기 시작. 쓰기 시작했으면 true (EEPROM 이 약 3.4ms 동안 바쁨)
static bool putByte(uint16_t addr, uint8_t v)
{
    if (EEPROM.read(addr) == v) return false;
    EEPROM.write(addr, v);
    return true;
}

void runlogPump()
{
    if (!s_qCount) return;
    if (!eeprom_is_ready()) return;                       // 앞 바이트를 아직 쓰는 중 → 다음에
    if (!s_magicDone) {
        s_magicDone = true;
        if (putByte(0, RL_MAGIC)) return;
    }
    uint16_t addr = RL_BASE + (uint16_t)s_nRec * RL_REC;
    const uint8_t* r = s_q[s_qHead];
    for (;;) {
        bool wrote = false;
        if (s_step == 0) {                                // ① 다음 칸 '끝' 표시
            if (s_nRec + 1 < RL_CAPACITY) wrote = putByte(addr + RL_REC, 0xFF);
        } else if (s_step <= 4) {                         // ② a, b, 시각
            wrote = putByte(addr + s_step, r[s_step]);
        } else {                                          // ③ 종류 → 이 줄 완성
            wrote = putByte(addr, r[0]);
            s_step = 0;
            s_nRec++;
            s_qHead = (uint8_t)(s_qHead + 1) % RL_QUEUE;
            s_qCount--;
            return;
        }
        s_step++;
        if (wrote) return;                                // 한 번에 1바이트만
    }
}

void runlogFlush()
{
    while (s_qCount) {
        runlogPump();
        delay(1);
    }
    while (!eeprom_is_ready()) {}
}

static const char RL_NAMES[] PROGMEM =
    "START\0NODE\0STOP\0TURN\0TMISS\0OBST\0LANE\0PLAN\0PICK\0DROP\0HOLD\0DONE\0FATAL\0ABORT\0";

static void printTypeName(uint8_t type)
{
    const char* p = RL_NAMES;
    for (uint8_t i = 1; i < type; i++) { while (pgm_read_byte(p)) p++; p++; }
    for (char c; (c = (char)pgm_read_byte(p)) != 0; p++) Serial.print(c);
}

void runlogDump()
{
    runlogFlush();
    Serial.println(F("===== 지난 주행 기록 (이 줄부터 '기록 끝'까지 복사해서 전달) ====="));
    if (EEPROM.read(0) != RL_MAGIC) {
        Serial.println(F("(기록 없음)"));
    } else {
        uint8_t n = 0;
        for (; n < RL_CAPACITY; n++) {
            uint16_t addr = RL_BASE + (uint16_t)n * RL_REC;
            uint8_t type = EEPROM.read(addr);
            if (type == 0 || type >= RL_TYPE_END) break;
            uint8_t a = EEPROM.read(addr + 1), b = EEPROM.read(addr + 2);
            uint16_t cs = EEPROM.read(addr + 3) | ((uint16_t)EEPROM.read(addr + 4) << 8);
            if (cs < 10000) Serial.print(' ');
            if (cs < 1000) Serial.print(' ');
            Serial.print(cs / 100); Serial.print('.');
            if (cs % 100 < 10) Serial.print('0');
            Serial.print(cs % 100); Serial.print(' ');
            printTypeName(type);
            switch (type) {
                case RL_NODE:
                    Serial.print(' '); printNode(a & 0x3F); Serial.print(' '); Serial.print(headingChar(a >> 6));
                    Serial.print(F(" err=")); Serial.print(b);
                    break;
                case RL_STOP: case RL_OBST: case RL_PICK: case RL_DROP: case RL_HOLD:
                    Serial.print(' '); printNode(a);
                    break;
                case RL_PLAN:
                    Serial.print(' '); printNode(a); Serial.print(F(" n=")); Serial.print(b);
                    break;
                case RL_TURN:
                    Serial.print(' '); Serial.print((char)a);
                    break;
                case RL_LANE:
                    for (uint8_t k = 0; k < 4; k++) { Serial.print(' '); Serial.print("?OX-"[(a >> (2 * k)) & 3]); }
                    break;
                case RL_FATAL:
                    Serial.print(' '); Serial.print(a);
                    break;
                default: break;
            }
            Serial.println();
        }
        Serial.print(F("(")); Serial.print(n); Serial.println(F("줄)"));
    }
    Serial.println(F("===== 기록 끝 ====="));
}

#else   // RUNLOG_ON 0: 아무 일도 하지 않는 함수

void runlogBegin() {}
void runlogAdd(uint8_t, uint8_t, uint8_t) {}
void runlogPump() {}
void runlogFlush() {}
void runlogDump() {}

#endif
