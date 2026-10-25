# Nuclear brighten for L_CHM_ComponentTest — unhide lights, raise exposure, tame fog.
# Safe to re-run. Tweaks at top.
import unreal
TARGET_BIAS = 0.5          # UE: +brightens. Was -2.5 (cave). Try 0..1.5
SKY_INTENSITY = 0.35
SUN_INTENSITY = 0.15       # keep low so floors don't nuke white
ASTRA_LIGHT_MULT = 0.7     # restore some of Astra's point/spot intensity from tags
FOG_DENSITY = 0.02
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
TAG = "SephOrigI="
done = {"ppv": 0, "sky": 0, "sun": 0, "points": 0, "unhidden": 0, "fog": 0}

def orig_or(comp, a):
    for t in a.tags:
        if str(t).startswith(TAG):
            return float(str(t)[len(TAG):])
    return comp.get_editor_property("intensity")

with unreal.ScopedEditorTransaction("Seph nuclear brighten") as tx:
    for a in EAS.get_all_level_actors():
        lab = a.get_actor_label()
        # Unhide anything lighting-related that we stashed
        try:
            if a.is_temporarily_hidden_in_editor():
                a.set_is_temporarily_hidden_in_editor(False)
                done["unhidden"] += 1
            a.set_actor_hidden_in_game(False)
        except Exception:
            pass

        if lab == "Seph_PPV_LabMood" or (isinstance(a, unreal.PostProcessVolume) and "Seph_PPV" in lab):
            s = a.settings
            for k, v in (
                ("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL),
                ("auto_exposure_apply_physical_camera_exposure", False),
                ("auto_exposure_bias", TARGET_BIAS),
                ("bloom_intensity", 0.2),
                ("vignette_intensity", 0.35),
            ):
                try:
                    s.set_editor_property("override_" + k, True)
                    s.set_editor_property(k, v)
                except Exception as e:
                    unreal.log_warning("PPV skip %s: %s" % (k, e))
            a.set_editor_property("settings", s)
            a.set_editor_property("priority", 50.0)
            a.set_editor_property("unbound", True)
            done["ppv"] += 1

        elif isinstance(a, unreal.SkyLight):
            c = a.light_component
            c.set_visibility(True)
            c.set_intensity(SKY_INTENSITY)
            done["sky"] += 1

        elif isinstance(a, unreal.DirectionalLight):
            c = a.light_component
            c.set_visibility(True)
            c.set_intensity(SUN_INTENSITY)
            done["sun"] += 1

        elif isinstance(a, (unreal.PointLight, unreal.SpotLight, unreal.RectLight)):
            c = a.light_component
            c.set_visibility(True)
            if lab.startswith("Seph_"):
                # our pools / red fills — keep modest
                try:
                    cur = c.get_editor_property("intensity")
                    if cur < 50:
                        c.set_intensity(200.0)
                except Exception:
                    c.set_intensity(200.0)
            else:
                o = orig_or(c, a)
                c.set_intensity(max(o * ASTRA_LIGHT_MULT, 80.0))
            done["points"] += 1

        elif isinstance(a, unreal.ExponentialHeightFog):
            f = a.component
            try:
                f.set_editor_property("fog_density", FOG_DENSITY)
                f.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.04, 0.05, 0.07, 1))
                done["fog"] += 1
            except Exception as e:
                unreal.log_warning("fog: %s" % e)

        elif isinstance(a, unreal.SkyAtmosphere):
            try:
                a.set_is_temporarily_hidden_in_editor(False)
                a.set_actor_hidden_in_game(False)
            except Exception:
                pass

unreal.log("Seph nuclear brighten: bias=%s %s" % (TARGET_BIAS, done))
LES.save_current_level()
