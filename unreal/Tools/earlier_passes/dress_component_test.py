import unreal, math
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
MAP = "/Game/ThirdPerson/L_CHM_ComponentTest"
W = unreal.EditorLevelLibrary.get_editor_world()
if W is None or W.get_path_name().split(".")[0] != MAP:
    LES.load_level(MAP)

A = "/Game/Scene_Warehouse/Assets/"
BEAM  = A + "Custom/Ind_War_Beam/Ind_War_L-Shape_Beam"
RUSTY = A + "MS/3D/Ind_Con_Supplies_Beam_Metal_Rusty_01/SM_Ind_Con_Supplies_Beam_Metal_Rusty_01"
LIGHTS = [A + "MS/3D/Ind_War_Light_Ceiling_Metal_Hanging_0%d/SM_Ind_War_Light_Ceiling_Metal_Hanging_0%d" % (i, i) for i in (1, 2)]
BULK  = A + "MS/3D/Ind_War_Bulkhead_Metal_Caged_01/SM_Ind_War_Bulkhead_Metal_Caged_01"
WIRE  = A + "Custom/Ind_War_Cable/SM_Ind_War_Wire"
def ld(p):
    o = unreal.EditorAssetLibrary.load_asset(p)
    if not o: unreal.log_warning("Seph: missing " + p)
    return o

# remove previous Seph pass so re-running is clean
for a in EAS.get_all_level_actors():
    if a.get_actor_label().startswith("Seph_"): EAS.destroy_actor(a)

# measure existing layout (static meshes only, skip huge floors/sky)
mn = [1e9]*3; mx = [-1e9]*3; floor_z = 1e9
for a in EAS.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor): continue
    o, e = a.get_actor_bounds(False)
    if max(e.x, e.y) > 20000: continue
    for i, (c, x) in enumerate(((o.x, e.x), (o.y, e.y), (o.z, e.z))):
        mn[i] = min(mn[i], c - x); mx[i] = max(mx[i], c + x)
    floor_z = min(floor_z, o.z - e.z)
if mn[0] > mx[0]: raise RuntimeError("Seph: no static meshes found to measure")
wall_top = mx[2]
CZ = max(floor_z + 450.0, min(wall_top, floor_z + 600.0))  # ceiling height
long_x = (mx[0]-mn[0]) >= (mx[1]-mn[1])
L0, L1 = (mn[0], mx[0]) if long_x else (mn[1], mx[1])
S0, S1 = (mn[1], mx[1]) if long_x else (mn[0], mx[0])
def P(l, s, z): return unreal.Vector(l, s, z) if long_x else unreal.Vector(s, l, z)
unreal.log("Seph: bounds %s %s floor %.0f ceiling %.0f" % (mn, mx, floor_z, CZ))

count = {}
def spawn(path, loc, rot, label, folder, span=None, axis_long=True):
    m = ld(path)
    if not m: return None
    a = EAS.spawn_actor_from_object(m, loc, rot)
    if not a: return None
    if span:  # stretch mesh along its longest local axis to reach span
        b = m.get_bounding_box(); ext = b.max - b.min
        ln = max(ext.x, ext.y, ext.z, 1.0)
        k = span / ln
        sc = unreal.Vector(k if ext.x == ln else 1, k if ext.y == ln and ext.x != ln else 1, k if ext.z == ln and ext.x != ln and ext.y != ln else 1)
        a.set_actor_scale3d(sc)
    a.set_actor_label("Seph_" + label)
    a.set_folder_path("Seph/" + folder)
    a.tags = [unreal.Name("Seph")]
    count[folder] = count.get(folder, 0) + 1
    return a

with unreal.ScopedEditorTransaction("Seph dress ComponentTest") as tx:
    yaw_cross = 90.0 if long_x else 0.0   # rafters span the short axis
    yaw_long = 0.0 if long_x else 90.0
    # rafters every ~350 cm along the long axis
    n = max(2, int((L1 - L0) // 350) + 1); step = (L1 - L0) / (n - 1)
    for i in range(n):
        l = L0 + i * step
        spawn(BEAM, P(l, (S0+S1)/2, CZ), unreal.Rotator(0, 0, yaw_cross), "Rafter_%02d" % i, "Rafters", span=S1-S0)
    # two long rusty girders (purlins) carrying the rafters
    for j, s in enumerate((S0 + (S1-S0)*0.25, S0 + (S1-S0)*0.75)):
        spawn(RUSTY, P((L0+L1)/2, s, CZ + 25), unreal.Rotator(0, 0, yaw_long), "Girder_%d" % j, "Rafters", span=L1-L0)
    # columns / bulkheads at perimeter every other rafter -> vertical structure
    for i in range(0, n, 2):
        l = L0 + i * step
        for side, s in (("A", S0), ("B", S1)):
            spawn(BULK, P(l, s, floor_z), unreal.Rotator(0, 0, yaw_long), "Bulkhead_%s%02d" % (side, i), "Bulkheads")
    # hanging lights over aisle centre, between rafters, plus a point light each
    for i in range(n - 1):
        l = L0 + (i + 0.5) * step
        s = (S0 + S1) / 2
        spawn(LIGHTS[i % 2], P(l, s, CZ - 10), unreal.Rotator(0, 0, 0), "CeilLight_%02d" % i, "Lights")
        pl = EAS.spawn_actor_from_class(unreal.PointLight, P(l, s, CZ - 120), unreal.Rotator(0, 0, 0))
        pl.point_light_component.set_intensity(3000.0)
        pl.point_light_component.set_attenuation_radius(700.0)
        pl.point_light_component.set_light_color(unreal.LinearColor(1.0, 0.85, 0.7, 1))
        pl.set_actor_label("Seph_CeilLightPL_%02d" % i); pl.set_folder_path("Seph/Lights")
        count["Lights"] = count.get("Lights", 0) + 1
    # 3 cable runs along the girders
    for k, f in enumerate((0.22, 0.5, 0.78)):
        spawn(WIRE, P((L0+L1)/2, S0 + (S1-S0)*f, CZ - 15), unreal.Rotator(0, 0, yaw_long), "CableRun_%d" % k, "Cables", span=L1-L0)

unreal.log("Seph: spawned %s" % count)
LES.save_current_level()
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
unreal.log("Seph: level saved")
