// 가짜 EEPROM: 쓰기 1바이트에 3.4ms 걸리고, 그동안 다시 쓰려면 기다려야 한다(실제와 동일)
#pragma once
#include <cstdint>
struct EEPROMStub {
    uint8_t read(int addr);
    void write(int addr, uint8_t v);
};
extern EEPROMStub EEPROM;
