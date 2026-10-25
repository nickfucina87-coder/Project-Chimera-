import unreal
AR = unreal.AssetRegistryHelpers.get_asset_registry()
flt = unreal.ARFilter(class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh")], package_paths=["/Game"], recursive_paths=True)
assets = AR.get_assets(flt)
print("static meshes in project:", len(assets))
import re
pat = re.compile(open(r"C:\Users\nickf\Documents\Unreal Projects\Chimera\Saved\SephScripts\tools\pattern.txt").read().strip(), re.I)
for a in assets:
    p = str(a.package_name)
    if pat.search(p):
        m = unreal.load_asset(p)
        b = m.get_bounding_box()
        print(p, f"size=({b.max.x-b.min.x:.0f},{b.max.y-b.min.y:.0f},{b.max.z-b.min.z:.0f}) z[{b.min.z:.0f},{b.max.z:.0f}]")
