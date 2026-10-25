# Seph lighting fix for L_CHM_ComponentTest -- underground-lab mood. Safe to re-run (absolute values, originals kept in tags).
# Set REVERT = True and re-run to put every touched light back to its original intensity.
import unreal
REVERT = False
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
MAP = "/Game/ThirdPerson/L_CHM_ComponentTest"
W = unreal.EditorLevelLibrary.get_editor_world()
if W is None or W.get_path_name().split(".")[0] != MAP:
    LES.load_level(MAP)

COOL = unreal.LinearColor(0.70, 0.82, 1.0, 1)   # cold fluorescent
WARM = unreal.LinearColor(1.0, 0.72, 0.45, 1)   # sodium / tungsten
TAG = "SephOrigI="

def remember(a, comp):
    for t in a.tags:
        if str(t).startswith(TAG): return float(str(t)[len(TAG):])
    v = comp.get_editor_property("intensity")
    a.tags = list(a.tags) + [unreal.Name(TAG + "%.4f" % v)]
    return v

def setI(a, comp, v, color=None):
    orig = remember(a, comp)
    comp.set_intensity(orig if REVERT else v)
    if color is not None and not REVERT: comp.set_light_color(color)

done = {"seph_lights": 0, "sun": 0, "sky": 0, "other_lights": 0, "fog": 0, "disabled": 0}
with unreal.ScopedEditorTransaction("Seph lighting fix") as tx:
    for a in EAS.get_all_level_actors():
        lab = a.get_actor_label()
        if lab.startswith("Seph_PPV") or lab.startswith("Seph_Lt_"):
            EAS.destroy_actor(a); continue  # rebuilt below
        if isinstance(a, unreal.DirectionalLight):
            setI(a, a.light_component, 0.02); done["sun"] += 1
            if not REVERT:
                try:
                    a.set_actor_hidden_in_game(True)
                    a.light_component.set_visibility(False)
                    done["disabled"] += 1
                except Exception:
                    pass
        elif isinstance(a, unreal.SkyLight):
            setI(a, a.light_component, 0.01); done["sky"] += 1
            if not REVERT:
                try:
                    a.light_component.set_visibility(False)
                    done["disabled"] += 1
                except Exception:
                    pass
        elif isinstance(a, (unreal.PointLight, unreal.SpotLight, unreal.RectLight)):
            c = a.light_component
            if lab.startswith("Seph_"):
                i = done["seph_lights"]
                setI(a, c, 180.0 if i % 2 == 0 else 100.0, COOL if i % 3 else WARM)
                if not REVERT:
                    c.set_attenuation_radius(550.0)
                    c.set_editor_property("cast_shadows", i % 2 == 0)
                done["seph_lights"] += 1
            else:
                orig = remember(a, c)
                c.set_intensity(orig if REVERT else orig * 0.20)
                done["other_lights"] += 1
        elif isinstance(a, unreal.ExponentialHeightFog) and not REVERT:
            f = a.component
            f.set_editor_property("fog_density", 0.06)
            f.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.01, 0.015, 0.03, 1))
            f.set_editor_property("enable_volumetric_fog", True)
            done["fog"] += 1
        elif isinstance(a, unreal.SkyAtmosphere) and not REVERT:
            try:
                a.set_actor_hidden_in_game(True)
                a.set_is_temporarily_hidden_in_editor(True)
            except Exception:
                pass

    if not REVERT:
        # unbound post process: lock exposure DOWN (UE bias: +brightens, -darkens)
        ppv = EAS.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
        ppv.set_actor_label("Seph_PPV_LabMood"); ppv.set_folder_path("Seph/Lighting")
        ppv.set_editor_property("unbound", True)
        ppv.set_editor_property("priority", 50.0)
        s = ppv.settings
        for k, v in (("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL),
                     ("auto_exposure_apply_physical_camera_exposure", False),
                     ("auto_exposure_bias", -2.5),
                     ("bloom_intensity", 0.15),
                     ("vignette_intensity", 0.65),
                     ("color_saturation", unreal.Vector4(0.85, 0.9, 1.0, 1.0)),
                     ("film_toe", 0.6)):
            try:
                s.set_editor_property("override_" + k, True)
                s.set_editor_property(k, v)
            except Exception as e:
                unreal.log_warning("Seph: PPV skip %s (%s)" % (k, e))
        # Also override any existing level PPV by bumping ours
        ppv.set_editor_property("settings", s)
        # two dim red emergency fills in opposite corners
        mn = [1e9]*3; mx = [-1e9]*3
        for a in EAS.get_all_level_actors():
            if not isinstance(a, unreal.StaticMeshActor): continue
            o, e = a.get_actor_bounds(False)
            if max(e.x, e.y) > 20000: continue
            for i, (c, x) in enumerate(((o.x, e.x), (o.y, e.y), (o.z, e.z))):
                mn[i] = min(mn[i], c - x); mx[i] = max(mx[i], c + x)
        for n, (x, y) in enumerate(((mn[0] + 150, mn[1] + 150), (mx[0] - 150, mx[1] - 150))):
            pl = EAS.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, mn[2] + 120), unreal.Rotator(0, 0, 0))
            c = pl.point_light_component
            c.set_intensity(180.0); c.set_attenuation_radius(700.0)
            c.set_light_color(unreal.LinearColor(1.0, 0.08, 0.04, 1)); c.set_editor_property("cast_shadows", False)
            pl.set_actor_label("Seph_Lt_RedFill_%d" % n); pl.set_folder_path("Seph/Lighting")

unreal.log("Seph lighting: %s REVERT=%s" % (done, REVERT))
LES.save_current_level()
