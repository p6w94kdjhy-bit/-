#!/usr/bin/env bash
# 모든 스케치 컴파일 확인.
#  1) arduino-cli 가 있으면 그것으로 (권장)
#       ./tools/build_check.sh            # cpu=atmega328
#       CPU=atmega328old ./tools/build_check.sh
#  2) 없으면 avr-gcc + ArduinoCore-avr + Servo 소스로 직접 컴파일 (MFRC522 는 AGV2 안에 포함)
#       CORE=~/ArduinoCore-avr SERVO=~/Servo ./tools/build_check.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SKETCHES="${SKETCHES:-course1_drive course1_drive_v2 course2_pallet course2_drive calib_ir tune_drive rfid_test}"
CPU="${CPU:-atmega328}"

if command -v arduino-cli >/dev/null 2>&1; then
    for s in $SKETCHES; do
        echo "== $s"
        arduino-cli compile --fqbn "arduino:avr:nano:cpu=$CPU" \
            --libraries "$ROOT/libraries" "$ROOT/$s"
    done
    exit 0
fi

: "${CORE:?arduino-cli 없음: CORE=<ArduinoCore-avr 경로> 지정}"
: "${SERVO:?SERVO=<arduino-libraries/Servo 경로> 지정}"
OUT="${OUT:-$(mktemp -d)}"
FLAGS="-c -g -Os -w -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=10819 -DARDUINO_AVR_NANO -DARDUINO_ARCH_AVR"
INC="-I$CORE/cores/arduino -I$CORE/variants/eightanaloginputs -I$CORE/variants/standard -I$SERVO/src  -I$CORE/libraries/SPI/src -I$CORE/libraries/EEPROM/src -I$ROOT/libraries/AGV2/src"
CXX="avr-g++ $FLAGS -std=gnu++11 -fpermissive -fno-exceptions -fno-threadsafe-statics -flto"
CC="avr-gcc $FLAGS -std=gnu11 -flto"

mkdir -p "$OUT/core" "$OUT/lib"
for f in "$CORE"/cores/arduino/*.c;   do $CC  $INC "$f" -o "$OUT/core/$(basename "$f").o"; done
for f in "$CORE"/cores/arduino/*.cpp; do $CXX $INC "$f" -o "$OUT/core/$(basename "$f").o"; done
avr-gcc -c -g -x assembler-with-cpp -flto -mmcu=atmega328p -DF_CPU=16000000L $INC \
    "$CORE/cores/arduino/wiring_pulse.S" -o "$OUT/core/wiring_pulse.S.o"
$CXX $INC "$SERVO/src/avr/Servo.cpp" -o "$OUT/lib/Servo.o"
$CXX $INC "$CORE/libraries/SPI/src/SPI.cpp" -o "$OUT/lib/SPI.o"
find "$ROOT/libraries/AGV2/src" -name "*.cpp" | while read -r f; do $CXX -Wall -Wextra $INC -I"$(dirname "$f")" "$f" -o "$OUT/lib/$(basename "$f").o"; done

for s in $SKETCHES; do
    echo "== $s"
    { echo '#include <Arduino.h>'; cat "$ROOT/$s/$s.ino"; } > "$OUT/$s.cpp"
    $CXX -Wall $INC "$OUT/$s.cpp" -o "$OUT/$s.o"
    avr-gcc -w -Os -g -flto -fuse-linker-plugin -Wl,--gc-sections -mmcu=atmega328p \
        -o "$OUT/$s.elf" "$OUT/$s.o" "$OUT"/lib/*.o "$OUT"/core/*.o -lm
    avr-size -C --mcu=atmega328p "$OUT/$s.elf" | grep -E "Program|Data"
done
