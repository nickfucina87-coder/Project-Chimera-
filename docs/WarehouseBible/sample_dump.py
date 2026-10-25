"""Open the factory sample map and dump every placed object (mesh, transform, bounds, attach parent, folder) to JSON."""
import unreal, json, collections
OUT = r"C:\Users\nickf\AppData\Local\Temp\claude\C--Users-nickf\0df46d9a-2366-4026-9062-dc693361cd0b\scratchpad\sample_dump.json"
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
LES.load_level("/Game/Maps/Demonstration")
rows = []
for a in EAS.get_all_level_actors():
    o, e = a.get_actor_bounds(False)
    t = a.get_actor_transform()
    comps = []
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        if isinstance(c, unreal.InstancedStaticMeshComponent):
            m = c.static_mesh
            comps.append({"ism": m.get_path_name().split(".")[0] if m else None, "count": c.get_instance_count()})
        elif c.static_mesh:
            ct = c.get_world_transform()
            comps.append({"mesh": c.static_mesh.get_path_name().split(".")[0], "loc": [ct.translation.x, ct.translation.y, ct.translation.z],
                          "rot": [ct.rotation.rotator().roll, ct.rotation.rotator().pitch, ct.rotation.rotator().yaw],
                          "scale": [ct.scale3d.x, ct.scale3d.y, ct.scale3d.z]})
    for c in a.get_components_by_class(unreal.SplineMeshComponent):
        comps.append({"splinemesh": c.static_mesh.get_path_name().split(".")[0] if c.static_mesh else None})
    par = a.get_attach_parent_actor()
    rows.append({"label": a.get_actor_label(), "class": a.get_class().get_name(), "folder": str(a.get_folder_path()),
                 "loc": [t.translation.x, t.translation.y, t.translation.z], "rot": [t.rotation.rotator().roll, t.rotation.rotator().pitch, t.rotation.rotator().yaw],
                 "bmin": [o.x - e.x, o.y - e.y, o.z - e.z], "bmax": [o.x + e.x, o.y + e.y, o.z + e.z],
                 "parent": par.get_actor_label() if par else None, "comps": comps})
json.dump(rows, open(OUT, "w"))
cls = collections.Counter(r["class"] for r in rows)
print("actors", len(rows), cls.most_common(20))
print("folders", collections.Counter(r["folder"].split("/")[0] for r in rows).most_common(30))
