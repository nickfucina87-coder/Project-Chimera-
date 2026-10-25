"""Strip props: keep structure everywhere, keep the server hall minus its wires and floor overlays.
MODE = 'dry' prints the plan; 'apply' deletes and saves."""
import unreal, re, collections, os
MODE = open(os.path.join(r"C:\Users\nickf\AppData\Local\Temp\claude\C--Users-nickf\0df46d9a-2366-4026-9062-dc693361cd0b\scratchpad", "strip_mode.txt")).read().strip()
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
FLOOR = -76.0
STRUCT_FOLDERS = ("Facility/Shell", "Facility/Roof", "Facility/Overhead/Catwalks", "Facility/Power/Walkway", "Warehouse")
STRUCT_MESH = re.compile(r"Platform|Stair|Railing|Pillar|Beam|Wall|Floor_|RoofFrame|HeavyPlatform|Lift|Shaft", re.I)
WIRE = re.compile(r"Cable|Wire|CableSet|Conduit", re.I)
SERVER = "Facility/ServerHall"
dele, keep = collections.Counter(), collections.Counter()
kill = []; dbg = collections.Counter()
for a in EAS.get_all_level_actors():
    f = str(a.get_folder_path()); lbl = a.get_actor_label()
    if isinstance(a, (unreal.Light, unreal.SkyLight, unreal.PostProcessVolume, unreal.ExponentialHeightFog, unreal.SkyAtmosphere,
                      unreal.VolumetricCloud, unreal.PlayerStart, unreal.SceneCapture2D, unreal.WorldDataLayers, unreal.WorldPartitionMiniMap)):
        continue
    dbg["a"] += 1
    if lbl.startswith("SEPH_Light_"): continue                      # high-bay fixtures hold the daylight spots
    if f.startswith(STRUCT_FOLDERS): continue
    if not isinstance(a, (unreal.StaticMeshActor, unreal.TextRenderActor, unreal.SkeletalMeshActor, unreal.Emitter, unreal.NiagaraActor)):
        continue
    mesh = a.static_mesh_component.static_mesh.get_name() if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh else ""
    o, e = a.get_actor_bounds(False)
    if f.startswith(SERVER):
        flat_floor = e.z < 3 and o.z < FLOOR + 10
        is_wire = bool(WIRE.search(mesh) or WIRE.search(lbl) or lbl.startswith(("FAC_Tray_", "FAC_FloorBundle", "FAC_CableDrop")))
        if is_wire or flat_floor:
            kill.append(a); dele[f + (" [wire]" if is_wire else " [floor overlay]")] += 1
        else: keep[f] += 1
        continue
    ext = sorted((e.x, e.y, e.z))
    if f.startswith("Facility/B4") and mesh == "Cube" and ext[1] > 100:
        keep[f + " [B4 room structure]"] += 1; continue
    if isinstance(a, unreal.StaticMeshActor) and STRUCT_MESH.search(mesh) and not f.startswith(("Facility/Plants", "Facility/Clutter", "Facility/Front/Entrance_SEPH", "Facility/Workshop/PowerFeed")):
        keep[f + " [structural mesh]"] += 1; continue
    kill.append(a); dele[f] += 1
print("DBG", dbg, len(EAS.get_all_level_actors())); print("== DELETE", sum(dele.values())); [print(f"   {v:4d} {k}") for k, v in sorted(dele.items())]
print("== KEEP (non-structure folders)", sum(keep.values())); [print(f"   {v:4d} {k}") for k, v in sorted(keep.items())]
if MODE == "apply":
    kill += [a for a in EAS.get_all_level_actors() if a.get_actor_label().startswith("SEPH_Ent_") and a not in kill]
    for a in kill: EAS.destroy_actor(a)
    print("deleted", len(kill), "saved", unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True))
