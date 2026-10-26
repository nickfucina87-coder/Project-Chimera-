"""Extract Project Zomboid tile art and tile properties from a LOCAL install, for private stand-in use.

    python -I Tools/ExtractPZ.py ["C:/Program Files (x86)/Steam/steamapps/common/ProjectZomboid"]

Writes ExternalAssets/PZ/ (gitignored, never committed or shipped; the art belongs to The Indie Stone):
    pages/<pack>_<n>.png   atlas pages copied out of media/texturepacks/Tiles2x(.floor).pack
    atlas.json             sprite name -> page, rect and trim offsets inside its 128x256 frame
    tiles.json             sprite name -> properties from media/newtiledefinitions.tiles.txt
    palette.json           example sprite names per building role (walls, doors, floors, furniture...)

Only the file layouts are read (the same thing PZ's own modding tools do); no game code is used.
"""
import json
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PZ = sys.argv[1] if len(sys.argv) > 1 else r"C:\Program Files (x86)\Steam\steamapps\common\ProjectZomboid"
OUT = os.path.join(ROOT, "ExternalAssets", "PZ")
PNG_SIG = b"\x89PNG\r\n\x1a\n"


def read_pack(path, tag):
    """PZPK v1: magic, version, page count; per page: name, entry count, alpha flag, entries
    (name, x, y, w, h, offset x, offset y, frame w, frame h), then the page's PNG."""
    data = open(path, "rb").read()
    assert data[:4] == b"PZPK", path
    pos = 4
    version, pages = struct.unpack_from("<ii", data, pos)
    pos += 8
    atlas = {}
    for p in range(pages):
        n = struct.unpack_from("<i", data, pos)[0]; pos += 4
        page_name = data[pos:pos + n].decode("latin-1"); pos += n
        count, _alpha = struct.unpack_from("<ii", data, pos); pos += 8
        entries = []
        for _ in range(count):
            n = struct.unpack_from("<i", data, pos)[0]; pos += 4
            name = data[pos:pos + n].decode("latin-1"); pos += n
            entries.append((name,) + struct.unpack_from("<8i", data, pos)); pos += 32
        start = data.index(PNG_SIG, pos)
        end = data.index(b"IEND", start) + 8
        file = f"{tag}_{p}.png"
        with open(os.path.join(OUT, "pages", file), "wb") as f:
            f.write(data[start:end])
        for (name, x, y, w, h, ox, oy, fw, fh) in entries:
            atlas[name] = {"page": file, "x": x, "y": y, "w": w, "h": h, "ox": ox, "oy": oy, "fw": fw, "fh": fh}
        pos = end
        # some versions put a 4-byte marker after the PNG
        if data[pos:pos + 4] == b"\xef\xbe\xad\xde":
            pos += 4
    print(f"{os.path.basename(path)}: v{version}, {pages} pages, {len(atlas)} sprites")
    return atlas


def read_tiles(path):
    """newtiledefinitions.tiles.txt: tileset { file = name ... tile { xy = c,r  Key = Value ... } }"""
    tiles, sheet, cols, cur = {}, None, 8, None
    for raw in open(path, encoding="utf-8", errors="replace"):
        line = raw.strip()
        if line.startswith("file = "):
            sheet = line[7:].strip()
        elif line.startswith("size = "):
            cols = int(line[7:].split(",")[0])
        elif line.startswith("xy = ") and sheet:
            c, r = (int(v) for v in line[5:].split(","))
            cur = {}
            tiles[f"{sheet}_{r * cols + c}"] = cur
        elif cur is not None and " = " in line and not line.startswith("//"):
            k, v = line.split(" = ", 1)
            cur[k.strip()] = v.strip()
        elif line.endswith("=") and cur is not None:
            cur[line[:-1].strip()] = ""
    return tiles


def first(tiles, atlas, pattern, need=(), avoid=()):
    rx = re.compile(pattern)
    for name in sorted(tiles, key=lambda s: (re.sub(r"_\d+$", "", s), int(re.search(r"(\d+)$", s).group(1)))):
        p = tiles[name]
        if name in atlas and rx.match(name) and all(k in p for k in need) and not any(k in p for k in avoid):
            return name
    return None


def main():
    os.makedirs(os.path.join(OUT, "pages"), exist_ok=True)
    atlas = {}
    for pack, tag in (("Tiles2x.pack", "t"), ("Tiles2x.floor.pack", "f")):
        path = os.path.join(PZ, "media", "texturepacks", pack)
        if os.path.exists(path):
            atlas.update(read_pack(path, tag))
    tiles = read_tiles(os.path.join(PZ, "media", "newtiledefinitions.tiles.txt"))
    json.dump(atlas, open(os.path.join(OUT, "atlas.json"), "w"))
    json.dump({k: v for k, v in tiles.items() if k in atlas}, open(os.path.join(OUT, "tiles.json"), "w"))

    # a starter palette for one wooden cottage: picked by the properties PZ's engine reads
    pal = {
        "wallN": first(tiles, atlas, r"walls_exterior_house_01_", ("WallN",)),
        "wallW": first(tiles, atlas, r"walls_exterior_house_01_", ("WallW",)),
        "wallNW": first(tiles, atlas, r"walls_exterior_house_01_", ("WallNW",)),
        "wallSE": first(tiles, atlas, r"walls_exterior_house_01_", ("WallSE",)),
        "doorframeN": first(tiles, atlas, r"walls_exterior_house_01_", ("DoorWallN",)),
        "doorframeW": first(tiles, atlas, r"walls_exterior_house_01_", ("DoorWallW",)),
        "windowN": first(tiles, atlas, r"walls_exterior_house_01_", ("WindowN",)),
        "windowW": first(tiles, atlas, r"walls_exterior_house_01_", ("WindowW",)),
        "doorN": first(tiles, atlas, r"fixtures_doors_01_", ("DoorN",)) or first(tiles, atlas, r"fixtures_doors_01_", ("doorN",)),
        "doorW": first(tiles, atlas, r"fixtures_doors_01_", ("DoorW",)) or first(tiles, atlas, r"fixtures_doors_01_", ("doorW",)),
        "floorWood": first(tiles, atlas, r"floors_interior_tilesandwood_01_"),
        "grass": first(tiles, atlas, r"blends_natural_01_"),
        "dirt": first(tiles, atlas, r"blends_natural_01_", (), ()),
        "bed": first(tiles, atlas, r"furniture_bedding_01_"),
        "table": first(tiles, atlas, r"furniture_tables_(high|low)_01_"),
        "chair": first(tiles, atlas, r"furniture_seating_indoor_01_"),
        "storage": first(tiles, atlas, r"furniture_storage_01_", ("ContainerType",)),
        "stove": first(tiles, atlas, r"appliances_cooking_01_"),
        "fence": first(tiles, atlas, r"fencing_01_"),
    }
    json.dump(pal, open(os.path.join(OUT, "palette.json"), "w"), indent=1)
    print(f"{len(atlas)} sprites, {len(tiles)} tile definitions")
    print(json.dumps(pal, indent=1))


if __name__ == "__main__":
    main()
