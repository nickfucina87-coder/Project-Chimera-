# Builds BP_Hexapod_Test (copy of the ThirdPerson character with the hexapod mesh) and BP_Hexapod_GameMode,
# then sets that GameMode as the World Settings override on L_CHM_ComponentTest ONLY. Re-runnable.
import unreal
EAL = unreal.EditorAssetLibrary
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
DIR = "/Game/Characters/Hexapod"
BP_PATH = DIR + "/BP_Hexapod_Test"
GM_PATH = DIR + "/BP_Hexapod_GameMode"
SRC_CHAR = "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"
SRC_GM = "/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode"
MAP = "/Game/ThirdPerson/L_CHM_ComponentTest"

# find the imported skeletal mesh + an anim
mesh = anim = None
for p in EAL.list_assets(DIR, recursive=True):
    o = EAL.load_asset(p)
    if isinstance(o, unreal.SkeletalMesh) and mesh is None: mesh = o
    if isinstance(o, unreal.AnimSequence) and anim is None: anim = o
if mesh is None: raise RuntimeError("Seph: no SkeletalMesh under " + DIR)
unreal.log("Seph: mesh %s anim %s" % (mesh.get_path_name(), anim.get_path_name() if anim else None))

def dup(src, dst):
    if EAL.does_asset_exist(dst): return EAL.load_asset(dst)
    return EAL.duplicate_asset(src, dst)

bp = dup(SRC_CHAR, BP_PATH)
gm = dup(SRC_GM, GM_PATH)

# fit mesh to capsule
b = mesh.get_bounds(); h = b.box_extent.z * 2.0
def setup_skel(c):
    c.set_editor_property("skeletal_mesh_asset" if hasattr(c, "skeletal_mesh_asset") else "skeletal_mesh", mesh)
    c.set_editor_property("anim_class", None)   # mannequin ABP won't fit hexapod skeleton
    if anim:
        c.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        d = c.get_editor_property("animation_data"); d.set_editor_property("anim_to_play", anim)
        d.set_editor_property("looping", True); c.set_editor_property("animation_data", d)
    else:
        c.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_CUSTOM_MODE)
    c.set_editor_property("relative_location", unreal.Vector(0, 0, -90.0))

ok = False
try:
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    for hnd in sds.k2_gather_subobject_data_for_blueprint(bp):
        o = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(hnd))
        if isinstance(o, unreal.SkeletalMeshComponent): setup_skel(o); ok = True
except Exception as e:
    unreal.log_warning("Seph: subobject path failed (%s), trying CDO" % e)
cdo = unreal.get_default_object(bp.generated_class())
try:
    setup_skel(cdo.get_editor_property("mesh")); ok = True
except Exception as e:
    unreal.log_warning("Seph: CDO mesh set failed (%s)" % e)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
EAL.save_loaded_asset(bp)

gcdo = unreal.get_default_object(gm.generated_class())
gcdo.set_editor_property("default_pawn_class", bp.generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(gm)
EAL.save_loaded_asset(gm)

W = unreal.EditorLevelLibrary.get_editor_world()
if W is None or W.get_path_name().split(".")[0] != MAP:
    LES.load_level(MAP); W = unreal.EditorLevelLibrary.get_editor_world()
ws = W.get_world_settings()
ws.set_editor_property("default_game_mode", gm.generated_class())
LES.save_current_level()
unreal.log("Seph: hexapod pawn ready (mesh set=%s, mesh height %.0f cm). Press Play (Alt+P) in L_CHM_ComponentTest." % (ok, h))
unreal.log("Seph: if he is invisible/tiny/huge, open BP_Hexapod_Test > Mesh and set Skeletal Mesh / scale by hand.")
