Ironclad Hexapod - Animation v1 (first pass, Seph)
Built 2026-10-24 by build_hexapod_anims.py (Blender 5.2, headless). Re-run: blender -b --python build_hexapod_anims.py

FILES
 Ironclad_Hexapod_v2_Skinned_src.glb  - copy of Content/Characters/Hexapod GLB (original untouched)
 Ironclad_Hexapod_Anim.blend          - working file (rig + IK + all actions)
 backups\                             - timestamped .blend after import / IK / head / gaits
 Ironclad_Hexapod_Anims_v1.fbx        - armature+mesh, all actions baked (-Z fwd, Y up, no leaf bones)
 preview_*.png                        - Workbench stills; build_log.txt - run log

ACTIONS (24 fps, loopable, in place)
 Walk_TripodGait       0-28.8  base walk, head rot/loc damped to ~55%
 Walk_TripodGait_ORIG  0-28.8  untouched original
 Sprint_Gallop         0-16    row-wave gallop, 0.9 leg-length stride, body bob
 Strafe_L / Strafe_R   0-28    tripod gait sideways, slight sway
 Walk_Backup           0-32    tripod gait in reverse
 CTRL_<action>_<leg>           IK-target source keys (empties only; not exported)

RIG
 IK on each *_ankle (chain 3, use tail) -> CTRL_IK_<L|R>_<front|mid|rear>, pole CTRL_POLE_*; all parented to Root bone.
 Gait actions are BAKED to FK bone keys; IK influence keyed 0 in every action. To edit a gait: set IK influence 1,
 assign the CTRL_* actions to the empties, tweak, re-bake (or just edit the script and re-run).

OPEN: double-click the .blend, Action Editor -> pick action. UE: import FBX, Import Animations on, skeleton = new or existing HexapodRig.

KNOWN LIMITS
 Mechanical v1: linear keys, no weight/impact easing, no foot roll (no foot/toe bones). Head not animated in new gaits.
 No grab/drag bones, no wall-crawl/ceiling/jump yet. Gallop is a fast row-wave, not a true bound.
