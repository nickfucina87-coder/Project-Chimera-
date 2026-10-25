# Chimera - Unreal (UE 5.8) project files

Only the project-authored parts of `Documents/Unreal Projects/Chimera` live here. Marketplace / sample content
(Scene_Warehouse, Fab, Megaplant, NiagaraExamples, /Game/Meshes factory kit, etc., ~19 GB) is not in git, so
`Lvl_Warehouse` will show missing meshes on a fresh clone until that content is installed.

| Path | What |
|---|---|
| `Chimera.uproject`, `Config/` | project file (adds the `Chimera` C++ module) and config (hardware ray tracing off) |
| `Source/Chimera/` | playable hexapod: `AHexapodCharacter` (surface climbing, corner crawl, gait selection, grab/upright/pounce/buck, zoom), `AHexapodPlayerController` (gravity-relative camera), `AHexapodGameMode` |
| `Content/Chimera/`, `Content/__ExternalActors__/Chimera/` | `Lvl_Warehouse` (World Partition, one file per actor) and project meshes/materials |
| `Content/Characters/HexapodV2/` | hexapod skeleton, 46 `A_Hex_*` animations, inputs. The 276 MB skeletal mesh asset is NOT committed - re-import `hexapod/Ironclad_Hexapod_v2_Animated.glb` to `/Game/Characters/HexapodV2` (see `Tools/import_glb.py`) |
| `Tools/` | editor automation used to build the level (run inside the editor via Web Remote Control: `python rc.py <script>`) |
| `HANDOFF.md` | current state, structure notes and next steps - read first |

Build: close the editor, then
`Engine\Build\BatchFiles\Build.bat ChimeraEditor Win64 Development -Project=<path>\Chimera.uproject`.
