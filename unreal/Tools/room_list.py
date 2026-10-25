import unreal, os
ROOM = open(os.path.join(os.path.dirname(r"C:\Users\nickf\Documents\Unreal Projects\Chimera\Saved\SephScripts\tools\x"), "room.txt")).read().split()
x0, y0, x1, y1 = map(float, ROOM[:4])
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
rows = []
for a in EAS.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor) or not a.static_mesh_component.static_mesh: continue
    o, e = a.get_actor_bounds(False)
    if not (x0 <= o.x <= x1 and y0 <= o.y <= y1) or e.x > 2500 or e.y > 2500: continue
    r = a.get_actor_rotation(); s = a.get_actor_scale3d()
    rows.append((str(a.get_folder_path()), a.get_actor_label(), a.static_mesh_component.static_mesh.get_name(),
                 f"c({o.x:.0f},{o.y:.0f},{o.z:.0f}) e({e.x:.0f},{e.y:.0f},{e.z:.0f}) r({r.roll:.0f},{r.pitch:.0f},{r.yaw:.0f}) s({s.x:.2g},{s.y:.2g},{s.z:.2g})"))
for row in sorted(rows): print("|".join(row))
print("N", len(rows))
