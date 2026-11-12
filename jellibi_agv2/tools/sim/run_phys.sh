#!/usr/bin/env bash
# 물리 시뮬레이션: 실제 라인트레이싱·격자 주행·주행 기록 코드를 가짜 로봇으로 달려 본다.
#   ./tools/sim/run_phys.sh            # 통과 속도 SPEED_CRUISE, 회전 오차 ±8°
#   ./tools/sim/run_phys.sh 200 12     # 통과 속도 PWM 200, 회전 오차 ±12°
#   LB=55 ./tools/sim/run_phys.sh      # LATE_BRAKE_PCT 55 로
#   ./tools/sim/run_phys.sh 168 8 log  # 끝에 주행 기록 출력 예시
set -e
D="$(cd "$(dirname "$0")" && pwd)"; L="$D/../../libraries/AGV2/src"; O="${TMPDIR:-/tmp}/agv2sim"; mkdir -p "$O"
DEFS=""; [ -n "${LB:-}" ] && DEFS="-DLATE_BRAKE_PCT=$LB"
[ -n "${REVL:-}" ] && DEFS="$DEFS -DREV_L_GAIN=$REVL"   # 후진 좌우 보정 (가짜 로봇은 103 이 똑바로)
[ -n "${SETTLE:-}" ] && DEFS="$DEFS -DREV_SETTLE=$SETTLE"
[ -n "${OB:-}" ] && DEFS="$DEFS -DOBST_BACK=$OB"; [ -n "${PB:-}" ] && DEFS="$DEFS -DPICK_BACK=$PB"; [ -n "${DB:-}" ] && DEFS="$DEFS -DDROP_BACK=$DB"
g++ -std=c++11 -O2 -Wall -Wno-unused-function $DEFS -I"$D/phys" -I"$L" "$D/phys/phys.cpp" \
    "$L/01_motor/Motor.cpp" "$L/02_sensor/Sensor.cpp" "$L/03_linetrace/LineTrace.cpp" "$L/05_obstacle/Obstacle.cpp" \
    "$L/08_map/Grid.cpp" "$L/09_nav/Nav.cpp" "$L/14_runlog/RunLog.cpp" -o "$O/phys"
#   C2=4 ./tools/sim/run_phys.sh   # course2_drive 전체 (막힌 통로 3가지 × 로봇 특성 9가지 × seed 4)
"$O/phys" "$@"
