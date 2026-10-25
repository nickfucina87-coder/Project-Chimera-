# Temporary capture camera -> PNG. Shots defined in SHOTS: (name, (x,y,z), (tx,ty,tz), fov)
import unreal, json, os
OUT = r"C:\Users\nickf\Documents\Unreal Projects\Chimera\Saved\SephScripts\tools\shots"
os.makedirs(OUT, exist_ok=True)
SHOTS = json.load(open(os.path.join(OUT, "shots.json"), encoding="utf-8-sig"))
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
rt = unreal.RenderingLibrary.create_render_target2d(W, 1280, 720, unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB)
cam = EAS.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0))
c = cam.capture_component2d
c.texture_target = rt
MODE = os.environ.get("SNAPMODE", "lit") if False else open(os.path.join(OUT, "mode.txt")).read().strip() if os.path.exists(os.path.join(OUT, "mode.txt")) else "lit"
c.capture_source = unreal.SceneCaptureSource.SCS_BASE_COLOR if MODE == "base" else unreal.SceneCaptureSource.SCS_FINAL_TONE_CURVE_HDR
c.capture_every_frame = False
try:
    for name, pos, tgt, fov in SHOTS:
        p = unreal.Vector(*pos); t = unreal.Vector(*tgt)
        rot = unreal.MathLibrary.find_look_at_rotation(p, t)
        cam.set_actor_location_and_rotation(p, rot, False, False)
        c.fov_angle = fov
        for _ in range(3): c.capture_scene()      # let exposure/Lumen settle a little
        unreal.RenderingLibrary.export_render_target(W, rt, OUT, name + ("_base" if MODE == "base" else "") + ".png")
        print("shot", name)
finally:
    EAS.destroy_actor(cam)
