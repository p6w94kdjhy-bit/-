"""게이트 수준 회로도를 SVG로 그린다 (외부 라이브러리 없음).

좌표는 직접 지정한다. 게이트 메서드는 입력/출력 핀 좌표를 돌려주므로
그 좌표를 wire()에 넘겨 배선한다.

theme="classic": 흰 배경의 단순한 그림
theme="eda":     EDA 회로도 프로그램 스타일 (점 격자, 도면 테두리, 포트 기호, 표제란)
"""

from xml.sax.saxutils import escape

THEMES = {
    "classic": dict(ink="#222", wire="#1f5fbf", pin="#c0392b", ref="#555", net="#1f5fbf",
                    body_fill="#fff", stroke=2),
    "eda": dict(ink="#800000", wire="#000080", pin="#000000", ref="#000000", net="#0000c0",
                body_fill="#ffffe8", stroke=1.6),
}
EDA_MARGIN = 60  # 도면 테두리와 포트 이름이 들어갈 왼쪽 여백
EDA_TITLE_BLOCK_H = 96


class Schematic:
    def __init__(self, width, height, title=None, theme="classic", sheet=None, date=None):
        self.theme = theme
        self.c = THEMES[theme]
        self.title = title
        self.sheet = sheet
        self.date = date
        self.items = []
        if theme == "eda":
            self.width = width + EDA_MARGIN
            self.height = height + EDA_TITLE_BLOCK_H + 20
        else:
            self.width = width
            self.height = height
            if title:
                self.text(16, 28, title, size=16, bold=True)

    def text(self, x, y, s, size=13, anchor="start", color=None, bold=False):
        weight = ' font-weight="bold"' if bold else ""
        self.items.append(
            f'<text x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" '
            f'fill="{color or self.c["ink"]}"{weight}>{escape(s)}</text>'
        )

    def net_label(self, x, y, s):
        self.text(x, y, s, size=12 if self.theme == "eda" else 13, color=self.c["net"])

    def wire(self, *points):
        d = " L".join(f"{x},{y}" for x, y in points)
        self.items.append(
            f'<path d="M{d}" stroke="{self.c["wire"]}" stroke-width="{self.c["stroke"]}" '
            f'fill="none" stroke-linejoin="round"/>'
        )

    def dot(self, x, y):
        self.items.append(f'<circle cx="{x}" cy="{y}" r="4" fill="{self.c["wire"]}"/>')

    def port(self, x, y, label, side="left"):
        """입력/출력 단자. side는 이름이 놓일 쪽."""
        if self.theme == "eda":
            # 오른쪽을 가리키는 오각형 포트 기호
            x0 = x - 34 if side == "left" else x
            self.items.append(
                f'<path d="M{x0},{y - 8} H{x0 + 24} L{x0 + 34},{y} L{x0 + 24},{y + 8} H{x0} Z" '
                f'stroke="{self.c["ink"]}" stroke-width="1.4" fill="{self.c["body_fill"]}"/>'
            )
            if side == "left":
                self.text(x0 - 6, y + 5, label, size=13, anchor="end", color="#000")
            else:
                self.text(x0 + 40, y + 5, label, size=13, color="#000")
            return
        self.items.append(
            f'<circle cx="{x}" cy="{y}" r="5" fill="#fff" stroke="{self.c["ink"]}" stroke-width="2"/>'
        )
        if side == "left":
            self.text(x - 10, y + 5, label, size=15, anchor="end", bold=True)
        else:
            self.text(x + 10, y + 5, label, size=15, bold=True)

    def _body(self, d):
        self.items.append(
            f'<path d="{d}" stroke="{self.c["ink"]}" stroke-width="{self.c["stroke"]}" '
            f'fill="{self.c["body_fill"]}"/>'
        )

    def _pin_numbers(self, ins, out, pins, in_offset=0):
        if not pins:
            return
        *in_pins, out_pin = pins
        for (x, y), p in zip(ins, in_pins):
            self.text(x - 4 - in_offset, y - 4, str(p), size=10, anchor="end", color=self.c["pin"])
        self.text(out[0] + 4, out[1] - 4, str(out_pin), size=10, color=self.c["pin"])

    def _ref(self, x, top, bottom, ref):
        """ref는 "U1A 74LS04" 형식. eda 테마는 부품 번호를 위, 부품명을 아래에 쓴다."""
        if not ref:
            return
        if self.theme == "eda":
            refdes, _, value = ref.partition(" ")
            self.text(x, top - 6, refdes, size=11, anchor="middle", color=self.c["ref"], bold=True)
            if value:
                self.text(x, bottom + 15, value, size=11, anchor="middle", color=self.c["ref"])
        else:
            self.text(x, bottom + 16, ref, size=11, anchor="middle", color=self.c["ref"])

    def and_gate(self, gx, cy, ref=None, pins=None):
        """2입력 AND. (입력1, 입력2, 출력) 좌표를 돌려준다."""
        self._body(
            f"M{gx},{cy - 20} H{gx + 30} A20,20 0 0 1 {gx + 30},{cy + 20} H{gx} Z"
        )
        ins = [(gx, cy - 10), (gx, cy + 10)]
        out = (gx + 50, cy)
        self._pin_numbers(ins, out, pins)
        self._ref(gx + 25, cy - 20, cy + 20, ref)
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
        self._ref(gx + 27, cy - 20, cy + 20, ref)
        return (*ins, out)

    def not_gate(self, x, cy, ref=None, pins=None):
        """NOT. (입력, 출력) 좌표를 돌려준다."""
        self._body(f"M{x},{cy - 14} L{x + 28},{cy} L{x},{cy + 14} Z")
        self.items.append(
            f'<circle cx="{x + 32}" cy="{cy}" r="4" stroke="{self.c["ink"]}" '
            f'stroke-width="{self.c["stroke"]}" fill="{self.c["body_fill"]}"/>'
        )
        inp = (x, cy)
        out = (x + 36, cy)
        self._pin_numbers([inp], out, pins)
        self._ref(x + 16, cy - 14, cy + 14, ref)
        return inp, out

    def _eda_frame(self):
        w, h = self.width, self.height
        out = [
            '<defs><pattern id="grid" width="10" height="10" patternUnits="userSpaceOnUse">'
            '<circle cx="5" cy="5" r="0.7" fill="#c8c8c8"/></pattern></defs>',
            f'<rect width="{w}" height="{h}" fill="#ffffff"/>',
            f'<rect x="8" y="8" width="{w - 16}" height="{h - 16}" fill="url(#grid)" '
            f'stroke="#000" stroke-width="1.2"/>',
        ]
        # 도면 구역 표시 (위쪽 1, 2, 3 ...)
        zones = max(2, (w - 16) // 220)
        zw = (w - 16) / zones
        for i in range(zones):
            zx = 8 + i * zw
            if i:
                out.append(f'<line x1="{zx:.1f}" y1="8" x2="{zx:.1f}" y2="16" stroke="#000"/>')
            out.append(
                f'<text x="{zx + zw / 2:.1f}" y="17" font-size="8" text-anchor="middle">{zones - i}</text>'
            )
        # 표제란
        bw, bh = 360, EDA_TITLE_BLOCK_H - 10
        bx, by = w - 8 - bw, h - 8 - bh
        row = bh / 3
        out.append(f'<rect x="{bx}" y="{by}" width="{bw}" height="{bh}" fill="#fff" stroke="#000"/>')
        for i in (1, 2):
            out.append(f'<line x1="{bx}" y1="{by + i * row:.1f}" x2="{bx + bw}" y2="{by + i * row:.1f}" stroke="#000"/>')
        out.append(f'<line x1="{bx + 60}" y1="{by + row:.1f}" x2="{bx + 60}" y2="{by + 2 * row:.1f}" stroke="#000"/>')
        out.append(f'<line x1="{bx + 260}" y1="{by + row:.1f}" x2="{bx + 260}" y2="{by + bh}" stroke="#000"/>')

        def cell(x, y, label, value, size=12, bold=False):
            weight = ' font-weight="bold"' if bold else ""
            out.append(f'<text x="{x + 4}" y="{y + 10}" font-size="7.5" fill="#444">{escape(label)}</text>')
            out.append(
                f'<text x="{x + 6}" y="{y + 24}" font-size="{size}"{weight}>{escape(value)}</text>'
            )

        cell(bx, by, "Title", self.title or "", size=13, bold=True)
        cell(bx, by + row, "Size", "A4")
        cell(bx + 60, by + row, "Document Number", "LOGIC-LAB")
        cell(bx + 260, by + row, "Rev", "1.0")
        cell(bx, by + 2 * row, "Date", self.date or "")
        cell(bx + 260, by + 2 * row, "Sheet", self.sheet or "1 of 1")
        return out

    def to_svg(self):
        head = (
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.width}" '
            f'height="{self.height}" viewBox="0 0 {self.width} {self.height}" '
            f'font-family="Arial, Helvetica, sans-serif">'
        )
        if self.theme == "eda":
            body = [
                *self._eda_frame(),
                f'<g transform="translate({EDA_MARGIN},0)">',
                *self.items,
                "</g>",
            ]
        else:
            body = [f'<rect width="{self.width}" height="{self.height}" fill="#ffffff"/>', *self.items]
        return "\n".join([head, *body, "</svg>"])
