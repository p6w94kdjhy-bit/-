"""ngspice로 lab_a_plus_not_b.cir를 돌리고, 결과를 SPICE 파형 뷰어 스타일 PNG로 그린다.

실행: 저장소 루트에서  python3 spice/plot_waveforms.py   (ngspice, matplotlib 필요)
"""

import subprocess
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter, MultipleLocator

ROOT = Path(__file__).resolve().parent.parent
NETLIST = ROOT / "spice" / "lab_a_plus_not_b.cir"
DATA = ROOT / "spice" / "out" / "lab_tran.txt"
OUT_DIR = ROOT / "examples" / "output" / "lab_a_plus_not_b" / "spice"

# wrdata 열 순서: (t, V(in_a)), (t, V(in_b)), (t, V(out_y1)), (t, V(out_y2))
TRACES = {"A": 1, "B": 3, "Y1": 5, "Y2": 7}
LABELS = {"A": "V(A)  IN_1", "B": "V(B)  IN_2", "Y1": "V(Y1)  LED1", "Y2": "V(Y2)  LED2"}
COLORS = {"A": "#00ff00", "B": "#ff2020", "Y1": "#4d8dff", "Y2": "#ffff00"}

BG = "#000000"
FG = "#e8e8e8"
GRID = "#5a5a5a"


def run_ngspice():
    DATA.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(["ngspice", "-b", str(NETLIST)], cwd=ROOT, check=True, capture_output=True)


def load():
    rows = [list(map(float, line.split())) for line in DATA.read_text().splitlines() if line.strip()]
    t = [r[0] for r in rows]
    return t, {name: [r[col] for r in rows] for name, col in TRACES.items()}


def time_formatter(decimals):
    def fmt(v, _):
        if abs(v) < 1e-15:
            return "0s"
        return f"{v * 1e6:.{decimals}f}us"

    return FuncFormatter(fmt)


def plot(t, v, names, t0, t1, path, major, decimals):
    n = len(names)
    height = 1.55 * n + 0.75
    fig, axes = plt.subplots(n, 1, sharex=True, figsize=(11, height), dpi=220)
    if n == 1:
        axes = [axes]
    fig.patch.set_facecolor(BG)

    for ax, name in zip(axes, names):
        ax.set_facecolor(BG)
        ts = [x for x in t if t0 <= x <= t1]
        lo = t.index(ts[0])
        ax.plot(ts, v[name][lo:lo + len(ts)], color=COLORS[name], linewidth=1.3)
        ax.set_xlim(t0, t1)
        ax.set_ylim(-0.5, 5.5)
        ax.yaxis.set_major_locator(MultipleLocator(2.5))
        ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: f"{y:.1f}V"))
        ax.xaxis.set_major_locator(MultipleLocator(major))
        ax.xaxis.set_major_formatter(time_formatter(decimals))
        ax.grid(True, color=GRID, linestyle=":", linewidth=0.8)
        ax.tick_params(colors=FG, labelsize=8, length=3)
        for spine in ax.spines.values():
            spine.set_color(FG)
            spine.set_linewidth(0.8)
        ax.text(
            0.003, 0.96, "■", transform=ax.transAxes, color=COLORS[name],
            fontsize=9, va="top", ha="left",
        )
        ax.text(
            0.018, 0.96, LABELS[name], transform=ax.transAxes, color=FG,
            fontsize=9, va="top", ha="left", family="DejaVu Sans Mono",
        )

    axes[-1].set_xlabel("Time", color=FG, fontsize=9)
    axes[-1].text(-0.075, 0.5, "SEL>>", transform=axes[-1].transAxes, color=FG,
                  fontsize=7, va="center", family="DejaVu Sans Mono")
    # 위 0.15in, 아래 0.6in(시간축 눈금과 "Time")만 남긴다
    fig.subplots_adjust(left=0.08, right=0.985, top=1 - 0.15 / height, bottom=0.6 / height,
                        hspace=0.12)
    fig.savefig(path, facecolor=BG)
    plt.close(fig)
    print(path.relative_to(ROOT))


def main():
    run_ngspice()
    t, v = load()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    full = dict(t0=0, t1=4e-6, major=0.5e-6, decimals=1)
    plot(t, v, ["A", "B"], path=OUT_DIR / "input_waveform.png", **full)
    plot(t, v, ["Y1", "Y2"], path=OUT_DIR / "output_waveform.png", **full)
    plot(t, v, ["A", "B", "Y1", "Y2"], path=OUT_DIR / "all_waveform.png", **full)
    plot(t, v, ["A", "B", "Y1", "Y2"], path=OUT_DIR / "glitch_zoom.png",
         t0=1.99e-6, t1=2.07e-6, major=0.01e-6, decimals=2)


if __name__ == "__main__":
    main()
