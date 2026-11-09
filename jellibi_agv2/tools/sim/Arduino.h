// PC 시뮬레이션용 최소 Arduino 스텁
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>
typedef uint8_t byte;
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define A0 14
#define A3 17
#define A6 20
#define A7 21
class __FlashStringHelper;
#define F(s) (reinterpret_cast<const __FlashStringHelper*>(s))
using std::min; using std::max;
unsigned long millis();
void delay(unsigned long);
inline void delayMicroseconds(unsigned) {}
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int digitalRead(int) { return HIGH; }
inline void analogWrite(int, int) {}
inline int analogRead(int) { return 0; }
inline void tone(int, unsigned, unsigned long = 0) {}
inline void noTone(int) {}
extern bool g_serialEcho;
struct SerialStub {
    void begin(long) {}
    int available() { return 0; }
    int read() { return -1; }
    template <class T> void print(T v) { if (g_serialEcho) out(v); }
    template <class T> void println(T v) { if (g_serialEcho) { out(v); std::putchar('\n'); } }
    void println() { if (g_serialEcho) std::putchar('\n'); }
    void out(const __FlashStringHelper* s) { std::fputs(reinterpret_cast<const char*>(s), stdout); }
    void out(const char* s) { std::fputs(s, stdout); }
    void out(char c) { std::putchar(c); }
    void out(int v) { std::printf("%d", v); }
    void out(unsigned v) { std::printf("%u", v); }
    void out(long v) { std::printf("%ld", v); }
    void out(unsigned long v) { std::printf("%lu", v); }
};
extern SerialStub Serial;
