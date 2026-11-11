#!/usr/bin/env bash
# PC에서 경로 로직 검증: ./tools/sim/run.sh
set -e
D="$(cd "$(dirname "$0")" && pwd)"; L="$D/../../libraries/AGV2/src"; O="${TMPDIR:-/tmp}/agv2sim"; mkdir -p "$O"
CXX="g++ -std=c++11 -O1 -Wall -Wno-unused-function -I$D -I$L"
for s in 0 1; do
  $CXX -DSIM_SXP=$s "$D/sim_course1.cpp" "$L/09_nav/Nav.cpp" "$L/08_map/Grid.cpp" -o "$O/c1_$s" && "$O/c1_$s"
done
$CXX -DSIM_SXP=0 -DCOURSE1_INO='"../../course1_drive_v2/course1_drive_v2.ino"' "$D/sim_course1.cpp" "$L/09_nav/Nav.cpp" "$L/08_map/Grid.cpp" -o "$O/c1v2" && echo "[v2]" && "$O/c1v2"
for f in 0 1; do
  $CXX -DFRONT_CHECK_WHILE_CARRY=$f "$D/sim_course2.cpp" "$L/09_nav/Nav.cpp" "$L/08_map/Grid.cpp" "$L/10_planner/Planner.cpp" "$L/07_logistics/Pallet.cpp" -o "$O/c2_$f" && "$O/c2_$f"
done
