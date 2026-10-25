import unreal, json, os
SP = r"C:\Users\nickf\AppData\Local\Temp\claude\C--Users-nickf\0df46d9a-2366-4026-9062-dc693361cd0b\scratchpad"
c = json.load(open(os.path.join(SP, "vel.json")))
W = unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
p = unreal.GameplayStatics.get_player_pawn(W, 0); m = p.character_movement
m.set_editor_property("ground_friction", 0.0); m.set_editor_property("braking_deceleration_walking", 0.0)
f = p.get_actor_forward_vector(); r = p.get_actor_right_vector()
v = f * c["fwd"] + r * c["right"]
m.set_editor_property("velocity", v)
inst = p.mesh.get_anim_instance()
print(c, "->", inst.get_animation_asset().get_name())
