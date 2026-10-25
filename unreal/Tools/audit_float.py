"""Level-wide sanity audit: props sunk into the floor, props floating with no support / wall / ceiling contact."""
import unreal, collections
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
FLOOR = -76.0
items = []
for a in EAS.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor) or not a.static_mesh_component.static_mesh: continue
    f = str(a.get_folder_path())
    if f.startswith(("Warehouse", "Facility/Roof", "Facility/Shell", "Facility/B4")): continue
    o, e = a.get_actor_bounds(False)
    items.append((a, f, o.x - e.x, o.y - e.y, o.z - e.z, o.x + e.x, o.y + e.y, o.z + e.z))
solids = items + [(a, str(a.get_folder_path()), o.x - e.x, o.y - e.y, o.z - e.z, o.x + e.x, o.y + e.y, o.z + e.z)
                  for a in EAS.get_all_level_actors() if isinstance(a, unreal.StaticMeshActor) and str(a.get_folder_path()).startswith(("Facility/Shell", "Facility/Roof"))
                  for o, e in [a.get_actor_bounds(False)]]
def touching(b, tol=12):
    for s in solids:
        if s[0] == b[0]: continue
        if s[2] - tol <= b[5] and b[2] - tol <= s[5] and s[3] - tol <= b[6] and b[3] - tol <= s[6] and s[4] - tol <= b[7] and b[4] - tol <= s[7]:
            return True
    return False
sunk, floating = [], []
for b in items:
    a, f, x0, y0, z0, x1, y1, z1 = b
    flat = (z1 - z0) < 3                      # decals / tape
    ground_level = z0 < 600                   # ground floor zone (2nd floor ~ 450+ handled by touching())
    if not flat and z0 < FLOOR - 15 and z1 > FLOOR and (z1 - z0) < 1000 and -1000 < z0:
        sunk.append(b)
    if not flat and z0 > FLOOR + 15 and not touching(b):
        floating.append(b)
def show(title, rows):
    print(f"== {title}: {len(rows)}")
    by = collections.Counter(r[1] for r in rows); print("   by folder:", by.most_common(15))
    for r in rows: print(f"   {r[0].get_actor_label():30s} {r[1]:34s} {r[0].static_mesh_component.static_mesh.get_name():34s} z[{r[4]:.0f},{r[7]:.0f}] xy({(r[2]+r[5])/2:.0f},{(r[3]+r[6])/2:.0f})")
show("SUNK below floor", sunk)
show("FLOATING (touches nothing)", floating)
