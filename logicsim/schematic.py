"""게이트 수준 회로도를 SVG로 그린다 (외부 라이브러리 없음).

좌표는 직접 지정한다. 게이트 메서드는 입력/출력 핀 좌표를 돌려주므로
그 좌표를 wire()에 넘겨 배선한다.
"""

from xml.sax.saxutils import escape

INK = "#222"
WIRE = "#1f5fbf"
PIN = "#c0392b"


class Schematic:
    def __init__(self, width, height, title=None):
        self.width = width
        self.height = height
        self.items = []
        if title:
            self.text(16, 28, title, size=16, bold=True)

    def text(self, x, y, s, size=13, anchor="start", color=INK, bold=False):
        weight = ' font-weight="bold"' if bold else ""
        self.items.append(
            f'<text x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" '
            f'fill="{color}"{weight}>{escape(s)}</text>'
        )

    def wire(self, *points):
        d = " L".join(f"{x},{y}" for x, y in points)
        self.items.append(
            f'<path d="M{d}" stroke="{WIRE}" stroke-width="2" fill="none" stroke-linejoin="round"/>'
        )

    def dot(self, x, y):
        self.items.append(f'<circle cx="{x}" cy="{y}" r="4" fill="{WIRE}"/>')

    def port(self, x, y, label, side="left"):
        """입력/출력 단자: 작은 원과 이름."""
        self.items.append(
            f'<circle cx="{x}" cy="{y}" r="5" fill="#fff" stroke="{INK}" stroke-width="2"/>'
        )
        if side == "left":
            self.text(x - 10, y + 5, label, size=15, anchor="end", bold=True)
        else:
            self.text(x + 10, y + 5, label, size=15, bold=True)

    def _body(self, d):
        self.items.append(f'<path d="{d}" stroke="{INK}" stroke-width="2" fill="#fff"/>')

    def _pin_numbers(self, ins, out, pins, in_offset=0):
        if not pins:
            return
        *in_pins, out_pin = pins
        for (x, y), p in zip(ins, in_pins):
            self.text(x - 4 - in_offset, y - 4, str(p), size=10, anchor="end", color=PIN)
        self.text(out[0] + 4, out[1] - 4, str(out_pin), size=10, color=PIN)

    def _ref(self, x, y, ref):
        if ref:
            self.text(x, y, ref, size=11, anchor="middle", color="#555")

    def and_gate(self, gx, cy, ref=None, pins=None):
        """2입력 AND. (입력1, 입력2, 출력) 좌표를 돌려준다."""
        self._body(
            f"M{gx},{cy - 20} H{gx + 30} A20,20 0 0 1 {gx + 30},{cy + 20} H{gx} Z"
        )
        ins = [(gx, cy - 10), (gx, cy + 10)]
        out = (gx + 50, cy)
        self._pin_numbers(ins, out, pins)
        self._ref(gx + 25, cy + 36, ref)
        return (*ins, out)

    def or_gate(self, gx, cy, ref=None, pins=None):
        """2입력 OR. (입력1, 입력2, 출력) 좌표를 돌려준다."""
        self._body(
            f"M{gx},{cy - 20} Q{gx + 35},{cy - 20} {gx + 55},{cy} "
            f"Q{gx + 35},{cy + 20} {gx},{cy + 20} Q{gx + 12},{cy} {gx},{cy - 20} Z"
        )
        ins = [(gx + 5, cy - 10), (gx + 5, cy + 10)]
        out = (gx + 55, cy)
        self._pin_numbers(ins, out, pins, in_offset=8)  # 뒤쪽 곡선과 겹치지 않게
        self._ref(gx + 27, cy + 36, ref)
        return (*ins, out)

    def not_gate(self, x, cy, ref=None, pins=None):
        """NOT. (입력, 출력) 좌표를 돌려준다."""
        self._body(f"M{x},{cy - 14} L{x + 28},{cy} L{x},{cy + 14} Z")
        self.items.append(
            f'<circle cx="{x + 32}" cy="{cy}" r="4" stroke="{INK}" stroke-width="2" fill="#fff"/>'
        )
        inp = (x, cy)
        out = (x + 36, cy)
        self._pin_numbers([inp], out, pins)
        self._ref(x + 16, cy + 30, ref)
        return inp, out

    def to_svg(self):
        return "\n".join(
            [
                f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.width}" '
                f'height="{self.height}" viewBox="0 0 {self.width} {self.height}" '
                f'font-family="sans-serif">',
                f'<rect width="{self.width}" height="{self.height}" fill="#ffffff"/>',
                *self.items,
                "</svg>",
            ]
        )
