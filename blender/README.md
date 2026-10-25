# Chimera Blender warehouse

## Current scene

[Deep elevator and secret B4 level](deep_elevator/README.md) is the newest saved scene. It extends the Maintenance 17 elevator 82 m below the warehouse and adds two concealed rooms. Its folder contains the scene parts, verified restore script, build script, and five previews.

## Earlier geometry QA scene

**Chimera_Warehouse_Geometry_QA.blend** (24 October 2026) contains the warehouse, naturalized environment, and geometry QA state before the deep elevator addition. To restore it, download this folder or clone the repository, then run:

```sh
python blender/restore.py
```

The script joins four numbered parts, verifies SHA-256 checksums, and creates `blender/Chimera_Warehouse_Geometry_QA.blend`. This folder also contains FBX exports, CSV manifests, JSON QA reports, and PNG previews. Earlier `.blend` working snapshots, Blender `.blend1` backups, and the 97 MB tour render are not included.
