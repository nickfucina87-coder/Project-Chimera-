# Seph blockout stubs: Zone D control room + Zone C corridor + vertical deck. Graybox only.
# Manages ONLY Seph_BO_* actors (Seph/Blockout). Does not touch lighting, PlayerStart or Astra layout.
import unreal
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
MAP = "/Game/ThirdPerson/L_CHM_ComponentTest"
W = unreal.EditorLevelLibrary.get_editor_world()
if W is None or W.get_path_name().split(".")[0] != MAP:
    LES.load_level(MAP)
CUBE = "/Game/LevelPrototyping/Meshes/SM_Cube"   # 100cm cube, pivot at bottom-center-ish; we place by center below
RAMP = "/Game/LevelPrototyping/Meshes/SM_Ramp"
WALL_T, H = 40.0, 500.0
cube = unreal.EditorAssetLibrary.load_asset(CUBE); ramp = unreal.EditorAssetLibrary.load_asset(RAMP)
if not cube: raise RuntimeError("Seph_BO: SM_Cube missing")
cb = cube.get_bounding_box(); CMINZ = cb.min.z  # handle pivot (0 or -50)

n_del = 0
for a in EAS.get_all_level_actors():
    if a.get_actor_label().startswith("Seph_BO_"): EAS.destroy_actor(a); n_del += 1

mn = [1e9]*3; mx = [-1e9]*3
for a in EAS.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor) or a.get_actor_label().startswith("Seph"): continue
    o, e = a.get_actor_bounds(False)
    if max(e.x, e.y) > 20000: continue
    for i, (c, x) in enumerate(((o.x, e.x), (o.y, e.y), (o.z, e.z))):
        mn[i] = min(mn[i], c - x); mx[i] = max(mx[i], c + x)
FZ = mn[2]
LONG_X = (mx[0] - mn[0]) >= (mx[1] - mn[1])
U0, U1, V0, V1 = (mn[0], mx[0], mn[1], mx[1]) if LONG_X else (mn[1], mx[1], mn[0], mx[0])
VC = (V0 + V1) / 2
def W2(u, v): return (u, v) if LONG_X else (v, u)
cnt = {}
def box(label, folder, u0, u1, v0, v1, z0, z1):
    """Axis-aligned graybox from local min/max (cm)."""
    uc, vc = (u0 + u1) / 2, (v0 + v1) / 2
    x, y = W2(uc, vc)
    sx, sy = ((u1 - u0), (v1 - v0)) if LONG_X else ((v1 - v0), (u1 - u0))
    sz = z1 - z0
    a = EAS.spawn_actor_from_object(cube, unreal.Vector(x, y, z0 - CMINZ * sz / 100.0), unreal.Rotator(0, 0, 0))
    a.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, sz / 100.0))
    a.set_actor_label("Seph_BO_" + label); a.set_folder_path("Seph/Blockout/" + folder)
    a.tags = [unreal.Name("Seph"), unreal.Name("SephBlockout")]
    cnt[folder] = cnt.get(folder, 0) + 1
    return a
def room(name, u0, u1, v0, v1, z0, h, doors):
    """Floor + 4 walls + ceiling; doors = list of (side, center, width, height) with side in u0/u1/v0/v1."""
    box(name + "_Floor", name, u0, u1, v0, v1, z0 - 20, z0)
    box(name + "_Ceil", name, u0, u1, v0, v1, z0 + h, z0 + h + 20)
    for side in ("u0", "u1", "v0", "v1"):
        ds = [d for d in doors if d[0] == side]
        along = (v0, v1) if side in ("u0", "u1") else (u0, u1)
        fixed = {"u0": (u0 - WALL_T, u0), "u1": (u1, u1 + WALL_T), "v0": (v0 - WALL_T, v0), "v1": (v1, v1 + WALL_T)}[side]
        segs = [along]; lintels = []
        for _, c, w, dh in ds:
            nxt = []
            for s0, s1 in segs:
                if s0 < c - w/2 and c + w/2 < s1: nxt += [(s0, c - w/2), (c + w/2, s1)]; lintels.append((c - w/2, c + w/2, dh))
                else: nxt.append((s0, s1))
            segs = nxt
        def wb(lbl, a0, a1, zz0, zz1):
            if side in ("u0", "u1"): box(lbl, name, fixed[0], fixed[1], a0, a1, zz0, zz1)
            else: box(lbl, name, a0, a1, fixed[0], fixed[1], zz0, zz1)
        for k, (s0, s1) in enumerate(segs): wb("%s_Wall_%s_%d" % (name, side, k), s0, s1, z0, z0 + h)
        for k, (s0, s1, dh) in enumerate(lintels): wb("%s_Lintel_%s_%d" % (name, side, k), s0, s1, z0 + dh, z0 + h)

with unreal.ScopedEditorTransaction("Seph Blockout Facility Stub") as tx:
    # Zone D control room off the +U (lab) end: 1200 x 1000, raised +150 (stair up = sightline down into lab).
    DZ = FZ + 150
    d0, d1 = U1 + 600, U1 + 1800
    # connector corridor lab -> D (300 wide), with ramp rising 150
    box("D_Conn_Floor", "D_Connector", U1, d0, VC - 150, VC + 150, FZ - 20, FZ)
    for s, (a, b) in (("S", (VC - 150 - WALL_T, VC - 150)), ("N", (VC + 150, VC + 150 + WALL_T))):
        box("D_Conn_Wall" + s, "D_Connector", U1, d0, a, b, FZ, DZ + 400)
    box("D_Conn_Ceil", "D_Connector", U1, d0, VC - 150, VC + 150, DZ + 400, DZ + 420)
    if ramp:
        x, y = W2(U1 + 300, VC)
        r = EAS.spawn_actor_from_object(ramp, unreal.Vector(x, y, FZ), unreal.Rotator(0, 0 if LONG_X else 90, 0))
        rb = ramp.get_bounding_box()
        r.set_actor_scale3d(unreal.Vector(400.0 / max(1, rb.max.x - rb.min.x), 280.0 / max(1, rb.max.y - rb.min.y), 150.0 / max(1, rb.max.z - rb.min.z)))
        r.set_actor_label("Seph_BO_D_Conn_Ramp"); r.set_folder_path("Seph/Blockout/D_Connector"); cnt["D_Connector"] = cnt.get("D_Connector", 0) + 1
        unreal.log("Seph_BO: check ramp direction (should rise toward Zone D); rotate yaw 180 if reversed.")
    box("D_Conn_Landing", "D_Connector", U1 + 500, d0, VC - 150, VC + 150, FZ, DZ)
    room("ZoneD_Control", d0, d1, VC - 500, VC + 500, DZ, 450,
         doors=[("u0", VC, 280, 300), ("v1", d0 + 900, 240, 280)])
    # D: console bank facing a viewing window slot back toward lab, plus central monitor island
    box("D_ConsoleBank", "ZoneD_Control", d0 + 150, d0 + 250, VC - 350, VC + 350, DZ, DZ + 110)
    box("D_MonitorIsland", "ZoneD_Control", d0 + 550, d0 + 750, VC - 150, VC + 150, DZ, DZ + 90)
    # Vertical: partial mezzanine deck in D (south half) + stair block
    box("D_Mezz_Deck", "ZoneD_Vertical", d0 + 700, d1, VC - 500, VC - 150, DZ + 280, DZ + 300)
    for k in range(7):  # 7 steps x 40cm = 280
        box("D_Stair_%d" % k, "ZoneD_Vertical", d0 + 700 - (7 - k) * 50, d0 + 700 - (6 - k) * 50, VC - 500, VC - 380, DZ, DZ + (k + 1) * 40)
    box("D_Mezz_Rail", "ZoneD_Vertical", d0 + 700, d1, VC - 160, VC - 150, DZ + 300, DZ + 400)
    # Zone C corridor from D's north door toward surface exit: long L-shaped hall rising via ramp-blocks.
    cu0, cu1 = d0 + 780, d0 + 1020
    c_v0 = VC + 500 + WALL_T
    box("C_Corr1_Floor", "ZoneC_Corridor", cu0, cu1, c_v0, c_v0 + 1600, DZ - 20, DZ)
    box("C_Corr1_WallW", "ZoneC_Corridor", cu0 - WALL_T, cu0, c_v0, c_v0 + 1600, DZ, DZ + 400)
    box("C_Corr1_WallE", "ZoneC_Corridor", cu1, cu1 + WALL_T, c_v0, c_v0 + 1600, DZ, DZ + 400)
    box("C_Corr1_Ceil", "ZoneC_Corridor", cu0, cu1, c_v0, c_v0 + 1600, DZ + 400, DZ + 420)
    # cover blocks staggered (combat sightline breaks)
    for k, (ou, ov) in enumerate(((40, 400), (150, 900), (40, 1300))):
        box("C_Cover_%d" % k, "ZoneC_Corridor", cu0 + ou, cu0 + ou + 60, c_v0 + ov, c_v0 + ov + 150, DZ, DZ + 110)
    # stair up 300 to surface lobby at corridor end
    SZ = DZ + 300
    for k in range(10):
        box("C_Stair_%d" % k, "ZoneC_Vertical", cu0, cu1, c_v0 + 1600 + k * 40, c_v0 + 1640 + k * 40, DZ, DZ + (k + 1) * 30)
    room("ZoneC_Lobby", cu0 - 600, cu1 + 600, c_v0 + 2000, c_v0 + 3000, SZ, 600,
         doors=[("v0", (cu0 + cu1) / 2, 240, 300), ("v1", (cu0 + cu1) / 2, 500, 450)])
    # Outdoor A/B: marker only (no art). Flat pad + tall marker pole where the bridge/forest begins.
    box("AB_OUTDOOR_STUB_Pad", "ZoneAB_Marker", cu0 - 1000, cu1 + 1000, c_v0 + 3000, c_v0 + 5000, SZ - 20, SZ)
    box("AB_OUTDOOR_STUB_Marker", "ZoneAB_Marker", (cu0 + cu1) / 2 - 25, (cu0 + cu1) / 2 + 25, c_v0 + 4000, c_v0 + 4050, SZ, SZ + 1200)
    unreal.log("Seph_BO: Zone A/B outdoor is a MARKER ONLY (pad + pole). Needs bridge pack / Megascans foliage added to project before real blockout.")

for a in EAS.get_all_level_actors():
    if isinstance(a, unreal.PlayerStart): unreal.log("Seph_BO: PlayerStart left at %s" % a.get_actor_location())
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
unreal.log("Seph_BO: removed %d old, placed %s total %d" % (n_del, cnt, sum(cnt.values())))
