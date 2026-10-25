# Move the editor viewport camera; a second call (vp_shot.py) takes the screenshot after exposure settles.
import unreal, json
P = json.load(open(r"C:\Users\nickf\Documents\Unreal Projects\Chimera\Saved\SephScripts\tools\vp.json"))
p = unreal.Vector(*P["pos"]); t = unreal.Vector(*P["tgt"])
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(p, unreal.MathLibrary.find_look_at_rotation(p, t))
