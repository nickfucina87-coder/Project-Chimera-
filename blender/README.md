# Chimera Blender warehouse

The current saved Blender scene is **Chimera_Warehouse_Geometry_QA.blend** (24 October 2026). It contains the warehouse, naturalized environment, and geometry QA state. The scene has no external Blender library, image, sound, or movie-clip dependencies.

GitHub's connected upload path required the 66 MB scene to be stored as four numbered parts. Download this folder or clone the repository, then run:

```sh
python blender/restore.py
```

The script joins the parts, verifies each part and the final SHA-256 checksum, and creates `blender/Chimera_Warehouse_Geometry_QA.blend`. Open that file in Blender. Python 3.8 or newer is required.

This folder also contains FBX exports, CSV manifests, JSON QA reports, and PNG previews from the warehouse and environment work. The numbered parts and these supporting files are source assets; keep all four parts together when downloading. Earlier `.blend` working snapshots, Blender `.blend1` backups, and the 97 MB tour render are not included in this repository copy.
