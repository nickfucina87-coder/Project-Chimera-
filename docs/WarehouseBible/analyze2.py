"""Extract placement rules from the factory sample dump."""
import json, collections, math, re, statistics as st
R = json.load(open("sample_dump.json"))
A = []
for r in R:
    if r["class"] != "StaticMeshActor" or not r["comps"] or "mesh" not in r["comps"][0]: continue
    r["mesh"] = r["comps"][0]["mesh"].split("/")[-1]; A.append(r)
def cat(m):
    for k, p in (("cable", r"Cable|Wire"), ("pipe", r"Pipe_round|Pipes_|Sewer"), ("duct", r"conditioning|AirConditioningPipe"), ("lamp", r"Lamp"),
                 ("tray", r"Tray|MetalFarm"), ("beam", r"Beam|Pillar|Arch|Truss|Frame"), ("wall", r"Wall|Window|Curtain"), ("floor", r"Floor|Path|Mat|Carpet|Grill|Platform"),
                 ("elec", r"Electrical|Switch|Junction|Socket|HV"), ("box", r"Box|Pallet|Container|Barrel|bin_|Crate"), ("shelf", r"Shelf|Rack|Storage")):
        if re.search(p, m, re.I): return k
    return "other"
for r in A: r["cat"] = cat(r["mesh"])
FLOOR = st.median(r["bmin"][2] for r in A if r["cat"] == "floor")
print("floor z ~", FLOOR)
print("\n== mount height (bottom above floor, cm) by category: p10 / median / p90, count")
for c in ("lamp", "cable", "pipe", "duct", "tray", "elec", "box"):
    zs = sorted(r["bmin"][2] - FLOOR for r in A if r["cat"] == c)
    if zs: print(f"  {c:6s} {zs[len(zs)//10]:7.0f} {zs[len(zs)//2]:7.0f} {zs[9*len(zs)//10]:7.0f}  n={len(zs)}")

def touch(a, b, tol):
    return all(b["bmin"][i] - tol <= a["bmax"][i] and a["bmin"][i] - tol <= b["bmax"][i] for i in range(3))
grid = collections.defaultdict(list)
for i, r in enumerate(A):
    for gx in range(int(r["bmin"][0] // 500), int(r["bmax"][0] // 500) + 1):
        for gy in range(int(r["bmin"][1] // 500), int(r["bmax"][1] // 500) + 1):
            grid[(gx, gy)].append(i)
def neighbours(i, tol=10):
    r = A[i]; seen = set()
    for gx in range(int((r["bmin"][0] - tol) // 500), int((r["bmax"][0] + tol) // 500) + 1):
        for gy in range(int((r["bmin"][1] - tol) // 500), int((r["bmax"][1] + tol) // 500) + 1):
            for j in grid[(gx, gy)]:
                if j != i and j not in seen and touch(r, A[j], tol): seen.add(j); yield A[j]

print("\n== what CABLES touch (share of cables touching each category)")
cab = [i for i, r in enumerate(A) if r["cat"] == "cable"]
tc = collections.Counter(); alone = 0
for i in cab:
    cs = {n["cat"] for n in neighbours(i)}
    if not cs - {"cable"}: alone += 1
    tc.update(cs)
for k, v in tc.most_common(): print(f"  {k:6s} {100*v/len(cab):5.1f}%")
print(f"  cables touching nothing but other cables: {100*alone/len(cab):.1f}%  (n={len(cab)})")

print("\n== cable orientation by mount height (roll,pitch rounded to 90)")
oc = collections.Counter()
for i in cab:
    r = A[i]; h = r["bmin"][2] - FLOOR
    band = "floor" if h < 30 else ("wall" if h < 350 else "ceiling")
    oc[(band, round(r["rot"][0] / 90) * 90 % 360, round(r["rot"][1] / 90) * 90 % 360)] += 1
for k, v in oc.most_common(15): print("  ", k, v)

print("\n== PIPE connectivity: pipe pieces whose both long-axis ends touch another pipe/duct/wall/beam/elec piece")
pp = [i for i, r in enumerate(A) if r["cat"] in ("pipe", "duct")]
both = one = 0
for i in pp:
    r = A[i]; ext = [r["bmax"][k] - r["bmin"][k] for k in range(3)]; ax = ext.index(max(ext))
    ends = 0
    for side in (0, 1):
        e = dict(r); e["bmin"] = list(r["bmin"]); e["bmax"] = list(r["bmax"])
        if side == 0: e["bmax"][ax] = r["bmin"][ax] + 5
        else: e["bmin"][ax] = r["bmax"][ax] - 5
        if any(touch(e, n, 8) and n["cat"] in ("pipe", "duct", "wall", "beam", "elec", "floor", "tray", "other") for n in neighbours(i, 15)): ends += 1
    both += ends == 2; one += ends == 1
print(f"  both ends {100*both/len(pp):.1f}%  one end {100*one/len(pp):.1f}%  n={len(pp)}")

print("\n== lamp spacing (nearest same-mesh neighbour, cm) and height")
for m in ("SM_LampSet_01", "SM_Lamp_with_Rope01", "SM_Lamp02"):
    L = [r for r in A if r["mesh"] == m]
    if len(L) < 2: continue
    d = sorted(min(math.dist(a["loc"][:2], b["loc"][:2]) for b in L if b is not a) for a in L)
    print(f"  {m:22s} n={len(L)} spacing median {d[len(d)//2]:.0f}  height median {st.median(r['bmin'][2]-FLOOR for r in L):.0f}")

print("\n== shelving: items stacked on each shelf unit")
sh = [i for i, r in enumerate(A) if re.search(r"MetalFarm|Shelf|Rack", r["mesh"], re.I)]
fill = collections.Counter(); per = []
for i in sh:
    r = A[i]; inside = [A[j] for j in grid[(int(r["loc"][0] // 500), int(r["loc"][1] // 500))]
              if A[j]["cat"] in ("box", "other", "elec") and all(r["bmin"][k] - 5 <= A[j]["loc"][k] <= r["bmax"][k] + 5 for k in range(3))]
    per.append(len(inside)); fill.update(x["mesh"] for x in inside)
print(f"  units {len(sh)}  items per unit median {st.median(per) if per else 0}  max {max(per) if per else 0}")
print("  most used shelf items:", fill.most_common(15))

print("\n== density: objects per 10x10 m cell in the busiest interior cells (meshes / decals / lights)")
D = collections.Counter((int(r["loc"][0] // 1000), int(r["loc"][1] // 1000)) for r in A)
dec = collections.Counter((int(r["loc"][0] // 1000), int(r["loc"][1] // 1000)) for r in R if r["class"] == "DecalActor")
lit = collections.Counter((int(r["loc"][0] // 1000), int(r["loc"][1] // 1000)) for r in R if "Light" in r["class"])
top = [c for c, _ in D.most_common(40)]
print("  median meshes", st.median(D[c] for c in top), " decals", st.median(dec[c] for c in top), " lights", st.median(lit[c] for c in top))
