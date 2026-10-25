import unreal
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for cmd in ("r.Lumen.HardwareRayTracing 0", "r.RayTracing.Enable 0"):
    unreal.SystemLibrary.execute_console_command(W, cmd)
print("HWRT now", unreal.SystemLibrary.get_console_variable_int_value("r.Lumen.HardwareRayTracing"))
n = 0
for a in EAS.get_all_level_actors():
    if isinstance(a, unreal.SceneCapture2D):
        c = a.capture_component2d
        c.capture_scene()                 # one still frame so the monitors aren't black
        c.set_editor_property("capture_every_frame", False)
        c.set_editor_property("capture_on_movement", False)
        n += 1
print("security cameras frozen:", n)
print("saved:", unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True))
