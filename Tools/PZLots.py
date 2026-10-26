"""Read Project Zomboid map cells (.lotheader + .lotpack) from a LOCAL install and cut out whole buildings.

    python -I Tools/PZLots.py probe  <map folder> [cell]        list buildings/rooms in the busiest cells
    python -I Tools/PZLots.py export <map folder> <cell> <n>... write buildings as lots to ExternalAssets/PZ/lots/

A lot is the same shape our game builds from: squares with tile stacks (floor, walls on N/W edges, objects)
plus named rooms. Only file layouts are read; no game code. Output stays local (ExternalAssets is gitignored).
"""
import json
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "ExternalAssets", "PZ", "lots")


class R:
    def __init__(self, data): self.d, self.p = data, 0
    def i(self): v = struct.unpack_from("<i", self.d, self.p)[0]; self.p += 4; return v
    def b(self): v = self.d[self.p]; self.p += 1; return v
    def line(self):
        e = self.d.index(b"\n", self.p); s = self.d[self.p:e].decode("latin-1"); self.p = e + 1; return s


def read_header(path):
    """LOTH v1: tile names (newline separated), chunk w/h, min/max level, rooms, buildings, zombie density."""
    r = R(open(path, "rb").read())
    assert r.d[:4] == b"LOTH"; r.p = 4
    version = r.i()
    tiles = [r.line() for _ in range(r.i())]
    cw, ch = r.i(), r.i()
    lmin, lmax = r.i(), r.i()
    rooms = []
    for _ in range(r.i()):
        name = r.line(); level = r.i()
        rects = [(r.i(), r.i(), r.i(), r.i()) for _ in range(r.i())]
        objs = [(r.i(), r.i(), r.i()) for _ in range(r.i())]   # type, x, y
        rooms.append({"name": name, "level": level, "rects": rects, "objects": len(objs)})
    buildings = [[r.i() for _ in range(r.i())] for _ in range(r.i())]
    return {"version": version, "tiles": tiles, "chunk": (cw, ch), "levels": (lmin, lmax), "rooms": rooms, "buildings": buildings}


def read_squares(path, hdr, want):
    """LOTP v1: chunk count, int64 offsets, then per chunk per level per square: count (-1 = skip n),
    count > 1 means room id followed by count-1 tile indices. Returns {(x, y, z): [tile names]} for wanted chunks."""
    d = open(path, "rb").read()
    assert d[:4] == b"LOTP"
    _version, n = struct.unpack_from("<ii", d, 4)
    offs = [struct.unpack_from("<q", d, 12 + i * 8)[0] for i in range(n)]
    per = int(n ** 0.5)
    cw, _ = hdr["chunk"]
    lmin, lmax = hdr["levels"]
    out = {}
    for ci in want:
        r = R(d); r.p = offs[ci]
        cx, cy = ci // per, ci % per
        skip = 0
        for z in range(lmin, lmax + 1):
            for x in range(cw):
                for y in range(cw):
                    if skip > 0: skip -= 1; continue
                    count = r.i()
                    if count == -1: skip = r.i() - 1; continue
                    if count > 1:
                        r.i()   # room id
                        out[(cx * cw + x, cy * cw + y, z)] = [hdr["tiles"][r.i()] for _ in range(count - 1)]
    return out


def building_bounds(hdr, bi):
    xs, ys = [], []
    for ri in hdr["buildings"][bi]:
        for (x, y, w, h) in hdr["rooms"][ri]["rects"]:
            xs += [x, x + w]; ys += [y, y + h]
    return min(xs), min(ys), max(xs), max(ys)


def main():
    mode, folder = sys.argv[1], sys.argv[2]
    if mode == "probe":
        cells = sorted((f for f in os.listdir(folder) if f.endswith(".lotheader")), key=lambda f: -os.path.getsize(os.path.join(folder, f)))
        if len(sys.argv) > 3: cells = [sys.argv[3] + ".lotheader"]
        for f in cells[:6]:
            h = read_header(os.path.join(folder, f))
            print(f, "v", h["version"], "levels", h["levels"], "rooms", len(h["rooms"]), "buildings", len(h["buildings"]), "tiles", len(h["tiles"]))
            for bi in range(min(8, len(h["buildings"]))):
                names = sorted({h["rooms"][ri]["name"] for ri in h["buildings"][bi]})
                print("  ", bi, building_bounds(h, bi), names)
        return
    cell, picks = sys.argv[3], [int(v) for v in sys.argv[4:]]
    h = read_header(os.path.join(folder, cell + ".lotheader"))
    os.makedirs(OUT, exist_ok=True)
    for bi in picks:
        x0, y0, x1, y1 = building_bounds(h, bi)
        x0, y0, x1, y1 = x0 - 2, y0 - 2, x1 + 2, y1 + 2   # a little yard around it
        cw = h["chunk"][0]
        per = 256 // cw
        want = sorted({(x // cw) * per + (y // cw) for x in range(x0, x1) for y in range(y0, y1)})
        sq = read_squares(os.path.join(folder, "world_" + cell + ".lotpack"), h, want)
        squares = [{"x": x - x0, "y": y - y0, "z": z, "tiles": t} for (x, y, z), t in sq.items() if x0 <= x < x1 and y0 <= y < y1]
        rooms = [{"name": h["rooms"][ri]["name"], "level": h["rooms"][ri]["level"],
                  "rects": [(x - x0, y - y0, w, hh) for (x, y, w, hh) in h["rooms"][ri]["rects"]]} for ri in h["buildings"][bi]]
        lot = {"source": f"{os.path.basename(folder)} {cell} #{bi}", "w": x1 - x0, "h": y1 - y0, "rooms": rooms, "squares": squares}
        name = f"{os.path.basename(folder).split(',')[0].lower()}_{cell}_{bi}.json"
        json.dump(lot, open(os.path.join(OUT, name), "w"))
        print("wrote", name, lot["w"], "x", lot["h"], len(squares), "squares,", sorted({r["name"] for r in rooms}))


if __name__ == "__main__":
    main()
