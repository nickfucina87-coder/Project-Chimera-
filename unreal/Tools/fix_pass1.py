"""Cleanup pass 1: nonsense props, sunk / floating props, power-room exterior hole."""
import unreal, re
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
FLOOR = -76.0
by = {a.get_actor_label(): a for a in EAS.get_all_level_actors()}
def num(lbl): return int(re.findall(r"\d+$", lbl)[0])

# 1) junk: cables hanging in mid-air, motor row, stray fans, floor-cable grid in the power room
junk = [l for l in by if (l.startswith(("FAC_CableDrop_", "FAC_GenDrop_")) and by[l].get_folder_path() and "SM_ElectricalCable_01" in by[l].static_mesh_component.static_mesh.get_name() if isinstance(by[l], unreal.StaticMeshActor) else False)]
junk += [l for l in by if l.startswith(("FAC_Motor_143", "FAC_PortableFan_", "FAC_PowerCable_14"))]
for l in sorted(set(junk)): EAS.destroy_actor(by.pop(l))
print("deleted", len(set(junk)), sorted(set(junk)))

# 2) raise sunk props so they sit on the floor (or at mounting height for wall boards)
def bottom_to(lbl, z):
    a = by.get(lbl)
    if not a: return
    o, e = a.get_actor_bounds(False); d = z - (o.z - e.z)
    a.set_actor_location(a.get_actor_location() + unreal.Vector(0, 0, d), False, False); print("raised", lbl, round(d))
for l in ("FAC_WallWiring_1423", "FAC_WallWiring_1424", "FAC_WallWiring_1425", "FAC_PipeRack_640", "FAC_EngCables_1198", "FAC_OfficeStuff_941"):
    bottom_to(l, FLOOR)
bottom_to("FAC_Whiteboard_1197", FLOOR + 80)

# 3) drop small floating props onto whatever is underneath
def drop(lbl):
    a = by.get(lbl)
    if not a: return
    o, e = a.get_actor_bounds(False)
    start = unreal.Vector(o.x, o.y, o.z - e.z - 1); end = unreal.Vector(o.x, o.y, FLOOR - 50)
    hit = unreal.SystemLibrary.line_trace_single(W, start, end, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [a],
                                                 unreal.DrawDebugTrace.NONE, True)
    z = hit.to_tuple()[4].z if hit and hit.to_tuple()[0] else FLOOR
    d = z - (o.z - e.z)
    a.set_actor_location(a.get_actor_location() + unreal.Vector(0, 0, d), False, False); print("dropped", lbl, round(d))
for l in ("FAC_Monitor_935", "FAC_Monitor_938", "FAC_EngMonitor_1199", "FAC_PartsBin_1207"):
    drop(l)

# 4) close the 8 m opening in the power room's back exterior wall, and pull the cooling tower inside
src = by["FAC_ExtBack_7"]
patch = EAS.spawn_actor_from_object(src.static_mesh_component.static_mesh, unreal.Vector(2200, -2980, (FLOOR - 30 + 424) / 2))
patch.set_actor_scale3d(unreal.Vector(8.0, 0.4, (424 - (FLOOR - 30)) / 100.0))
for i in range(src.static_mesh_component.get_num_materials()):
    patch.static_mesh_component.set_material(i, src.static_mesh_component.get_material(i))
patch.set_actor_label("FAC_ExtBack_Patch"); patch.set_folder_path("Facility/Shell")
t = by["FAC_CoolTower_1404"]; o, e = t.get_actor_bounds(False)
t.set_actor_location(t.get_actor_location() + unreal.Vector(0, (-2960 + 5) - (o.y - e.y), 0), False, False)
print("patched back wall; cooling tower moved", round((-2960 + 5) - (o.y - e.y)))
print("saved", unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True))
