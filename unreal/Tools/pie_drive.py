import unreal, json, os
SP = r"C:\Users\nickf\AppData\Local\Temp\claude\C--Users-nickf\0df46d9a-2366-4026-9062-dc693361cd0b\scratchpad"
cmd = json.load(open(os.path.join(SP, "drive_cmd.json")))
worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
if not worlds: print("NO PIE"); raise SystemExit
W = worlds[0]
pawn = unreal.GameplayStatics.get_player_pawn(W, 0)
pc = unreal.GameplayStatics.get_player_controller(W, 0)
if cmd.get("place"):
    x, y, z, yaw = cmd["place"]
    pawn.set_actor_location_and_rotation(unreal.Vector(x, y, z), unreal.Rotator(0, 0, yaw), False, True)
    pc.set_control_rotation(unreal.Rotator(0, -15, yaw))
if cmd.get("move"):
    f = pawn.get_actor_forward_vector()
    pawn.add_movement_input(f, cmd["move"], True)
subs = [s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)]
sub = subs[-1] if subs else None
IA = lambda p: unreal.load_asset(p)
if "inject" in cmd:
    a = IA(cmd["inject"]["action"]); val = cmd["inject"]["value"]
    vv = [float(x) for x in val] + [0.0] * (3 - len(val))
    v = unreal.InputActionValue(vv[0], vv[1], vv[2], unreal.InputActionValueType.AXIS2D if len(val) == 2 else unreal.InputActionValueType.BOOLEAN)
    if cmd["inject"].get("continuous"): sub.start_continuous_input_injection_for_action(a, v, [], [])
    elif cmd["inject"].get("stop"): sub.stop_continuous_input_injection_for_action(a)
    else: sub.inject_input_for_action(a, v, [], [])
if cmd.get("speed"):
    pawn.character_movement.set_editor_property("max_walk_speed", cmd["speed"])
if cmd.get("shot"):
    unreal.SystemLibrary.execute_console_command(W, "HighResShot 960x540")
mesh = pawn.mesh
inst = mesh.get_anim_instance()
asset = inst.get_animation_asset() if hasattr(inst, "get_animation_asset") else None
l = pawn.get_actor_location(); u = pawn.get_actor_up_vector(); v = pawn.get_velocity()
print(f"loc({l.x:.0f},{l.y:.0f},{l.z:.0f}) up({u.x:.2f},{u.y:.2f},{u.z:.2f}) v{v.length():.0f} mode {pawn.character_movement.movement_mode} anim {asset.get_name() if asset else None}")



