"""Usage: python3 transcribe.py <audio> <out.txt> <prompt_file>"""
import sys, time, shutil, subprocess
from faster_whisper import WhisperModel


def has_nvidia():
    if not shutil.which("nvidia-smi"):
        return False
    return subprocess.run(["nvidia-smi"], capture_output=True).returncode == 0


def ts(s):
    h, r = divmod(int(s), 3600)
    m, sec = divmod(r, 60)
    return f"{h:02d}:{m:02d}:{sec:02d}"


audio, out, prompt_file = sys.argv[1:4]
prompt = open(prompt_file, encoding="utf-8").read().strip()
device, ctype = ("cuda", "float16") if has_nvidia() else ("cpu", "int8")
print(f"device={device} compute_type={ctype}", flush=True)

import numpy as np

pcm = subprocess.run(
    ["ffmpeg", "-nostdin", "-v", "error", "-i", audio, "-ac", "1", "-ar", "16000",
     "-f", "s16le", "-"],
    capture_output=True, check=True,
).stdout
samples = np.frombuffer(pcm, np.int16).astype(np.float32) / 32768.0

model = WhisperModel("large-v3", device=device, compute_type=ctype)
segments, info = model.transcribe(
    samples,
    language="ko",
    initial_prompt=prompt,
    beam_size=5,
    vad_filter=True,
    condition_on_previous_text=False,
)
print(f"duration={info.duration:.0f}s", flush=True)
t0 = time.time()
with open(out, "w", encoding="utf-8") as f:
    for seg in segments:
        line = f"[{ts(seg.start)} → {ts(seg.end)}] {seg.text.strip()}"
        f.write(line + "\n")
        f.flush()
        print(f"{seg.end/info.duration*100:5.1f}% | {line}", flush=True)
print(f"done in {time.time()-t0:.0f}s", flush=True)
