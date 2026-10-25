"""Server hall build to WAREHOUSE_BIBLE rules. Re-runnable: deletes SEPH_SV_* first.
Hall: x -4480..480 (walls), y -2960..1980 (walls), floor -76, roof trusses from 1209, roof ~1774."""
import unreal, math, collections
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ROOT = "Facility/ServerHall/SEPH"
FLOOR = -76.0
WX0, WX1, WY0, WY1 = -4480.0, 480.0, -2960.0, 1980.0
TRAY_Z = 270.0          # top of row cable trays (racks top out at 138)
STRUT_Z = 667.0         # ceiling strut grid (hangs under the main beams)
MAIN_Z = 700.0          # main beams along Y, framed into the pillars
DUCT_Z = 1050.0         # cooling ducts, below the roof trusses (1209)
def M(p): return unreal.load_asset(p if p.startswith("/") else "/Game/Meshes/" + p)
for a in EAS.get_all_level_actors():
    if a.get_actor_label().startswith("SEPH_SV_"): EAS.destroy_actor(a)
acts = EAS.get_all_level_actors()

cnt = collections.Counter()
def put(mesh, loc, rot=(0, 0, 0), scale=(1, 1, 1), tag="x", sub=""):
    a = EAS.spawn_actor_from_object(M(mesh), unreal.Vector(*loc), unreal.Rotator(*rot))
    a.set_actor_scale3d(unreal.Vector(*scale)); cnt[tag] += 1
    a.set_actor_label(f"SEPH_SV_{tag}_{cnt[tag]}"); a.set_folder_path(ROOT + ("/" + sub if sub else ""))
    return a

# ---- existing layout
bb = lambda a: (lambda o, e: (o.x - e.x, o.y - e.y, o.z - e.z, o.x + e.x, o.y + e.y, o.z + e.z))(*a.get_actor_bounds(False))
racks = [a for a in acts if a.get_actor_label().startswith("FAC_RackRow")]
pillars = [bb(a) for a in acts if a.get_actor_label().startswith("FAC_Pillar") and isinstance(a, unreal.StaticMeshActor)]
catwalk = [bb(a) for a in acts if str(a.get_folder_path()) == "Facility/Overhead/Catwalks"]
strips = [a for a in acts if a.get_actor_label().startswith("FAC_LightStrip")]
ys = sorted({round(a.get_actor_bounds(False)[0].y) for a in racks})
pairs = []
i = 0
while i < len(ys) - 1:
    if abs(ys[i + 1] - ys[i] - 112) < 20: pairs.append((ys[i] + ys[i + 1]) / 2); i += 2
    else: i += 1
aisles = [(pairs[k] + pairs[k + 1]) / 2 for k in range(len(pairs) - 1)]
RX0 = min(bb(a)[0] for a in racks); RX1 = max(bb(a)[3] for a in racks)
print("pairs", [round(p) for p in pairs], "rack x", round(RX0), round(RX1))
def hits(box_list, x0, y0, z0, x1, y1, z1, pad=5):
    return any(b[0] - pad < x1 and x0 < b[3] + pad and b[1] - pad < y1 and y0 < b[4] + pad and b[2] - pad < z1 and z0 < b[5] + pad for b in box_list)

# ---- helpers for linear runs
def run(mesh, axis, a0, a1, fixed, piece, rot, other_scale, tag, sub, along_index):
    """Chain `mesh` (native length `piece` along its local axis `along_index`) from a0 to a1 on world `axis` ('x'/'y')."""
    k = max(1, round((a1 - a0) / piece)); L = (a1 - a0) / k
    for j in range(k):
        s = list(other_scale); s[along_index] = L / piece
        p = (a0 + j * L, fixed[0], fixed[1]) if axis == "x" else (fixed[0], a0 + j * L, fixed[1])
        put(mesh, p, rot, tuple(s), tag, sub)
def rod(x, y, z0, z1, tag="Rod", sub="Hangers", w=2.5):
    if z1 <= z0: return
    put("SM_MetalBeam10", (x, y, z0), scale=(w / 111, w / 112, (z1 - z0) / 800), tag=tag, sub=sub)

# 1) Ceiling layer A: main beams along Y framed into the pillars (MetalBeam01 600 long on X -> yaw 90)
px = sorted({round((p[0] + p[3]) / 2) for p in pillars})
main_x = []
for x in px:
    if not main_x or x - main_x[-1] > 300: main_x.append(x)
for x in main_x:
    run("SM_MetalBeam01", "y", WY0, WY1, (x, MAIN_Z), 600, (0, 0, 90), (1, 1, 1), "MainBeam", "Ceiling", 0)
print("main beams at x", main_x)
# 2) Ceiling layer B: struts along X over every rack pair and aisle, hanging just under the mains (MetalBeam15 1200 on X)
strut_y = sorted(pairs + aisles)
for y in strut_y:
    yy = y + (35 if y == pairs[0] else 0)
    run("SM_MetalBeam15", "x", WX0, WX1, (yy, STRUT_Z), 1200, (0, 0, 0), (1, 0.12, 0.15), "Strut", "Ceiling", 0)

# 3) Row cable trays (wire-mesh tray = SM_Fence_01 rolled 270: x -128..0 along run, width 0..101 on +Y, z -4..0)
TW = 0.7                                                   # tray width 71 cm
tray_y = {}
for y in pairs:
    yc = y + (35 if y == pairs[0] else 0); tray_y[y] = yc        # first pair dodges the catwalk posts
    y0 = yc - 50.5 * TW
    k = max(1, round((150 - RX0) / 128)); L = (150 - RX0) / k
    for j in range(k):
        put("SM_Fence_01", (RX0 + (j + 1) * L, y0, TRAY_Z), (270, 0, 0), (L / 128, 1, TW), "Tray", "Trays")
    for dy in (-15, 15):                                      # two cable bundles lying in the tray (Cables04_06: 402 on -Y -> yaw 90 runs +X)
        run("SM_Cables04_06", "x", RX0, 150, (yc + dy, TRAY_Z + 4), 402, (0, 0, 90), (0.6, 1, 1), "TrayCable", "Trays", 1)
    put("SM_Cables04_04", (RX0, yc, TRAY_Z + 4), (0, 0, -90), tag="TrayEnd", sub="Trays")   # end cap at the first rack
    for x in range(int(RX0) + 150, 150, 400):                  # hangers from both tray edges up to the strut
        for ey in (yc - 34, yc + 34):
            if not hits(catwalk, x - 2, ey - 2, TRAY_Z, x + 2, ey + 2, STRUT_Z):
                rod(x, ey, TRAY_Z, STRUT_Z - 4)
# collector tray along Y at x 150 that every row tray tees into
k = max(1, round((tray_y[pairs[-1]] + 40 - (tray_y[pairs[0]] - 40)) / 128)); ya, yb = tray_y[pairs[0]] - 40, tray_y[pairs[-1]] + 40
L = (yb - ya) / k
for j in range(k):
    put("SM_Fence_01", (150 + 71, ya + (j + 1) * L, TRAY_Z), (270, 0, 90), (L / 128, 1, TW), "Collector", "Trays")
for dx in (-20, 20):
    run("SM_Cables04_06", "y", ya, yb, (185 + dx, TRAY_Z + 4), 402, (0, 0, 180), (0.6, 1, 1), "CollectorCable", "Trays", 1)

# 4) Risers: cable sets from every rack row up into its tray (CableSet_02: 86 x 17 x 200, z 0..200)
for a in racks:
    o, e = a.get_actor_bounds(False)
    yc = min(tray_y.values(), key=lambda t: abs(t - o.y))
    for dx in (-150, 150):
        put("SM_CableSet_02", (o.x + dx, (o.y + yc) / 2, o.z + e.z), (0, 0, 0), (0.8, 1, (TRAY_Z - 4 - (o.z + e.z)) / 200), "Riser", "Risers")

# 5) Backup power: UPS / switchgear along the east wall, fed from the collector, feeding the power room through the wall
UPS_X = 455.0
ups_y = [y for y in range(-2600, 1800, 150) if not (-1950 < y < -1350 or 550 < y < 1150)]   # skip the two doorways
for y in ups_y:
    put("SM_ElectricalSupply_Switchboard01", (UPS_X, y, FLOOR), (0, 0, 90), tag="UPS", sub="BackupPower")
for y in ups_y[::2]:                                          # collector -> UPS: short tray stubs + drops onto cabinet tops
    run("SM_Cables04_06", "x", 185, UPS_X, (y, TRAY_Z + 4), 402, (0, 0, 90), (0.6, 1, 1), "UPSFeed", "BackupPower", 1)
    put("SM_CableSet_02", (UPS_X - 10, y, FLOOR + 200), (0, 0, 90), (0.6, 1, (TRAY_Z - FLOOR - 200) / 200), "UPSDrop", "BackupPower")
for y in (-2300, -900, 200, 1500):                             # UPS -> up the wall -> through to the power room
    put("SM_CableSet_02", (WX1 - 12, y, FLOOR + 200), (0, 0, 90), (0.8, 1, (MAIN_Z - FLOOR - 200) / 200), "PowerRiser", "BackupPower")
run("SM_Cables04_06", "y", -2600, 1700, (WX1 - 30, MAIN_Z + 30), 402, (0, 0, 180), (1.2, 1, 1), "PowerMain", "BackupPower", 1)
for y in (-2200, 0):
    put("SM_ElectricalSupply_Group04", (WX1 - 8, y, 640), (0, 0, 90), tag="WallPanel", sub="BackupPower")

# 6) Light strips: two hanger wires each, up to the aisle strut
WIRE = "/Game/Scene_Warehouse/Assets/Custom/Ind_War_Cable/SM_Ind_War_Wire"
for s in strips:
    o, e = s.get_actor_bounds(False)
    for dx in (-e.x * 0.8, e.x * 0.8):
        x, y, z0 = o.x + dx, o.y, o.z + e.z
        if not hits(catwalk, x - 2, y - 2, z0, x + 2, y + 2, STRUT_Z):
            put(WIRE, (x, y, z0), scale=(1, 1, (STRUT_Z - 4 - z0) / 60), tag="LightWire", sub="Hangers")

# 7) Cooling ducts along X over aisles clear of pillars (pipe_conditioning01_01: 80 dia, 600 on Y -> yaw 90 runs along X)
nd = 0
for y in aisles[::2]:
    if hits(pillars, WX0, y - 60, DUCT_Z - 60, WX1, y + 60, DUCT_Z + 60): continue
    run("SM_pipe_conditioning01_01", "x", WX0, WX1, (y, DUCT_Z), 600, (0, 0, 90), (1.4, 1, 1.4), "Duct", "Cooling", 1)
    for x in range(int(WX0) + 300, int(WX1), 400):
        put("SM_Pipe_round_vent", (x, y, DUCT_Z - 56), scale=(1.6, 1.6, 1), tag="Vent", sub="Cooling")
        rod(x + 100, y, DUCT_Z + 56, 1209)
    nd += 1
print("ducts", nd)
# CRAC units along the north wall + exhaust fans/vents high on the long walls
for x in (-4000, -3300, -2600, -1100, -400):
    put("SM_AirConditioner01", (x, WY1 - 40, FLOOR), (0, 0, 180), tag="CRAC", sub="Cooling")
for x in range(-4100, 200, 700):
    put("SM_AirVentilation01", (x, WY1 - 8, 900), (0, 0, 180), (2, 1, 2), "WallFan", "Cooling")
    put("SM_AirVentilation01", (x, WY0 + 8, 900), (0, 0, 0), (2, 1, 2), "WallFan", "Cooling")

# 8) Wall signs (hanging-style zone signs, 168 x 15 x 112, top at z 0)
signs = ["SM_Sign15", "SM_Sign16", "SM_Sign17", "SM_Sign18", "SM_Sign19", "SM_Sign20"]
for i, x in enumerate((-3600, -2400, -1000, 0)):
    put(signs[i % len(signs)], (x, WY1 - 9, 520), (0, 0, 180), tag="Sign", sub="Signs")
for i, x in enumerate((-3000, -1500, -300)):
    put(signs[(i + 3) % len(signs)], (x, WY0 + 9, 780), (0, 0, 0), tag="Sign", sub="Signs")

# 9) Aisle padding (FloorMatsMerged01: 801 x 201 on X, z 0..4)
for y in aisles:
    run("SM_FloorMatsMerged01", "x", RX0, RX1, (y, FLOOR + 0.5), 801, (0, 0, 0), (1, 0.85, 1), "AisleMat", "Floor", 0)
gx = sorted({round(a.get_actor_bounds(False)[0].x) for a in racks})
gap = max(zip(gx, gx[1:]), key=lambda p: p[1] - p[0])          # central cross aisle
run("SM_FloorMatsMerged01", "y", pairs[0] - 112, pairs[-1] + 112, ((gap[0] + gap[1]) / 2, FLOOR + 0.5), 801, (0, 0, 90), (1, 1, 1), "AisleMat", "Floor", 0)

# 10) Catwalk: the west rail ran across the stair landing - shorten it so the stair opens onto the platform
by = {a.get_actor_label(): a for a in EAS.get_all_level_actors()}
st, rl = by.get("FAC_CatStair_351"), by.get("FAC_RailWest_318")
if st and rl:
    sy0 = bb(st)[1]; r0 = bb(rl)
    if r0[4] > sy0:
        rl.set_actor_scale3d(unreal.Vector(1, (sy0 - r0[1]) / (r0[4] - r0[1]), 1))
        r1 = bb(rl); rl.set_actor_location(rl.get_actor_location() + unreal.Vector(0, r0[1] - r1[1], 0), False, False)
        print("stair rail now y", [round(v) for v in bb(rl)[1::3]][:2])

print(dict(cnt))
print("saved", unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True))
