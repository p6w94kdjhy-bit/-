"""실습: Y1 = A'B' + AB' + AB  와  Y2 = A + B'  비교.

부품: 74LS04 2개, 74LS08 1개, 74LS32 2개
입력: IN_1 = A, IN_2 = B (클럭, 시작값 LOW, 주기 배수 10 / 20)
출력: Y1 → LED1, Y2 → LED2

실행: 저장소 루트에서  python3 -m examples.lab_a_plus_not_b
"""

import itertools
from pathlib import Path

from logicsim import Circuit, Schematic, ls04, ls08, ls32, render_svg

OUT_DIR = Path(__file__).parent / "output" / "lab_a_plus_not_b"

# 클럭 "multiplied by" 1칸을 100ns로 가정 → A 주기 1µs, B 주기 2µs
UNIT_NS = 100
A_PERIOD_NS = 10 * UNIT_NS
B_PERIOD_NS = 20 * UNIT_NS
T_END_NS = 2 * B_PERIOD_NS


def build(ideal=True):
    c = Circuit(ideal=ideal)
    u1, u2 = c.add(ls04("U1")), c.add(ls04("U2"))
    u3 = c.add(ls08("U3"))
    u4, u5 = c.add(ls32("U4")), c.add(ls32("U5"))
    for chip in (u1, u2, u3, u4, u5):
        c.connect(chip, 14, "VCC")
        c.connect(chip, 7, "GND")

    wiring = [
        # 회로 1: Y1 = A'B' + AB' + AB
        (u1, 1, "A"), (u1, 2, "NA"),               # U1A: A'
        (u1, 3, "B"), (u1, 4, "NB"),               # U1B: B'
        (u3, 1, "NA"), (u3, 2, "NB"), (u3, 3, "P1"),  # U3A: A'B'
        (u3, 4, "A"), (u3, 5, "NB"), (u3, 6, "P2"),   # U3B: AB'
        (u3, 9, "A"), (u3, 10, "B"), (u3, 8, "P3"),   # U3C: AB
        (u4, 1, "P1"), (u4, 2, "P2"), (u4, 3, "S1"),  # U4A: A'B' + AB'
        (u4, 4, "S1"), (u4, 5, "P3"), (u4, 6, "Y1"),  # U4B: Y1
        # 회로 2: Y2 = A + B'
        (u2, 1, "B"), (u2, 2, "NB2"),              # U2A: B'
        (u5, 1, "A"), (u5, 2, "NB2"), (u5, 3, "Y2"),  # U5A: Y2
    ]
    for chip, pin, net in wiring:
        c.connect(chip, pin, net)
    return c


def clock(c, net, period_ns, t_end_ns):
    """시작값 LOW, 50% 듀티 클럭."""
    half = period_ns // 2
    for k in range(t_end_ns // half + 1):
        c.set_input(net, k % 2, k * half)


def lab_run(ideal=True):
    c = build(ideal)
    clock(c, "A", A_PERIOD_NS, T_END_NS)
    clock(c, "B", B_PERIOD_NS, T_END_NS)
    c.run()
    return c


def truth_table():
    rows = []
    for a, b in itertools.product((0, 1), repeat=2):
        c = build()
        c.set_input("A", a, 0)
        c.set_input("B", b, 0)
        c.run()
        rows.append((a, b, c.nets["Y1"].value, c.nets["Y2"].value))
    return rows


def glitch_widths(history, max_ns=50):
    """max_ns보다 짧게 유지된 펄스 폭 목록."""
    return [t2 - t1 for (t1, _), (t2, _) in zip(history, history[1:]) if t2 - t1 < max_ns]


def schematic_circuit1():
    s = Schematic(880, 350, "회로 1:  Y1 = A'·B' + A·B' + A·B")
    not_a = s.not_gate(200, 60, "U1A 74LS04", pins=(1, 2))
    not_b = s.not_gate(200, 140, "U1B 74LS04", pins=(3, 4))
    and1 = s.and_gate(380, 100, "U3A 74LS08", pins=(1, 2, 3))
    and2 = s.and_gate(380, 200, "U3B 74LS08", pins=(4, 5, 6))
    and3 = s.and_gate(380, 300, "U3C 74LS08", pins=(9, 10, 8))
    or1 = s.or_gate(520, 150, "U4A 74LS32", pins=(1, 2, 3))
    or2 = s.or_gate(660, 225, "U4B 74LS32", pins=(4, 5, 6))

    # A, B 입력과 분기
    s.wire((90, 60), not_a[0])
    s.wire((140, 60), (140, 290), and3[0])
    s.wire((140, 190), and2[0])
    s.wire((90, 140), not_b[0])
    s.wire((170, 140), (170, 310), and3[1])
    for p in [(140, 60), (140, 190), (170, 140)]:
        s.dot(*p)

    # A', B'
    s.wire(not_a[1], (300, 60), (300, 90), and1[0])
    s.wire(not_b[1], (330, 140), (330, 110), and1[1])
    s.wire((330, 140), (330, 210), and2[1])
    s.dot(330, 140)
    s.text(250, 52, "A'", size=13, color="#1f5fbf")
    s.text(250, 132, "B'", size=13, color="#1f5fbf")

    # 곱항 → OR
    s.wire(and1[2], (470, 100), (470, 140), or1[0])
    s.wire(and2[2], (490, 200), (490, 160), or1[1])
    s.wire(or1[2], (620, 150), (620, 215), or2[0])
    s.wire(and3[2], (640, 300), (640, 235), or2[1])
    s.text(446, 92, "A'B'", size=12, color="#1f5fbf")
    s.text(446, 192, "AB'", size=12, color="#1f5fbf")
    s.text(446, 292, "AB", size=12, color="#1f5fbf")

    s.wire(or2[2], (770, 225))
    s.port(90, 60, "A (IN_1)")
    s.port(90, 140, "B (IN_2)")
    s.port(770, 225, "Y1 (LED1)", side="right")
    return s.to_svg()


def schematic_circuit2():
    s = Schematic(540, 180, "회로 2:  Y2 = A + B'")
    not_b = s.not_gate(170, 130, "U2A 74LS04", pins=(1, 2))
    or1 = s.or_gate(300, 95, "U5A 74LS32", pins=(1, 2, 3))
    s.wire((90, 70), (260, 70), (260, 85), or1[0])
    s.wire((90, 130), not_b[0])
    s.wire(not_b[1], (270, 130), (270, 105), or1[1])
    s.text(225, 122, "B'", size=13, color="#1f5fbf")
    s.wire(or1[2], (410, 95))
    s.port(90, 70, "A (IN_1)")
    s.port(90, 130, "B (IN_2)")
    s.port(410, 95, "Y2 (LED2)", side="right")
    return s.to_svg()


INPUTS = [("A", "A (IN_1)"), ("B", "B (IN_2)")]
OUTPUTS = [("Y1", "Y1 (LED1)"), ("Y2", "Y2 (LED2)")]


def main():
    print("Y1 = A'B' + AB' + AB,  Y2 = A + B'\n")
    print(" A B | Y1 Y2")
    print("-----+------")
    for a, b, y1, y2 in truth_table():
        print(f" {a} {b} |  {y1}  {y2}{'   일치' if y1 == y2 else '   불일치!'}")

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUT_DIR / "circuit1.svg").write_text(schematic_circuit1(), encoding="utf-8")
    (OUT_DIR / "circuit2.svg").write_text(schematic_circuit2(), encoding="utf-8")

    c = lab_run(ideal=True)
    wave = dict(t0=0, t1=T_END_NS, grid_ns=A_PERIOD_NS // 2, unit="µs", unit_ns=1000)
    files = {
        "input_waveform.svg": (INPUTS, "입력 파형  (IN_1: 주기 1µs, IN_2: 주기 2µs, 시작값 LOW)"),
        "output_waveform.svg": (OUTPUTS, "출력 파형"),
        "all_waveform.svg": (INPUTS + OUTPUTS, "입력·출력 파형  (Y1 = Y2 = A + B')"),
    }
    for name, (signals, title) in files.items():
        (OUT_DIR / name).write_text(render_svg(c, signals, title=title, **wave), encoding="utf-8")

    # 실제 IC 지연을 넣으면 A, B가 동시에 바뀌는 2µs 지점에서 글리치가 생긴다
    c = lab_run(ideal=False)
    (OUT_DIR / "glitch_zoom.svg").write_text(
        render_svg(
            c,
            INPUTS + [("NA", "A'"), ("NB", "B'"), ("S1", "A'B'+AB'"), ("P3", "AB")] + OUTPUTS,
            t0=1990,
            t1=2070,
            grid_ns=10,
            title="2µs 확대 (실제 IC 지연 적용): AB 11→00 순간의 글리치",
        ),
        encoding="utf-8",
    )
    for net in ("Y1", "Y2"):
        glitches = glitch_widths(c.nets[net].history)
        print(f"{net} 글리치(ns): {glitches}")

    print(f"\n그림 저장: {OUT_DIR}")


if __name__ == "__main__":
    main()
