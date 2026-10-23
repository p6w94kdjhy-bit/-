"""시뮬레이션 결과를 SVG 타이밍 다이어그램으로 그린다 (외부 라이브러리 없음)."""

from xml.sax.saxutils import escape

LABEL_W = 110
ROW_H = 44
WAVE_H = 24
TOP = 30
BOTTOM = 36
RIGHT = 20


def _segments(net, t0, t1):
    """[t0, t1] 구간을 (시작, 끝, 값) 조각으로 나눈다."""
    value = None
    for t, v in net.history:
        if t <= t0:
            value = v
    points = [(t0, value)]
    for t, v in net.history:
        if not t0 < t < t1:
            continue
        if points[-1][0] == t:
            points.pop()  # 같은 시각의 변화(지연 0 글리치)는 마지막 값만 남김
        if points and points[-1][1] == v:
            continue
        points.append((t, v))
    segs = []
    for i, (t, v) in enumerate(points):
        end = points[i + 1][0] if i + 1 < len(points) else t1
        segs.append((t, end, v))
    return segs


def render_svg(circuit, signals, t0, t1, title, grid_ns, width=960, unit="ns", unit_ns=1):
    """signals: [(넷 이름, 표시 이름), ...]. 시간축은 unit_ns 단위로 나눠 unit으로 표시."""
    plot_w = width - LABEL_W - RIGHT
    height = TOP + ROW_H * len(signals) + BOTTOM
    scale = plot_w / (t1 - t0)

    def x(t):
        return LABEL_W + (t - t0) * scale

    out = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}" font-family="monospace" font-size="13">',
        f'<rect width="{width}" height="{height}" fill="#ffffff"/>',
        f'<text x="{LABEL_W}" y="18" font-size="14" font-weight="bold" fill="#222">{escape(title)}</text>',
    ]

    t = t0
    axis_y = TOP + ROW_H * len(signals)
    while t <= t1:
        out.append(
            f'<line x1="{x(t):.1f}" y1="{TOP}" x2="{x(t):.1f}" y2="{axis_y}" stroke="#e3e3e3"/>'
        )
        out.append(
            f'<text x="{x(t):.1f}" y="{axis_y + 16}" text-anchor="middle" fill="#666">{t / unit_ns:g}</text>'
        )
        t += grid_ns
    out.append(
        f'<text x="{LABEL_W + plot_w}" y="{axis_y + 32}" text-anchor="end" fill="#666">시간 ({unit})</text>'
    )

    for row, (name, label) in enumerate(signals):
        net = circuit.nets[name]
        hi = TOP + row * ROW_H + (ROW_H - WAVE_H) / 2
        lo = hi + WAVE_H
        out.append(
            f'<text x="8" y="{(hi + lo) / 2 + 4:.1f}" fill="#222">{escape(label)}</text>'
        )
        path = []
        for start, end, v in _segments(net, t0, t1):
            if v is None:
                out.append(
                    f'<rect x="{x(start):.1f}" y="{hi:.1f}" width="{x(end) - x(start):.1f}" '
                    f'height="{WAVE_H}" fill="#f0c0c0"/>'
                )
                continue
            y = hi if v == 1 else lo
            path.append(f"M{x(start):.1f},{y:.1f} H{x(end):.1f}")
            if end < t1:
                path.append(f"M{x(end):.1f},{hi:.1f} V{lo:.1f}")
        out.append(
            f'<path d="{" ".join(path)}" stroke="#1f5fbf" stroke-width="2" fill="none"/>'
        )

    out.append("</svg>")
    return "\n".join(out)
