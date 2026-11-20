"""Usage: python3 topdf.py <src.md|src.txt> <out.pdf> <title>"""
import sys, html, re, markdown
import subprocess, json, os

src, out, title = sys.argv[1:4]
text = open(src, encoding="utf-8").read()

CSS = """
@page { size: A4; margin: 16mm 14mm 18mm 14mm; }
body { font-family: 'NanumGothic', 'Noto Color Emoji', sans-serif; font-size: 10pt;
       line-height: 1.6; color: #1f2328; background: #fff; }
h1 { font-size: 20pt; border-bottom: 3px solid #2f5d8a; padding-bottom: 6px; color: #1d3c5a; }
h2 { font-size: 15pt; color: #fff; background: #2f5d8a; padding: 6px 10px; border-radius: 4px;
     margin-top: 26px; break-before: page; }
h1 + * h2, body > h2:first-of-type { break-before: auto; }
h3 { font-size: 12.5pt; color: #1d3c5a; border-left: 5px solid #2f5d8a; padding-left: 8px; margin-top: 20px; }
h4 { font-size: 11pt; color: #2f5d8a; margin: 16px 0 6px; }
code { font-family: 'NanumGothicCoding', monospace; font-size: 8.5pt; background: #eef2f6;
       color: #2f5d8a; padding: 1px 4px; border-radius: 3px; }
blockquote { margin: 10px 0; padding: 8px 12px; background: #fff8e6; border-left: 4px solid #e0a800;
             color: #5a4500; }
blockquote p { margin: 3px 0; }
table { border-collapse: collapse; margin: 8px 0; width: 100%; font-size: 9.5pt; break-inside: avoid; }
th, td { border: 1px solid #c8d1db; padding: 4px 8px; text-align: left; vertical-align: top; }
th { background: #e8eef5; }
li { margin: 2px 0; }
hr { border: none; border-top: 1px solid #c8d1db; margin: 20px 0; }
strong { color: #0f2a44; }
/* transcript */
.sec { font-size: 13pt; font-weight: bold; color: #fff; background: #2f5d8a; padding: 5px 10px;
       border-radius: 4px; margin: 18px 0 8px; break-before: page; }
.sec.first { break-before: auto; }
.note { color: #555; font-size: 9pt; margin: 2px 0; }
.ln { display: flex; gap: 8px; font-size: 9.5pt; line-height: 1.5; margin: 1px 0; break-inside: avoid; }
.ts { flex: 0 0 auto; font-family: 'NanumGothicCoding', monospace; font-size: 8pt; color: #6a7a8c; padding-top: 2px; }
.q { background: #fff2b3; border-radius: 2px; padding: 0 2px; }
.bad { color: #999; }
"""

if src.endswith(".md"):
    body = markdown.markdown(text, extensions=["tables", "sane_lists"])
    body = re.sub(r"(\[\?[^\]<]*\])", r'<span class="q">\1</span>', body)
else:
    parts, first = [], True
    for line in text.splitlines():
        if not line.strip():
            continue
        if line.startswith("====="):
            parts.append(f'<div class="sec{" first" if first else ""}">{html.escape(line.strip("= "))}</div>')
            first = False
        elif line.startswith("#"):
            parts.append(f'<div class="note">{html.escape(line.lstrip("# "))}</div>')
        else:
            m = re.match(r"(\[[^\]]+\])\s?(.*)", line)
            ts, t = (m.group(1), m.group(2)) if m else ("", line)
            t = html.escape(t)
            cls = ' class="bad"' if "인식 불가" in t else ""
            t = re.sub(r"(\[\?[^\]]*\])", r'<span class="q">\1</span>', t)
            parts.append(f'<div class="ln"><span class="ts">{html.escape(ts)}</span><span{cls}>{t}</span></div>')
    body = f"<h1>{html.escape(title)}</h1>" + "\n".join(parts)

doc = f"<!doctype html><html lang='ko'><head><meta charset='utf-8'><title>{html.escape(title)}</title><style>{CSS}</style></head><body>{body}</body></html>"
footer = ("<div style='width:100%;font-size:8px;color:#888;font-family:NanumGothic;text-align:center'>"
          f"{html.escape(title)} · <span class='pageNumber'></span> / <span class='totalPages'></span></div>")
hp = out[:-4] + ".tmp.html"
open(hp, "w", encoding="utf-8").write(doc)
js = """
const { chromium } = require('playwright');
(async () => {
  const [hp, out, footer] = JSON.parse(process.argv[1]);
  const b = await chromium.launch();
  const p = await b.newPage();
  await p.goto('file://' + hp, { waitUntil: 'load' });
  await p.pdf({ path: out, format: 'A4', printBackground: true, displayHeaderFooter: true,
    headerTemplate: '<div></div>', footerTemplate: footer,
    margin: { top: '16mm', bottom: '18mm', left: '14mm', right: '14mm' } });
  await b.close();
})();
"""
subprocess.run(["node", "-e", js, json.dumps([hp, out, footer])], check=True, cwd=os.path.dirname(os.path.abspath(__file__)))
os.remove(hp)
print("ok", out)
