# Seph smart placement for Zone E (L_CHM_ComponentTest).
# Manages ONLY actors labeled Seph_Smart_* (deleted + rebuilt every run).
# REMOVE_E2 = True also deletes the older random pass (Seph_E2_*). Lighting / PPV / Astra layout untouched.
import unreal, random
REMOVE_E2 = True
AISLE = 200.0      # clear center aisle width (cm)
WALL_GAP = 15.0    # gap between prop back and wall
random.seed(42)
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
MAP = "/Game/ThirdPerson/L_CHM_ComponentTest"
W = unreal.EditorLevelLibrary.get_editor_world()
if W is None or W.get_path_name().split(".")[0] != MAP:
    LES.load_level(MAP)
A = "/Game/Scene_Warehouse/Assets/MS/3D/"
def ms(n, suf=""): return A + n + "/SM_" + n + suf
D = "/Game/Scene_Warehouse/Assets/MS/Decals/"
RACK   = "/Game/Fab/Server_Rack/server_rack/StaticMeshes/server_rack"
BATT   = "/Game/Fab/Server_Battery_2_Slot/server_battery_2_slot/StaticMeshes/server_battery_2_slot"
MACH   = "/Game/Fab/Industrial_machine/industrial_machine/StaticMeshes/industrial_machine"
SHELF  = [ms("Ind_War_Pallet_Shelf_Metal_Modular_01", "_" + s) for s in "ABCD"] + [ms("Ind_War_Storage_Shelf_Metal_Rusted_02")]
CAB    = ms("Ind_War_Cabinet_Electric_Metal_Dirty_01")
EBOX   = [ms("Ind_Fac_Panel_ElectricalBox_Metal_Worn_03"), ms("Urb_Str_ElectricalBox_Metal_Worn_01")]
CABLE  = [ms("Ind_War_Cable_Power_Rubber_Weathered_03"), ms("Ind_War_Equipment_RadioCable_01")]
LADDER = ms("Ind_War_Equipment_Ladder_Metal_03")
BEAM   = ms("Ind_Con_Supplies_Beam_Metal_Rusty_01")
BULK   = ms("Ind_War_Bulkhead_Metal_Caged_01")
HANG   = [ms("Ind_War_Light_Ceiling_Metal_Hanging_01"), ms("Ind_War_Light_Ceiling_Metal_Hanging_02")]
TABLE  = ms("Ind_War_Furniture_Table_Wood_Worn_01", "_A")
STOOL  = ms("Res_Fur_Stool_Metal_Worn_01")
TOOLB  = ms("Res_Gar_Storage_Toolbox_Wood_Worn_01")
CORNER_SETS = [
    [ms("Ind_Fac_Container_Drum_Metal_Worn_01"), ms("Ind_Fac_Container_Drum_Metal_Worn_02"), ms("Ind_Aba_Storage_Barrel_Metal_Rust_01")],
    [ms("Ind_War_Storage_Crate_Plastic_Teal_01"), ms("Ind_War_Storage_Crate_Plastic_Blue_01"), ms("Ind_War_Storage_Box_Plastic_Blue_04"), ms("Ind_War_Storage_Pallet_Wood_Worn_01")],
    [ms("Ind_Aba_Tank_Gas_Metal_Worn_02"), ms("Ind_War_Container_Jerrycan_Metal_Red_03"), ms("Ind_War_Trash_Bin_Metal_Worn_02", "_A")],
    [ms("Ind_War_Storage_Cabinet_Metal_Rusty_01"), ms("Ind_War_Equipment_Handtruck_Metal_Worn_02", "_A"), TOOLB],
]
DEC_WALL  = [D + "Decal_Leak_Grime_01/MI_Decal_Leak_Grime_01_A", D + "Decal_Leak_Grime_02/MI_Decal_Leak_Grime_02_A"]
DEC_FLOOR = [D + "Decal_Stain_Mud_01/MI_Decal_Stain_Mud_01_A", D + "Decal_Plate_Metal_Rusted_01/MI_Decal_Plate_Metal_Rusted_01"]
DEC_LINE  = D + "Decal_Line_Concrete_01/MI_Decal_Line_Concrete_01"
CUBE   = "/Game/LevelPrototyping/Meshes/SM_Cube"   # catwalk deck/posts only (structure, not props)
DUST   = "/Game/Downwash_Dust_FX"

_cache = {}; missing = []
def ld(p):
    if p in _cache: return _cache[p]
    o = unreal.EditorAssetLibrary.load_asset(p) if unreal.EditorAssetLibrary.does_asset_exist(p) else None
    if not o: missing.append(p); unreal.log_warning("Seph_Smart: missing, skipped " + p)
    _cache[p] = o; return o

def mesh_ext(m):
    b = m.get_bounding_box(); return (b.max.x - b.min.x, b.max.y - b.min.y, b.max.z - b.min.z, b.min.z)

# --- cleanup ---
removed = {"Seph_Smart_": 0, "Seph_E2_": 0}
for a in EAS.get_all_level_actors():
    l = a.get_actor_label()
    if l.startswith("Seph_Smart_"): EAS.destroy_actor(a); removed["Seph_Smart_"] += 1
    elif REMOVE_E2 and l.startswith("Seph_E2_"): EAS.destroy_actor(a); removed["Seph_E2_"] += 1
# E2 spotlight under catwalk had no label prefix in pass2; leave it (it is lighting).

# --- measure room from Astra's (non-Seph) static meshes ---
mn = [1e9]*3; mx = [-1e9]*3
for a in EAS.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor) or a.get_actor_label().startswith("Seph"): continue
    o, e = a.get_actor_bounds(False)
    if max(e.x, e.y) > 20000: continue
    for i, (c, x) in enumerate(((o.x, e.x), (o.y, e.y), (o.z, e.z))):
        mn[i] = min(mn[i], c - x); mx[i] = max(mx[i], c + x)
# wall thickness guess: pull 30cm in from outer bounds
X0, X1, Y0, Y1 = mn[0] + 30, mx[0] - 30, mn[1] + 30, mx[1] - 30
FZ = mn[2]
# floor top: tallest-thin wide mesh near bottom
for a in EAS.get_all_level_actors():
    if isinstance(a, unreal.StaticMeshActor) and not a.get_actor_label().startswith("Seph"):
        o, e = a.get_actor_bounds(False)
        if e.x > (X1-X0)*0.3 and e.y > (Y1-Y0)*0.3 and e.z < 60 and abs((o.z - e.z) - mn[2]) < 5:
            FZ = max(FZ, o.z + e.z)
CEIL = mx[2]
LONG_X = (X1 - X0) >= (Y1 - Y0)
unreal.log("Seph_Smart: room X[%.0f,%.0f] Y[%.0f,%.0f] floor %.0f ceil %.0f long=%s" % (X0, X1, Y0, Y1, FZ, CEIL, "X" if LONG_X else "Y"))

# local frame: u = along long axis, v = across (v0 = 'south' long wall, v1 = 'north' long wall)
U0, U1, V0, V1 = (X0, X1, Y0, Y1) if LONG_X else (Y0, Y1, X0, X1)
def W2(u, v): return (u, v) if LONG_X else (v, u)
def yaw_face(dirn):  # yaw so +X of mesh faces into room from given wall
    base = {"v0": 90, "v1": -90, "u0": 0, "u1": 180}[dirn]
    return base if LONG_X else base - 90

cnt = {}
def put(path, u, v, z, yaw=0.0, label="x", folder="Props", scale=None, flush=None):
    m = ld(path)
    if not m: return None
    if flush:  # push the back of the mesh against a wall using its depth
        ex = mesh_ext(m); depth = min(ex[0], ex[1]) * (scale.x if scale else 1.0)
        off = depth / 2 + WALL_GAP
        if flush == "v0": v = V0 + off
        elif flush == "v1": v = V1 - off
        elif flush == "u0": u = U0 + off
        elif flush == "u1": u = U1 - off
    x, y = W2(u, v)
    a = EAS.spawn_actor_from_object(m, unreal.Vector(x, y, z), unreal.Rotator(0, 0, yaw))
    if not a: return None
    if scale: a.set_actor_scale3d(scale)
    a.set_actor_label("Seph_Smart_%s" % label); a.set_folder_path("Seph/Smart/" + folder)
    a.tags = [unreal.Name("Seph"), unreal.Name("SephSmart")]
    cnt[folder] = cnt.get(folder, 0) + 1
    return a

def decal(path, u, v, z, rot, size, label, folder="Decals"):
    mi = ld(path)
    if not mi: return
    x, y = W2(u, v)
    d = EAS.spawn_actor_from_class(unreal.DecalActor, unreal.Vector(x, y, z), rot)
    d.set_decal_material(mi); d.set_actor_scale3d(size)
    d.set_actor_label("Seph_Smart_" + label); d.set_folder_path("Seph/Smart/" + folder)
    cnt[folder] = cnt.get(folder, 0) + 1

L = U1 - U0; Wd = V1 - V0; VC = (V0 + V1) / 2
with unreal.ScopedEditorTransaction("Seph Smart Place Zone E") as tx:
    # 1) SERVER ROWS: two rows of racks back-to-back down the room center line? No -> keep center aisle clear.
    #    Racks flush to the north long wall (v1) in runs of 4, 120cm service gap between runs.
    rk = ld(RACK)
    rw = mesh_ext(rk)[0] if rk else 60.0
    run, gap = 4, 120.0
    u = U0 + 250; i = 0
    while u + rw < U1 - 250 and rk:
        for k in range(run):
            if u + rw > U1 - 250: break
            put(RACK, u + rw/2, 0, FZ, yaw_face("v1"), "Rack_%02d" % i, "ServerRow", flush="v1"); u += rw; i += 1
        # battery unit closes each run
        put(BATT, u + 40, 0, FZ, yaw_face("v1"), "Batt_%02d" % i, "ServerRow", flush="v1")
        u += 80 + gap
    # 2) SHELVING on south long wall (v0) — catwalk sits above it, so lower half only, middle third left open for a door/sightline.
    s = ld(SHELF[0]); sw = mesh_ext(s)[0] if s else 250.0
    u = U0 + 200; j = 0
    while u + sw < U1 - 200:
        t = (u - U0) / L
        if 0.42 < t < 0.58: u += sw; continue  # breathing space mid-wall
        put(SHELF[j % 4], u + sw/2, 0, FZ, yaw_face("v0"), "Shelf_%02d" % j, "Shelving", flush="v0"); u += sw + 20; j += 1
    # 3) MACHINES: one industrial machine on each short wall, centered, with electric cabinet beside it.
    for end in ("u0", "u1"):
        put(MACH, 0, VC, FZ, yaw_face(end), "Machine_" + end, "Machines", flush=end)
        put(CAB, 0, VC + 260, FZ, yaw_face(end), "MachineCab_" + end, "Machines", flush=end)
        put(EBOX[0 if end == "u0" else 1], 0, VC - 220, FZ + 140, yaw_face(end), "MachineEBox_" + end, "Electrical", flush=end)
        # oil/grime under machine (not on the aisle: machines sit at the room ends)
        uu = U0 + 120 if end == "u0" else U1 - 120
        decal(DEC_FLOOR[1], uu, VC, FZ + 2, unreal.Rotator(-90, 0, 0), unreal.Vector(0.4, 1.2, 1.2), "Decal_MachineBase_" + end)
    # 4) CABLES run along both long walls, aligned to the wall (yaw along u), stepped evenly.
    n = max(3, int(L // 400))
    for k in range(n):
        uu = U0 + L * (k + 0.5) / n
        put(CABLE[0], uu, V0 + 25, FZ + 1, yaw_face("v0") + 90, "Cable_S_%02d" % k, "Cables")
        put(CABLE[1], uu, V1 - 25, FZ + 1, yaw_face("v1") + 90, "Cable_N_%02d" % k, "Cables")
    # 5) CORNER CLUTTER: groups of 2-4 in the 4 corners, varied yaw, tucked 60-160cm from corner.
    corners = [(U0, V0, 1, 1), (U1, V0, -1, 1), (U0, V1, 1, -1), (U1, V1, -1, -1)]
    for ci, (cu, cv, su, sv) in enumerate(corners):
        group = CORNER_SETS[ci % len(CORNER_SETS)]
        k = random.randint(2, min(4, len(group) + 1))
        for g in range(k):
            off_u = 70 + (g % 2) * 80 + random.uniform(-10, 10)
            off_v = 70 + (g // 2) * 80 + random.uniform(-10, 10)
            put(group[g % len(group)], cu + su * off_u, cv + sv * off_v, FZ, random.uniform(0, 360), "Corner%d_%d" % (ci, g), "CornerClutter")
    # 6) WORK STATION: table + stool + toolbox in the shelving gap (mid south wall), facing the room.
    put(TABLE, U0 + L * 0.5, 0, FZ, yaw_face("v0"), "WorkTable", "WorkStation", flush="v0")
    put(STOOL, U0 + L * 0.5 + 40, V0 + 160, FZ, random.uniform(0, 360), "WorkStool", "WorkStation")
    # 7) CATWALK: single run above the shelving (south wall), ladder at each end, rail beams, caged bulkhead lights.
    CWZ = FZ + min(340.0, max(260.0, (CEIL - FZ) * 0.55)); DW = 160.0
    cu = (U0 + U1) / 2
    put(CUBE, cu, V0 + DW/2, CWZ, 0 if LONG_X else 90, "Catwalk_Deck", "Catwalk", unreal.Vector((L - 200) / 100.0, DW / 100.0, 0.1))
    put(BEAM, cu, V0 + DW, CWZ + 100, yaw_face("v0") + 90, "Catwalk_Rail", "Catwalk")
    for k in range(int((L - 200) // 500) + 1):
        put(CUBE, U0 + 100 + k * 500, V0 + DW - 10, FZ, 0, "Catwalk_Post_%d" % k, "Catwalk", unreal.Vector(0.15, 0.15, (CWZ - FZ) / 100.0))
    put(LADDER, U0 + 130, V0 + DW + 30, FZ, yaw_face("v0"), "Ladder_W", "Catwalk")
    put(LADDER, U1 - 130, V0 + DW + 30, FZ, yaw_face("v0"), "Ladder_E", "Catwalk")
    for k in range(3):
        put(BULK, U0 + L * (k + 1) / 4, V0 + 5, CWZ + 180, yaw_face("v0"), "Catwalk_Bulkhead_%d" % k, "Catwalk")
    # 8) HANGING LIGHT FIXTURES (meshes only, no new lights) above aisle centerline.
    for k in range(4):
        put(HANG[k % 2], U0 + L * (k + 0.5) / 4, VC, CEIL - 120, 0, "HangFixture_%d" % k, "Fixtures")
    # 9) WALL decals (grime streaks) on long walls between props; aisle edge lines on floor.
    for k in range(4):
        uu = U0 + L * (k + 0.5) / 4
        decal(DEC_WALL[k % 2], uu, V1 - 2, FZ + 150, unreal.Rotator(0, yaw_face("v1"), 0), unreal.Vector(0.3, 1.5, 2.0), "Decal_WallN_%d" % k)
    for side in (-1, 1):
        decal(DEC_LINE, (U0 + U1) / 2, VC + side * AISLE / 2, FZ + 2, unreal.Rotator(-90, 0 if LONG_X else 90, 0), unreal.Vector(0.3, 0.1, (L - 600) / 200.0), "Decal_AisleLine_%d" % (0 if side < 0 else 1))

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
unreal.log("Seph_Smart: removed %s; placed %s; total %d; missing %d" % (removed, cnt, sum(cnt.values()), len(set(missing))))
for p in sorted(set(missing)): unreal.log_warning("Seph_Smart missing: " + p)
