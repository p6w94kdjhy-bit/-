"""F = (A+B)·A·C 를 74LS32(OR) 1개 + 74LS08(AND) 2게이트로 구현.

    A ──┬──────────────┐
        │   ┌─────┐    │  ┌─────┐
        └───┤ OR  ├─N1─┴──┤ AND ├─N2──┐  ┌─────┐
    B ──────┤ U1A │       │ U2A │     └──┤ AND ├── F
            └─────┘       └─────┘   C ───┤ U2B │
                                         └─────┘

실행: 저장소 루트에서  python3 -m examples.and_or_f
"""

import itertools
from pathlib import Path

from logicsim import Circuit, ls08, ls32, render_svg

OUT_DIR = Path(__file__).parent / "output"


def build():
    c = Circuit()
    u1 = c.add(ls32("U1"))
    u2 = c.add(ls08("U2"))
    for chip in (u1, u2):
        c.connect(chip, 14, "VCC")
        c.connect(chip, 7, "GND")

    # U1A: N1 = A + B
    c.connect(u1, 1, "A")
    c.connect(u1, 2, "B")
    c.connect(u1, 3, "N1")
    # U2A: N2 = N1 · A
    c.connect(u2, 1, "N1")
    c.connect(u2, 2, "A")
    c.connect(u2, 3, "N2")
    # U2B: F = N2 · C
    c.connect(u2, 4, "N2")
    c.connect(u2, 5, "C")
    c.connect(u2, 6, "F")
    return c


def evaluate(a, b, cin):
    c = build()
    for name, v in zip("ABC", (a, b, cin)):
        c.set_input(name, v, 0)
    c.run()
    return c.nets["F"].value


def truth_table():
    return [(a, b, cin, evaluate(a, b, cin)) for a, b, cin in itertools.product((0, 1), repeat=3)]


def counting_run(step_ns=100):
    """ABC를 000 → 111 순서로 바꿔가며 돌린다."""
    c = build()
    for i, (a, b, cin) in enumerate(itertools.product((0, 1), repeat=3)):
        for name, v in zip("ABC", (a, b, cin)):
            c.set_input(name, v, i * step_ns)
    c.run()
    return c, 8 * step_ns


def critical_path_run(change_at_ns=20):
    """B=0, C=1에서 A만 0→1: 신호가 OR → AND → AND 를 모두 거치는 가장 긴 경로."""
    c = build()
    c.set_input("A", 0, 0)
    c.set_input("B", 0, 0)
    c.set_input("C", 1, 0)
    c.set_input("A", 1, change_at_ns)
    c.run()
    f_rise = next(t for t, v in c.nets["F"].history if v == 1)
    return c, f_rise - change_at_ns


SIGNALS = [
    ("A", "A"),
    ("B", "B"),
    ("C", "C"),
    ("N1", "N1 = A+B"),
    ("N2", "N2 = N1·A"),
    ("F", "F"),
]


def main():
    print("F = (A+B)·A·C   [U1: 74LS32, U2: 74LS08]\n")
    print(" A B C | F | 식 (A+B)AC | 간소화 AC")
    print("-------+---+------------+----------")
    for a, b, cin, f in truth_table():
        expected = int((a or b) and a and cin)
        simplified = a & cin
        mark = "OK" if f == expected == simplified else "불일치!"
        print(f" {a} {b} {cin} | {f} |     {expected}      |    {simplified}    {mark}")

    OUT_DIR.mkdir(exist_ok=True)
    c, t_end = counting_run()
    (OUT_DIR / "waveform_all.svg").write_text(
        render_svg(c, SIGNALS, 0, t_end, "ABC = 000 → 111 (100ns 간격)", grid_ns=100),
        encoding="utf-8",
    )

    c, delay = critical_path_run()
    (OUT_DIR / "waveform_critical_path.svg").write_text(
        render_svg(c, SIGNALS, 0, 80, f"최장 경로: A↑ → F↑ = {delay} ns (B=0, C=1)", grid_ns=10),
        encoding="utf-8",
    )
    print(f"\n최장 경로 지연 (A↑ → F↑): {delay} ns  = OR 14 + AND 8 + AND 8")
    print(f"파형 저장: {OUT_DIR}/waveform_all.svg, waveform_critical_path.svg")


if __name__ == "__main__":
    main()
