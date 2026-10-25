# Chimera - handoff notes (for the next chat)

Last updated 2026-10-25. Read this first, then `WarehouseBible/WAREHOUSE_BIBLE.md`.

## Project
- UE 5.8 project: `C:\Users\nickf\Documents\Unreal Projects\Chimera\Chimera.uproject`
- THE level: `/Game/Chimera/Maps/Lvl_Warehouse` (World Partition; actors live in `Content/__ExternalActors__/Chimera/Maps/Lvl_Warehouse`).
  NOT `ThirdPerson/L_CHM_ComponentTest` (old test map; user wants it gone eventually - ask before deleting).
- Factory sample (style reference "bible"): `/Game/Maps/Demonstration` (+ Overview, TechArt). Same mesh library as ours (`/Game/Meshes`).
- Hexapod animations live outside UE: `C:\Users\nickf\Documents\Chimera_Hexapod_Anim\` (see README_v2.txt there).

## How to drive the editor (no MCP restart needed)
- Editor must be open. Web Remote Control listens on 127.0.0.1:30010; MCP server on 127.0.0.1:8000/mcp.
- Run any Unreal Python file inside the editor:  `python -I tools\rc.py <script.py>`
  (PUTs to `/remote/object/call` -> `PythonScriptLibrary.ExecutePythonCommandEx`, ExecuteFile mode; prints log output).
- Real screenshot of what the user sees (editor viewport, real exposure):
  `powershell tools\vpshot.ps1 <name> x y z  tx ty tz` -> `tools\shots\vp_<name>.png`.
- Scene-capture snapshots (`tools\snap.py` + `shots\shots.json`, mode.txt = `lit` or `base`): base-colour mode is great for
  seeing geometry/placement; lit capture IGNORES auto-exposure, don't judge brightness with it.
- `tools\ue.py` = minimal MCP JSON-RPC client (toolsets are sandboxed; Remote Control python is the workhorse).
- Always save before/after bulk changes: `unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)`.

## Level layout (cm, floor z = -76, building x -4500..4500, y -3000..3600, roof trusses ~1209-1774)
Outliner folders = rooms:
- `Facility/Shell` - blockout walls/floor (engine Cube meshes, labels FAC_Ext*, FAC_*Split*, *Lintel* = door headers).
- `Facility/ServerHall` x -4250..0, y -2750..1500 (+Overhead trays/light strips, Safety). User LIKES the ground level here.
- `Facility/Power` x 500..4500, y -2980..-500 (boilers, cooling towers, chiller, switchboards, walkway at z 346).
- `Facility/Workshop` x 500..4500, y -500..2000; `Workshop/Chamber` = hexapod build/test chamber x 1640..3430.
- `Facility/Front` y 2000..3600 (entrance at x -300..300 on y=3580, corridor, lab, security office, wash room, staging),
  low ceiling (RoofFront z 644).
- `Facility/Roof`, `Facility/Overhead` (catwalks, wiring), `Facility/Lights/*`, `Facility/Signs`, `Facility/Cameras`.
- Elevator shaft + secret room: `Warehouse/DeepElevator_B4` (shaft at x -4100 y -2600 down to z -8500), `Facility/B4_Control`.
- `Warehouse/Environment_Placeholders` = outdoor terrain/cliffs/river/bridge placeholders. PlayerStart_Bridge at (0,5144,150).
- Actor label prefixes: FAC_* (earlier facility passes), HZ_* (env/horror), SEPH_Light_* (high-bay lamps; rebuilt by lighting.py).

## What has been done (2026-10-25)
- Perf: Lumen hardware RT off (runtime + DefaultEngine.ini `r.RayTracing=False`, `r.Lumen.HardwareRayTracing=0`;
  backup `Config/DefaultEngine.ini.bak_20261025`; full effect after editor restart). 8 security SceneCapture2Ds set to
  NOT capture every frame (frozen still) - re-enable `capture_every_frame` per camera if live feeds are needed.
- Removed all broken/damage props (Facility/Damage/*), 37 mid-air cables/motor row/fans/floor-cable grid,
  fixed 7 sunk + 4 floating props (monitors now sit on the floor - need desks), sealed the power-room back-wall hole
  (`FAC_ExtBack_Patch`), moved FAC_CoolTower_1404 inside.
- Daylight: `tools\lighting.py` + `lighting.json` (sun 15000 lux pitch -48, sky 3.0, exposure EV 1..9 bias +1,
  existing lights x60 from original intensity stored in actor tag `orig_i=`, red lights x10, 13 high-bay lamps+spots).
  Re-runnable. User: "just need to be able to see", no atmospherics work right now.

## 2026-10-25 later: strip + server hall rebuild
- User disliked the entrance pass: ALL props outside the server hall were deleted (structure kept). Backup of the level
  before the strip: `Saved\Backups\Lvl_Warehouse_before_strip_20261025\` (umap + ExternalActors folder).
- Server hall rebuilt by `tools\build_server.py` (re-runnable, label prefix SEPH_SV_, folder Facility/ServerHall/SEPH):
  main beams on pillar lines (z700), strut grid (z667), wire-mesh trays = SM_Fence_01 rolled 270 over every rack pair (z270)
  with cable bundles, CableSet_02 risers from every rack row, hanger rods, collector tray at x150 -> UPS/switchgear bank on
  east wall -> risers + main cable up to z700 into the power-room wall; light-strip hanger wires; 3 cooling ducts with vents
  (z1050), 5 CRAC units on north wall, wall vents, zone signs, aisle floor mats. Stair rail FAC_RailWest_318 shortened.
- Editor crashes on load_asset paths with '//' (fatal). If Remote Control returns empty output, the user is in PIE.

## User's working rules (important)
- Do things in the ORDER the user gives. When told to stop, stop and wait. Structure first, not atmosphere.
- Room by room, starting at the ENTRANCE. Show screenshots after each room.
- Every cable/pipe must attach at both ends and be oriented correctly (nothing upside down); every machine, server row
  and rack should have power runs that lead back toward the power room (floor along walls/bases, or up to the beam grid).
- Delete-and-rebuild of unconnected cables is approved. No "broken stuff" for now.

## Next steps (in order)
1. Entrance (Facility/Front around x -300..300, y 2000..3600): clean unconnected cables/pipes, then dress to the bible -
   proper entry (lit doorway, signage, security/check-in point, floor markings), wall panels with real cable runs,
   ceiling light rows, clutter.
2. Then corridor/front rooms -> server hall (fix cable orientation, add power runs to each row, hanger rods for trays)
   -> workshop/chamber -> power room -> overhead/roof -> elevator/B4.

## Playable hexapod (2026-10-25)
- C++ module `Source/Chimera` (added to Chimera.uproject; backup Chimera.uproject.bak_20261025). Build with
  `Engine\Build\BatchFiles\Build.bat ChimeraEditor Win64 Development -Project=...` while the editor is CLOSED (Live Coding blocks it),
  or Ctrl+Alt+F11 Live Coding for edits to existing classes.
- AHexapodCharacter: sphere capsule 48, custom-gravity climbing (inside corner = probe ahead; outside corner = probe under the lip,
  teleport onto face), jump detaches, sticks to walls when jumping into them, visual slerp of mesh after each surface change.
  Single-node animation: BS_Hex_Locomotion (Direction -180..180, Speed 0..300) + clips for jump/fall/land/grab/attack/upright/stealth/ceiling idle.
- AHexapodPlayerController: gravity-relative camera (Epic custom-gravity pattern). AHexapodGameMode picks PlayerStart tag "Hexapod".
  Lvl_Warehouse World Settings GameMode override = HexapodGameMode; PlayerStart_Hexapod in the server hall cross aisle.
- Assets: /Game/Characters/HexapodV2 (glb import: mesh+skeleton+38 A_Hex_* anims, BS_Hex_Locomotion, Input/IA_Hex_*, IMC_Hex).
  Source glb: Documents\Chimera_Hexapod_Anim\Ironclad_Hexapod_v2_Animated.glb (export_glb_v2.py). The FBX-imported anims
  (/Game/Characters/Hexapod/Anims, /AnimsV2) have WRONG bone frames vs the glb mesh - do not use them.
- Collision: 27 level meshes got box collision; stairs use complex; cable meshes have no blocking collision.
- Wiring prototype: one rack pair (y -618, east block) via tools\proto_rack.py (SEPH_SV2_*); rest still has old SEPH_SV_ wiring.

## Update (later 2026-10-25)
- Blend space dropped (python-authored samples never built); gait clips chosen in C++ by speed/direction with play-rate matching.
- Corner crawl: BeginCorner/UpdateCorner arc the body around the edge over CornerTime (MOVE_None during the crawl, gravity slerped).
- Jump anticipation (launch on Jump_Start frame 9). New anims (glb re-import, 46 total): GrabWalk_Fwd/Back, GrabStrafe_L/R,
  Upright_Walk_Fwd/Back, Upright_Pounce (launch on f13), Upright_Buck. Inputs: IA_Hex_Zoom (mouse wheel), IA_Hex_Buck (RMB).
- PIE automation: injected input does not reach an unfocused PIE window; tools\vel.py tests clip selection by setting velocity.
- Repo: git not installed, no local repo yet (user has a GitHub "Chimera" project; needs URL + Git/LFS install).
