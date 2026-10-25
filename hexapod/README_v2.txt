Ironclad Hexapod - Animation v2 (2026-10-25)
Rebuild: blender -b Ironclad_Hexapod_Anim.blend --python build_hexapod_anims_v2.py   (v1 .blend is read, never overwritten)
Previews: blender -b Ironclad_Hexapod_Anim_v2.blend --python render_sheets_v2.py [-- ActionName ...]  -> previews_v2\*.png
UE FBX:   blender -b Ironclad_Hexapod_Anim_v2.blend --python export_anims_light.py  -> Ironclad_Hexapod_Anims_v2_light.fbx

FILES
 Ironclad_Hexapod_Anim_v2.blend   rig + mesh + 34 actions (fake-user, Action Editor to browse)
 Ironclad_Hexapod_Anims_v2_light.fbx  rig + proxy cube, all actions (same import route as v1_light)
 anim_manifest_v2.json            per action: frames, loop, root motion, speed, joint/stretch stats
 previews_v2\                     contact sheet per action

Loops carry a Cycles modifier so they repeat in Blender at any timeline length. All 24 fps, forward -Y, legs solved by analytic IK on the real joint pivots, baked to FK keys.
v1 Blender-IK constraints/CTRL empties removed (they solved fake vertical bone segments).

LOCOMOTION (in place, loop)           speed for blendspace
 Walk_Fwd 32f 0.38 m/s | Walk_Backup 32f 0.30 | Strafe_L/R 28f 0.34 | Run_Fwd 18f 1.33
 Sprint_Fwd 14f 2.08 (low, leaning) | Gallop_Fwd 14f 2.57 (bound: rear->mid->front, flight phase)
 Stealth_Crawl 72f 0.09 (wave gait, one leg at a time, body low) | Turn_L/R 28f 44 deg/s
 Walk_TripodGait = original walk kept. Wall/ceiling crawling: use these same loops with the actor aligned to the surface.
IDLE  Idle 72f | Idle_LookAround 120f | Idle_Ceiling_Hang 72f (body sags away from the ceiling)
JUMP  Jump_Start 12f (launch capsule at f9) -> Jump_Fall loop 24f -> Jump_Land 18f (f0 = contact); Jump_InPlace 42f
GRAB  Grab_Reach 30f (object ~0.8 m ahead on the ground) -> Grab_Hold loop 48f -> Grab_Release 20f
POSES Reach_Up 48f | Stand_Upright_Start 30f -> _Loop 48f -> _End 30f | Attack_Stab 30f | Hit_React 16f | Death 48f

ROOT MOTION (enable root motion on these; Root bone carries translation + rotation)
 Walking corner crawls - each starts on the surface in its name (so they read correctly in Blender):
 Trans_Floor_To_Wall 48f | Trans_Wall_To_Ceiling 48f (head-up on wall -> upside down) | Trans_Ceiling_To_Wall 48f
 Trans_Wall_To_Floor 48f (head-down) | Trans_Floor_To_Ceiling 96f | Trans_Ceiling_To_Floor 96f (one continuous crawl)
 Corner geometry: next surface 0.85 m ahead of the root.
 Trans_Over_Ledge      48f outside corner: over an edge 0.62 m ahead and down the face, ends 0.65 m below
 Jump_Wall_To_Floor    35f head-DOWN on wall, pushes off, lands f19 ~1.3 m below, ~0.9 m out, facing away
 Drop_Wall_To_Floor    35f head-UP on wall, falls back, lands f19 facing the wall
 Drop_Ceiling_To_Floor 36f upside down on ceiling, half-roll, lands f20 (~2 m ceiling)
 Heights/distances are fixed. For other heights, cut to Jump_Fall/Jump_Land at the landing frame.

KNOWN LIMITS
 Rig is asymmetric (Meshy mesh): R legs reach farther than L. Fast gaits stretch L_rear slightly (<=1.17x reach);
 Reach_Up and Trans_Over_Ledge peak at ~1.25x (visible as a fully straight leg, no mesh break). Death curls to 94 deg.
 No foot/toe bones, so no foot roll. Feet are tilted toward the surface normal on contact.
