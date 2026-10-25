import unreal, collections, math
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = EAS.get_all_level_actors()
def bb(a):
    o, e = a.get_actor_bounds(False); return (o.x - e.x, o.y - e.y, o.z - e.z, o.x + e.x, o.y + e.y, o.z + e.z)
groups = collections.defaultdict(list)
for a in acts:
    top = "/".join(str(a.get_folder_path()).split("/")[:2])
    groups[top].append(a)
print("== folder bounds (cm): count  min  max")
for k, v in sorted(groups.items()):
    bs = [bb(a) for a in v if not isinstance(a, (unreal.SkyLight, unreal.DirectionalLight, unreal.SkyAtmosphere, unreal.VolumetricCloud, unreal.ExponentialHeightFog, unreal.PostProcessVolume, unreal.WorldDataLayers, unreal.WorldPartitionMiniMap))]
    if not bs: continue
    lo = [round(min(b[i] for b in bs)) for i in range(3)]; hi = [round(max(b[i + 3] for b in bs)) for i in range(3)]
    print(f"{k:40s} {len(v):4d} {lo} {hi}")
# exact / near duplicates (same mesh, same place)
seen = collections.defaultdict(list)
for a in acts:
    if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh:
        l = a.get_actor_location(); r = a.get_actor_rotation(); s = a.get_actor_scale3d()
        key = (a.static_mesh_component.static_mesh.get_name(), round(l.x / 5), round(l.y / 5), round(l.z / 5), round(r.yaw / 5) % 72, round(s.x, 1), round(s.y, 1), round(s.z, 1))
        seen[key].append(a.get_actor_label())
d = [v for v in seen.values() if len(v) > 1]
print("== duplicate groups", len(d), "extra copies", sum(len(v) - 1 for v in d))
for v in d[:25]: print("  ", v)
# shell cubes: list the big blockout pieces (walls/floors)
cubes = [a for a in acts if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh and a.static_mesh_component.static_mesh.get_name() == "Cube"]
print("== cubes by folder", collections.Counter(str(a.get_folder_path()) for a in cubes).most_common(30))
