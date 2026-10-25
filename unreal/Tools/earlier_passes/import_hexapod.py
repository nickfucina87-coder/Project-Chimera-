import unreal
dst = "/Game/Characters/Hexapod"
if unreal.EditorAssetLibrary.does_asset_exist(dst + "/Ironclad_Hexapod_v2_Skinned"):
    unreal.log("Seph: hexapod already imported")
else:
    t = unreal.AssetImportTask()
    t.filename = r"C:/Users/nickf/Documents/Unreal Projects/Chimera/Content/Characters/Hexapod/Ironclad_Hexapod_v2_Skinned.glb"
    t.destination_path = dst
    t.automated = True
    t.save = True
    t.replace_existing = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
    unreal.log("Seph: imported -> %s" % list(t.imported_object_paths))
