"""Entrance + front corridor dressing, built to WAREHOUSE_BIBLE rules. Re-runnable: deletes SEPH_Ent_* first.
Corridor: x -2180..2980, y 2020..2980 (wall faces), floor -76, ceiling 624. Vestibule: x -480..480, y 3020..3560."""
import unreal, math
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
FOLDER = "Facility/Front/Entrance_SEPH"
FLOOR, CEIL = -76.0, 624.0
X0, X1 = -2180.0, 2980.0           # corridor end walls
NWALL, SWALL = 2980.0, 2020.0       # corridor wall faces
M = lambda n: unreal.load_asset(n if n.startswith("/") else "/Game/Meshes/" + n)
for a in EAS.get_all_level_actors():
    if a.get_actor_label().startswith("SEPH_Ent_"): EAS.destroy_actor(a)
by = {a.get_actor_label(): a for a in EAS.get_all_level_actors()}
old = [l for l in by if l.startswith(("FAC_CorrCable_", "FAC_CorrPipe_")) or l == "FAC_Keypad_524"]
for l in old: EAS.destroy_actor(by[l])
print("removed old corridor cables/pipes/keypad:", len(old))

n = [0]
def put(mesh, loc, rot=(0, 0, 0), scale=(1, 1, 1), tag="x"):
    a = EAS.spawn_actor_from_object(mesh if not isinstance(mesh, str) else M(mesh), unreal.Vector(*loc), unreal.Rotator(*rot))
    a.set_actor_scale3d(unreal.Vector(*scale)); n[0] += 1
    a.set_actor_label(f"SEPH_Ent_{tag}_{n[0]}"); a.set_folder_path(FOLDER)
    return a
def rod(x, y, z0, z1, w=3.0, tag="Hanger"):           # thin steel rod from z0 up to z1 (MetalBeam10: 111x112x800, z 0..800)
    return put("SM_MetalBeam10", (x, y, z0), scale=(w / 111, w / 112, (z1 - z0) / 800), tag=tag)
def run_x(mesh, x0, x1, y, z, piece, rot, scale_other=(1, 1), tag="Run"):
    """Chain a mesh whose length axis maps to +X under `rot` from x0 to x1 exactly (pieces stretched evenly)."""
    k = max(1, round((x1 - x0) / piece)); L = (x1 - x0) / k
    for i in range(k):
        put(mesh, (x0 + i * L, y, z), rot, (scale_other[0], L / piece, scale_other[1]), tag)
    return k

# 1) Pipes: two runs wall-to-wall on the north wall (SM_Pipe_round_long: length 200 on +Y; yaw -90 -> +X)
run_x("SM_Pipe_round_long", X0, X1, 2930, 554, 200, (0, 0, -90), (1, 1), "Pipe")
run_x("SM_Pipe_round_long", X0, X1, 2870, 554, 200, (0, 0, -90), (0.7, 0.7), "Pipe")
for x in range(int(X0) + 200, int(X1), 400):
    rod(x, 2930, 575, CEIL); rod(x, 2870, 569, CEIL)

# 2) Power trunk: flat cable bundle on wall brackets, north wall under the pipes (SM_Cables04_06: 402 on -Y; yaw 90 -> +X)
TZ = 496.0; TY = NWALL - 24
run_x("SM_Cables04_06", X0, X1, TY, TZ, 402, (0, 0, 90), (1, 1), "Trunk")
for x in range(int(X0) + 100, int(X1), 200):
    put("SM_MetalBeam10", (x, NWALL - 27, TZ - 7), scale=(4 / 111, 54 / 112, 3 / 800), tag="Bracket")

# 3) Drops: trunk -> junction box -> floor socket -> device cord
def drop_to_box(x, box_z, wall_y=NWALL, floor_cord=True):
    # junction box on the wall (JunctionBox01: y 0..19 into room when yaw 180, z -1..39)
    put("SM_ElectricalSupply_JunctionBox01", (x, wall_y, box_z), (0, 0, 180), tag="JBox")
    top = box_z + 39
    put("SM_Cables05", (x + 2, wall_y - 6, top), scale=(1, 1, (TZ - 4 - top) / 200), tag="Drop")      # up into the trunk
    if floor_cord:
        put("SM_Cables05", (x - 8, wall_y - 6, FLOOR), scale=(1, 1, (box_z - FLOOR) / 200), tag="Drop")  # down to floor socket
        put("SM_ElectricalSupply_Socket01", (x - 8, wall_y - 3, FLOOR + 30), (0, 0, 180), tag="Socket")
        put("/Game/Scene_Warehouse/Assets/MS/3D/Ind_War_Cable_Power_Rubber_Weathered_03/SM_Ind_War_Cable_Power_Rubber_Weathered_03",
            (x - 40, wall_y - 25, FLOOR + 1), (0, 0, 90), tag="Cord")
for x in (-900, -600):                       # vending machine, water cooler
    drop_to_box(x, 150)
for lbl in ("FAC_Sconce_415", "FAC_Sconce_423"):   # north-wall corridor lights: conduit from trunk down to the fixture
    s = by.get(lbl)
    if s:
        o, e = s.get_actor_bounds(False)
        put("SM_Cables05", (o.x + 2, NWALL - 6, o.z + e.z), scale=(1, 1, (TZ - 4 - (o.z + e.z)) / 200), tag="Conduit")

# 4) Corridor ceiling light row (LampSet_01 modelled facing up -> roll 180 hangs it under the ceiling)
for x in range(int(X0) + 180, int(X1) - 100, 300):
    put("SM_LampSet_01", (x, 2500, CEIL), (180, 0, 90), tag="Strip")
    L = EAS.spawn_actor_from_class(unreal.RectLight, unreal.Vector(x, 2500, CEIL - 20), unreal.Rotator(0, -90, 0))
    c = L.rect_light_component; c.set_intensity_units(unreal.LightUnits.CANDELAS); c.set_intensity(400)
    c.set_source_width(140); c.set_source_height(20); c.set_attenuation_radius(900); c.set_editor_property("cast_shadows", False)
    c.set_light_color(unreal.LinearColor(0.95, 0.97, 1.0, 1)); n[0] += 1
    L.set_actor_label(f"SEPH_Ent_StripLight_{n[0]}"); L.set_folder_path(FOLDER)

# 5) Floor walkway paint down the corridor and from the vestibule (PedestrianPath01: 300 on -Y)
run_x("SM_PedestrianPath01", X0 + 30, X1 - 30, 2500, FLOOR + 0.5, 300, (0, 0, 90), (1, 1), "Walkway")
put("SM_PedestrianPath01", (0, 2980, FLOOR + 0.5), tag="Walkway"); put("SM_PedestrianPath01", (0, 2680, FLOOR + 0.5), (0, 0, 0), (1, 0.6, 1), tag="Walkway")

# 6) Vestibule: lights, card reader + conduit, camera, bench, notice board, extinguisher
for y in (3160, 3420):
    put("SM_LampSet_01", (0, y, CEIL), (180, 0, 0), tag="VestStrip")
L = EAS.spawn_actor_from_class(unreal.RectLight, unreal.Vector(0, 3290, CEIL - 20), unreal.Rotator(0, -90, 0))
c = L.rect_light_component; c.set_intensity_units(unreal.LightUnits.CANDELAS); c.set_intensity(500)
c.set_source_width(120); c.set_source_height(300); c.set_attenuation_radius(900); c.set_editor_property("cast_shadows", False)
L.set_actor_label("SEPH_Ent_VestLight"); L.set_folder_path(FOLDER)
put("SM_ElectricalSupply_JunctionBox04", (330, 3020, 120), (0, 0, 0), tag="CardReader")
put("SM_Cables05", (330, 3024, 127), scale=(1, 1, (CEIL - 127) / 200), tag="Conduit")
put("SM_VideoCamera_02", (430, 3045, CEIL - 22), (0, 0, 180), tag="Camera")
put("SM_Bench_01", (-480, 3150, FLOOR), (0, 0, -90), tag="Bench")
put("SM_WhiteBoard", (474, 3290, 210), (0, 0, 0), tag="Notice")
put("SM_fireextinguisher_01", (471, 3520, 30), (0, 0, 180), tag="Extinguisher")

print("placed", n[0])
print("saved", unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True))
