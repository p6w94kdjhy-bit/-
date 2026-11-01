#!/usr/bin/env python3
"""Korean market (KOSPI/KOSDAQ) daily-close setup scan.

Universe: the top N stocks by market cap per market from Naver's mobile
stock API (preferred shares and non-stock items dropped). Daily closes come
from Yahoo (.KS / .KQ) and the setups are trader-scanner's find_setups. For
each hit it also replays the same setup over the last two years: how often
the close five sessions later was higher, and the average move.

  python3 jobs/kr_scan.py                 # top 200 + 200
  python3 jobs/kr_scan.py --top 100 --json
"""
import argparse
import datetime
import importlib.util
import json
import os
import sys
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
NAVER = "https://m.stock.naver.com/api/stocks/marketValue/%s?page=%d&pageSize=100"


def load_scan():
    for base in (os.path.join(ROOT, ".claude", "skills"), os.path.expanduser("~/.claude/skills")):
        path = os.path.join(base, "trader-scanner", "scan.py")
        if os.path.exists(path):
            spec = importlib.util.spec_from_file_location("trader_scan", path)
            mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(mod)
            return mod
    sys.exit("trader-scanner/scan.py not found under .claude/skills")


def naver_universe(top):
    out = []
    for market, suffix in (("KOSPI", "KS"), ("KOSDAQ", "KQ")):
        got = 0
        for page in range(1, (top + 99) // 100 + 1):
            req = urllib.request.Request(NAVER % (market, page), headers={"User-Agent": "Mozilla/5.0"})
            with urllib.request.urlopen(req, timeout=20) as r:
                doc = json.loads(r.read().decode("utf-8"))
            for st in doc.get("stocks") or []:
                name, code = st.get("stockName", ""), st.get("itemCode", "")
                if st.get("stockEndType") != "stock" or not code.isdigit() or name.endswith(("우", "우B", "우C")):
                    continue
                if got < top:
                    out.append({"symbol": "%s.%s" % (code, suffix), "code": code, "name": name, "market": market,
                                "cap_won": int(st.get("marketValueRaw") or 0)})
                    got += 1
    return out


def history(scan, closes, setup, ahead=5):
    """Every past day the setup fired (no overlap): close `ahead` sessions later vs that close."""
    rets, i = [], 30
    while i < len(closes) - ahead:
        tags, _ = scan.find_setups(closes[:i + 1])
        if setup in tags:
            rets.append(closes[i + ahead] / closes[i] - 1)
            i += ahead
        else:
            i += 1
    return rets


def plan(setup, closes, highs, lows):
    """Entry, stop and target (2R, or the first resistance for a bounce)."""
    last = closes[-1]
    if setup in ("oversold", "oversold_bounce"):
        n = len(lows)
        swing = [lows[k] for k in range(max(5, n - 250), n - 2)
                 if lows[k] == min(lows[k - 5:k + 6]) and lows[k] < last]
        near = [x for x in swing if x >= last * 0.85]
        if near:
            top = max(near)
            zl, zh = min(x for x in near if x >= top * 0.97), top
        else:
            zl = zh = lows[-1]
        zl = min(zl, lows[-1])
        entry, stop = zh, zl * 0.99
        ma20 = sum(closes[-20:]) / 20
        res = [x for x in [ma20] + [highs[k] for k in range(max(5, n - 60), n - 1)
                                    if highs[k] == max(highs[k - 5:k + 6])] if x > entry * 1.01]
        kind = "지지 반등"
    else:
        entry = highs[-1] * 1.001
        stop = min(lows[-5:] if setup == "bull_flag" else lows[-3:]) * 0.99
        res = []
        kind = {"bull_flag": "깃발 돌파", "breakout_20d": "20일 고점 돌파", "reclaim_ma20": "20일선 회복"}[setup]
    risk = entry - stop
    target = min([entry + 2 * risk] + [min(res)] if res else [entry + 2 * risk])
    return {"kind": kind, "entry": entry, "stop": stop, "target": target,
            "r": (target - entry) / risk if risk > 0 else 0, "stop_pct": risk / entry * 100 if entry else 0}


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--top", type=int, default=200, help="stocks per market by market cap (default 200)")
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args(argv)
    scan = load_scan()
    uni = naver_universe(a.top)
    # Yahoo's spark endpoint has no Korean tickers, so one v8 chart per stock
    kst = datetime.timezone(datetime.timedelta(hours=9))
    today = datetime.datetime.now(kst)
    hits, failed = [], {}
    for k, u in enumerate(uni):
        if k:
            scan.SLEEP(scan.PAUSE)
        try:
            # scan.chart_path would turn .KS into -KS, which Yahoo does not know
            path = "/v8/finance/chart/%s?range=2y&interval=1d" % u["symbol"]
            ch = scan.parse_chart(scan.yahoo_get(path), u["symbol"])
        except (scan.ScanError, ValueError) as e:
            failed[u["symbol"]] = str(e)
            continue
        # today's bar counts only once the 15:30 KST close has passed
        done = today.hour * 60 + today.minute >= 15 * 60 + 30
        bars = [(c, h, l) for t, c, h, l in zip(ch["t"], ch["c"], ch["h"], ch["l"])
                if c and h and l and (done or datetime.datetime.fromtimestamp(t, kst).date() < today.date())]
        if len(bars) < 60:
            continue
        C, H, L = [b[0] for b in bars], [b[1] for b in bars], [b[2] for b in bars]
        closes = C
        tags, nums = scan.find_setups(closes)
        for setup in tags:
            rets = history(scan, C, setup)
            hits.append(dict(u, setup=setup, close=closes[-1], rsi=round(nums["rsi"] or 0, 1),
                             n=len(rets), win=round(100 * sum(r > 0 for r in rets) / len(rets)) if rets else None,
                             avg=round(100 * sum(rets) / len(rets), 2) if rets else None,
                             **plan(setup, C, H, L)))
    if a.json:
        print(json.dumps({"scanned": len(uni) - len(failed), "failed": sorted(failed), "hits": hits},
                         ensure_ascii=False, indent=1))
    else:
        print("Scanned %d of %d (%d failed); %d setup hits." % (len(uni) - len(failed), len(uni), len(failed), len(hits)))
        for h in sorted(hits, key=lambda h: -(h["avg"] or -99)):
            print("%-10s %-12s %-8s %-14s close %10s RSI %5.1f | past n=%d win %s%% avg %s%% | entry %s stop %s (%.1f%%) target %s (%.1fR)" % (
                h["symbol"], h["name"], h["market"], h["setup"], "{:,.0f}".format(h["close"]), h["rsi"], h["n"], h["win"],
                h["avg"], "{:,.0f}".format(h["entry"]), "{:,.0f}".format(h["stop"]), h["stop_pct"],
                "{:,.0f}".format(h["target"]), h["r"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
