import sys, importlib.util, re
fix, src, dst = sys.argv[1:4]
spec = importlib.util.spec_from_file_location("f", fix); m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
lines = open(src, encoding="utf-8").read().splitlines()
# drop consecutive duplicate texts (whisper repetition)
out = []; prev = None
for l in lines:
    t = l.split("] ", 1)[-1]
    if t == prev: continue
    out.append(l); prev = t
text = "\n".join(out)
miss = []
for a, b in m.R:
    if a not in text: miss.append(a)
    text = text.replace(a, b)
open(dst, "w", encoding="utf-8").write(text + "\n")
print("unmatched:", miss)
print("[?] count:", text.count("[?"))
