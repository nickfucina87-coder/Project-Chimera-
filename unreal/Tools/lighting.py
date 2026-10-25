"""Daytime + working lights. Re-runnable: deletes and rebuilds actors labelled SEPH_Light_*; scales existing lights
from their ORIGINAL intensity (stored once in a tag) so repeated runs don't compound."""
import unreal, json, os
P = json.load(open(r"C:\Users\nickf\Documents\Unreal Projects\Chimera\Saved\SephScripts\tools\lighting.json"))
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = EAS.get_all_level_actors()
for a in acts:
    if a.get_actor_label().startswith("SEPH_Light_"): EAS.destroy_actor(a)
acts = EAS.get_all_level_actors()

# --- sun + sky
sun = [a for a in acts if isinstance(a, unreal.DirectionalLight)][0]
sun.set_actor_rotation(unreal.Rotator(0, P["sun_pitch"], P["sun_yaw"]), False)
sc = sun.light_component
sc.set_intensity(P["sun_lux"]); sc.set_light_color(unreal.LinearColor(1.0, 0.95, 0.88, 1)); sc.set_editor_property("cast_shadows", True)
sun.set_actor_label("HZ_Sun"); sc.set_editor_property("volumetric_scattering_intensity", 0.15)
sky = [a for a in acts if isinstance(a, unreal.SkyLight)][0].light_component
sky.set_intensity(P["sky"]); sky.set_light_color(unreal.LinearColor(1, 1, 1, 1))
try: sky.set_editor_property("real_time_capture", True)
except Exception as e: print("skylight realtime:", e)

# --- exposure
ppv = [a for a in acts if isinstance(a, unreal.PostProcessVolume)][0]
s = ppv.settings
for k, v in (("auto_exposure_min_brightness", P["ev_min"]), ("auto_exposure_max_brightness", P["ev_max"]), ("auto_exposure_bias", P["ev_bias"]),
             ("vignette_intensity", 0.4)):
    s.set_editor_property("override_" + k, True); s.set_editor_property(k, v)
ppv.settings = s

# --- existing lights: scale from original
for a in acts:
    if not isinstance(a, unreal.Light) or isinstance(a, unreal.DirectionalLight): continue
    c = a.get_component_by_class(unreal.LightComponent)
    tag = [str(t) for t in a.tags if str(t).startswith("orig_i=")]
    if tag: orig = float(tag[0][7:])
    else:
        orig = c.intensity; a.tags = list(a.tags) + [unreal.Name(f"orig_i={orig}")]
    col = c.light_color
    red = col.r > 200 and col.g < 60
    f = P["red_scale"] if red else P["existing_scale"]
    if str(a.get_folder_path()).startswith("Horror/Lights/Exterior"): f = 1.0
    c.set_intensity(orig * f)

# --- high-bay grid: hanging lamp + downlight
LAMP = unreal.load_asset("/Game/Scene_Warehouse/Assets/MS/3D/Ind_War_Light_Ceiling_Metal_Hanging_02/SM_Ind_War_Light_Ceiling_Metal_Hanging_02")
n = 0
for x, y, z in P["bays"]:
    lamp = EAS.spawn_actor_from_object(LAMP, unreal.Vector(x, y, z))
    lamp.set_actor_scale3d(unreal.Vector(3, 3, 3)); lamp.set_actor_label(f"SEPH_Light_Lamp_{n}"); lamp.set_folder_path("Facility/Lights/HighBay")
    sp = EAS.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(x, y, z - 90), unreal.Rotator(0, -90, 0))
    lc = sp.spot_light_component
    lc.set_intensity_units(unreal.LightUnits.CANDELAS); lc.set_intensity(P["bay_cd"])
    lc.set_light_color(unreal.LinearColor(1.0, 0.93, 0.82, 1)); lc.set_attenuation_radius(2600)
    lc.set_outer_cone_angle(62); lc.set_inner_cone_angle(30); lc.set_editor_property("cast_shadows", P["bay_shadows"])
    sp.set_actor_label(f"SEPH_Light_Bay_{n}"); sp.set_folder_path("Facility/Lights/HighBay"); n += 1
print("sun", P["sun_lux"], "high-bays", n)
print("saved", unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True))

