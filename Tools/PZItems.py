"""List every Project Zomboid item from a LOCAL install as a rename sheet for an Emberfall total-conversion mod.

    python -I Tools/PZItems.py ["C:/Program Files (x86)/Steam/steamapps/common/ProjectZomboid"]

Writes ExternalAssets/PZ/items.csv (local; PZ data): id, English name, category, type, weight, source file,
plus empty columns to fill in (Emberfall name, reskin notes, keep/cut).
"""
import csv
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PZ = sys.argv[1] if len(sys.argv) > 1 else r"C:\Program Files (x86)\Steam\steamapps\common\ProjectZomboid"
SCRIPTS = os.path.join(PZ, "media", "scripts")


def names():
    """English display names from the translation tables (ItemName_EN / ItemName.json style files)."""
    out = {}
    for base, _, files in os.walk(os.path.join(PZ, "media", "lua", "shared", "Translate", "EN")):
        for f in files:
            if not f.lower().startswith("itemname"):
                continue
            if f.lower().endswith(".json"):   # Build 42: {"Base.Axe": "Axe", ...}
                import json
                for k, v in json.load(open(os.path.join(base, f), encoding="utf-8", errors="replace")).items():
                    out[k.split(".")[-1]] = v
                continue
            for line in open(os.path.join(base, f), encoding="utf-8", errors="replace"):
                m = re.search(r'"?ItemName_(?:Base\.)?([\w.]+)"?\s*[=:]\s*"([^"]*)"', line)
                if m:
                    out[m.group(1).split(".")[-1]] = m.group(2)
    return out


def main():
    en = names()
    rows = []
    for base, _, files in os.walk(SCRIPTS):
        for f in files:
            if not f.endswith(".txt"):
                continue
            text = open(os.path.join(base, f), encoding="utf-8", errors="replace").read()
            for m in re.finditer(r"^\s*item\s+(\w+)\s*\{(.*?)^\s*\}", text, re.S | re.M):
                iid, body = m.group(1), m.group(2)
                field = lambda k: (re.search(rf"^\s*{k}\s*=\s*([^,\n]+)", body, re.M) or [None, ""])[1].strip()
                rows.append({
                    "id": iid,
                    "english_name": en.get(iid) or field("DisplayName") or iid,
                    "category": field("DisplayCategory"),
                    "type": field("ItemType") or field("Type"),
                    "weight": field("Weight"),
                    "file": os.path.relpath(os.path.join(base, f), SCRIPTS),
                    "emberfall_name": "", "reskin_notes": "", "keep_cut": "",
                })
    rows.sort(key=lambda r: (r["category"], r["english_name"]))
    out = os.path.join(ROOT, "ExternalAssets", "PZ", "items.csv")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    cats = {}
    for r in rows:
        cats[r["category"]] = cats.get(r["category"], 0) + 1
    print(len(rows), "items ->", out)
    print(sorted(cats.items(), key=lambda kv: -kv[1])[:25])


if __name__ == "__main__":
    main()
