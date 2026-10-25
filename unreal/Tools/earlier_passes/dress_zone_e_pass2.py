# Zone E pass 2 -- lab clutter, electrical boxes, catwalk + ladders. Only touches Seph_E2_* actors.
# NOTE: re-running dress_component_test.py deletes ALL Seph_* actors (including these + lighting PPV). Re-run this + lighting after.
import unreal, random
random.seed(7)
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
MAP = "/Game/ThirdPerson/L_CHM_ComponentTest"
W = unreal.EditorLevelLibrary.get_editor_world()
if W is None or W.get_path_name().split(".")[0] != MAP:
    LES.load_level(MAP)
A = "/Game/Scene_Warehouse/Assets/MS/3D/"
def ms(n, suf=""): return A + n + "/SM_" + n + suf
EBOX  = [ms("Ind_Fac_Panel_ElectricalBox_Metal_Worn_03"), ms("Urb_Str_ElectricalBox_Metal_Worn_01"), "/Game/Sparks_Embers/Meshes/SM_Electric_Box_"]
CAB   = ms("Ind_War_Cabinet_Electric_Metal_Dirty_01")
CABLE = [ms("Ind_War_Cable_Power_Rubber_Weathered_03"), ms("Ind_War_Equipment_RadioCable_01")]
LADDER= [ms("Ind_War_Equipment_Ladder_Metal_03"), ms("Ind_War_Equipment_Ladder_Metal_Worn_01")]
BEAM  = ms("Ind_Con_Supplies_Beam_Metal_Rusty_01")
CLUT  = [ms("Ind_Fac_Container_Drum_Metal_Worn_01"), ms("Ind_Aba_Tank_Gas_Metal_Worn_02"), ms("Ind_War_Storage_Crate_Plastic_Teal_01"),
         ms("Res_Gar_Storage_Toolbox_Wood_Worn_01"), ms("Ind_War_Storage_Cabinet_Metal_Rusty_01"), ms("Ind_Aba_Chain_Metal_Hanging_01")]
DECAL = ["/Game/Scene_Warehouse/Assets/MS/Decals/Decal_Leak_Grime_01/MI_Decal_Leak_Grime_01_A",
         "/Game/Scene_Warehouse/Assets/MS/Decals/Decal_Stain_Mud_01/MI_Decal_Stain_Mud_01_A",
         "/Game/Scene_Warehouse/Assets/MS/Decals/Decal_Papers_Dirty_01/MI_Decal_Papers_Dirty_01"]
CUBE  = "/Game/LevelPrototyping/Meshes/SM_Cube"
SPARK = "/Game/Sparks_Embers/NS_Sparks01"

def ld(p):
    o = unreal.EditorAssetLibrary.load_asset(p)
    if not o: unreal.log_warning("Seph: missing " + p)
    return o
for a in EAS.get_all_level_actors():
    if a.get_actor_label().startswith("Seph_E2_"): EAS.destroy_actor(a)

mn = [1e9]*3; mx = [-1e9]*3
for a in EAS.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor) or a.get_actor_label().startswith("Seph"): continue
    o, e = a.get_actor_bounds(False)
    if max(e.x, e.y) > 20000: continue
    for i, (c, x) in enumerate(((o.x, e.x), (o.y, e.y), (o.z, e.z))):
        mn[i] = min(mn[i], c - x); mx[i] = max(mx[i], c + x)
FZ = mn[2]
cnt = {}
def put(path, x, y, z, yaw=0, label="x", folder="Clutter", scale=None):
    m = ld(path)
    if not m: return None
    a = EAS.spawn_actor_from_object(m, unreal.Vector(x, y, z), unreal.Rotator(0, 0, yaw))
    if not a: return None
    if scale: a.set_actor_scale3d(scale)
    a.set_actor_label("Seph_E2_" + label); a.set_folder_path("Seph/E2_" + folder)
    a.tags = [unreal.Name("Seph")]; cnt[folder] = cnt.get(folder, 0) + 1
    return a

with unreal.ScopedEditorTransaction("Seph Zone E pass 2") as tx:
    X0, X1, Y0, Y1 = mn[0] + 60, mx[0] - 60, mn[1] + 60, mx[1] - 60
    # electrical boxes on the two long walls + a breaker cabinet with sparks
    put(EBOX[0], X0 + (X1-X0)*0.3, Y0, FZ + 140, 90, "EBox_A", "Electrical")
    put(EBOX[1], X0 + (X1-X0)*0.7, Y1, FZ + 120, -90, "EBox_B", "Electrical")
    cab = put(CAB, X1, Y0 + (Y1-Y0)*0.5, FZ, 180, "BreakerCab", "Electrical")
    ns = ld(SPARK)
    if ns and cab:
        s = EAS.spawn_actor_from_object(ns, unreal.Vector(X1 - 30, Y0 + (Y1-Y0)*0.5, FZ + 160), unreal.Rotator(0, 0, 0))
        if s: s.set_actor_label("Seph_E2_Sparks"); s.set_folder_path("Seph/E2_Electrical")
    # floor cable snakes along the walls
    for i in range(6):
        y = Y0 + 30 if i % 2 == 0 else Y1 - 30
        put(CABLE[i % 2], X0 + (X1-X0)*(i+0.5)/6, y, FZ + 2, random.uniform(-20, 20), "FloorCable_%d" % i, "Cables")
    # perimeter clutter, kept 80cm off walls only (aisles untouched)
    for i in range(10):
        side = i % 4
        t = random.uniform(0.08, 0.92)
        x, y = {0: (X0 + 40, Y0 + (Y1-Y0)*t), 1: (X1 - 40, Y0 + (Y1-Y0)*t), 2: (X0 + (X1-X0)*t, Y0 + 40), 3: (X0 + (X1-X0)*t, Y1 - 40)}[side]
        put(CLUT[i % len(CLUT)], x, y, FZ, random.uniform(0, 360), "Clutter_%02d" % i)
    # grime decals on the floor
    for i in range(6):
        mi = ld(DECAL[i % 3])
        if not mi: continue
        d = EAS.spawn_actor_from_class(unreal.DecalActor, unreal.Vector(random.uniform(X0, X1), random.uniform(Y0, Y1), FZ + 5), unreal.Rotator(-90, 0, random.uniform(0, 360)))
        d.set_decal_material(mi); d.set_actor_scale3d(unreal.Vector(0.5, 1.5, 1.5))
        d.set_actor_label("Seph_E2_Decal_%d" % i); d.set_folder_path("Seph/E2_Decals")
    # second elevation: catwalk along wall Y0 at +320cm, beam rails, two ladders
    CWZ = FZ + 320.0; L = X1 - X0
    deck = put(CUBE, (X0+X1)/2, Y0 + 90, CWZ, 0, "Catwalk_Deck", "Catwalk", unreal.Vector(L/100.0, 1.5, 0.1))
    for j, yy in enumerate((Y0 + 20, Y0 + 160)):
        put(BEAM, (X0+X1)/2, yy, CWZ + 100, 0, "Catwalk_Rail_%d" % j, "Catwalk")
    for k in range(int(L // 400) + 1):
        put(CUBE, X0 + k*400, Y0 + 160, FZ, 0, "Catwalk_Post_%d" % k, "Catwalk", unreal.Vector(0.15, 0.15, CWZ/100.0 - FZ/100.0))
    put(LADDER[0], X0 + 120, Y0 + 180, FZ, 0, "Ladder_A", "Catwalk")
    put(LADDER[1], X1 - 120, Y0 + 180, FZ, 0, "Ladder_B", "Catwalk")
    # one cool spot under the catwalk, nothing else (fewer lights = mood)
    sp = EAS.spawn_actor_from_class(unreal.SpotLight, unreal.Vector((X0+X1)/2, Y0 + 90, CWZ - 20), unreal.Rotator(-90, 0, 0))
    c = sp.spot_light_component; c.set_intensity(800.0); c.set_attenuation_radius(500.0)
    c.set_light_color(unreal.LinearColor(0.6, 0.8, 1.0, 1)); c.set_editor_property("outer_cone_angle", 60.0)
    sp.set_actor_label("Seph_E2_UnderCatwalkSpot"); sp.set_folder_path("Seph/E2_Catwalk")

unreal.log("Seph E2: %s" % cnt)
LES.save_current_level()
