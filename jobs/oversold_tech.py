#!/usr/bin/env python3
"""Tech-stock RSI(14) oversold check for the scheduled phone alerts.

Reuses trader-scanner's Yahoo fetch and Wilder RSI. During the session the
live price stands in for today's close, so two numbers are shown:
  rsi_prev  RSI on completed daily closes (through yesterday)
  rsi_live  RSI with the live price as today's provisional close
A symbol is listed when either is under the threshold (default 30).

  python3 jobs/oversold_tech.py                  # jobs/tech.txt
  python3 jobs/oversold_tech.py --symbols AAPL,NVDA --threshold 35 --json
  python3 jobs/oversold_tech.py --volume-of trader/watchlist.json   # rank hits by volume
"""
import argparse
import datetime
import importlib.util
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)


def load_scan():
    for base in (os.path.join(ROOT, ".claude", "skills"), os.path.expanduser("~/.claude/skills")):
        path = os.path.join(base, "trader-scanner", "scan.py")
        if os.path.exists(path):
            spec = importlib.util.spec_from_file_location("trader_scan", path)
            mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(mod)
            return mod
    sys.exit("trader-scanner/scan.py not found under .claude/skills")


def read_list(path):
    out = []
    with open(path) as f:
        for line in f:
            sym = line.split("#")[0].split("|")[0].strip().upper()
            if sym and sym not in out:
                out.append(sym)
    return out


def check(scan, symbols, threshold):
    now = datetime.datetime.now(datetime.timezone.utc)
    data, failed = scan.fetch_spark(symbols, "3mo", "1d", scan.PAUSE)
    rows = []
    for sym in symbols:
        s = data.get(sym)
        if not s:
            continue
        closes = [c for _, c in scan.daily_closes(s, now)]
        meta = s.get("meta") or {}
        price = meta.get("regularMarketPrice")
        if len(closes) < 20 or price is None:
            continue
        r_prev = scan.rsi(closes)
        r_live = scan.rsi(closes + [float(price)])
        if r_prev is None or r_live is None:
            continue
        rows.append({"symbol": sym, "price": float(price), "prev_close": closes[-1],
                     "change_pct": (float(price) / closes[-1] - 1) * 100,
                     "rsi_prev": round(r_prev, 1), "rsi_live": round(r_live, 1),
                     "volume": meta.get("regularMarketVolume"),
                     "oversold": r_prev < threshold or r_live < threshold})
    return rows, failed, scan.to_eastern(now)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--symbols", help="comma-separated tickers instead of --list")
    ap.add_argument("--list", default=os.path.join(HERE, "tech.txt"))
    ap.add_argument("--threshold", type=float, default=30.0)
    ap.add_argument("--volume-of", help="a watchlist.json: print its hits ranked by today's volume")
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args(argv)
    scan = load_scan()

    if a.volume_of:
        hits = {h["symbol"]: h for h in json.load(open(a.volume_of))["hits"]}
        data, _ = scan.fetch_spark(sorted(hits), "1d", "5m", scan.PAUSE)
        for sym, h in hits.items():
            h["volume"] = ((data.get(sym) or {}).get("meta") or {}).get("regularMarketVolume")
        ranked = sorted(hits.values(), key=lambda h: -(h["volume"] or 0))
        if a.json:
            print(json.dumps(ranked, indent=1))
        else:
            for h in ranked:
                print("%-6s %12s shares  $%-9.2f %s  +%.2f%% over %.2f" % (
                    h["symbol"], "{:,}".format(h["volume"] or 0), h["price"], h["at"], h["pct_over"], h["or_high"]))
        return 0

    symbols = [s.strip().upper() for s in a.symbols.split(",")] if a.symbols else read_list(a.list)
    rows, failed, now_et = check(scan, symbols, a.threshold)
    hits = sorted((r for r in rows if r["oversold"]), key=lambda r: min(r["rsi_prev"], r["rsi_live"]))
    if a.json:
        print(json.dumps({"as_of": now_et.strftime("%Y-%m-%d %H:%M ET"), "threshold": a.threshold,
                          "requested": len(symbols), "checked": len(rows), "failed": sorted(failed),
                          "hits": hits}, indent=1))
    else:
        print("%s: RSI(14) under %g, %d of %d tech symbols checked (%d failed)." % (
            now_et.strftime("%Y-%m-%d %H:%M ET"), a.threshold, len(rows), len(symbols), len(failed)))
        for r in hits:
            print("%-6s $%-9.2f %+6.2f%%  RSI prev %5.1f  live %5.1f  vol %s" % (
                r["symbol"], r["price"], r["change_pct"], r["rsi_prev"], r["rsi_live"],
                "{:,}".format(r["volume"] or 0)))
        if not hits:
            print("No tech symbol is oversold right now.")
    return 0 if hits else 1


if __name__ == "__main__":
    sys.exit(main())
