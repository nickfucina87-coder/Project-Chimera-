# CHM-007 — Hexapod animation inventory and rig validation
**Draft for owner review — 2026-10-20.** Related to [Issue #7](../../issues/7), [Master GDD](../MASTER_GAME_DESIGN.md), and [animation sourcing PR #12](../../pull/12).

## Evidence / current state (do not turn reports into verified assets)
- **Owner-reported:** Meshy provided a hexapod mesh; Claude worked live in Blender, separated/refined parts, rigged it, and achieved a functioning walking animation. The owner could inspect the scene and direct refinements interactively. A second assistant (reported as Quac) was compared; Claude was preferred.
- **Owner-reported:** Other basic animations may be present, but the owner has **not saved/listed all clips**. Do not mark any unspecified animation completed.
- **Not inspected here:** original Meshy provenance, Blender .blend, skeleton, animation action names, screen recordings, FBX export, UE import, engine runtime, replicated locomotion, playable controls.
- **Current asset verification:** `Walk` = **OWNER-REPORTED WORKING IN BLENDER** only. All other rows below = **PLANNED / STATUS UNKNOWN** until enumerated against the actual Blender file.
- Rig and asset work are now being done through **Meshy + Claude/Blender** rather than relying on the old Tripo-only wording in Issue #7. The old design brief is superseded *for tool assignment only*, not for accepted gameplay requirements.
- **No binary model or animation has been uploaded to GitHub by this document.** Avoid committing purchased/source third-party assets to this public repository.

## Core contract
One reusable six-leg skeleton, with **independent rigid upper/lower leg armor and explicit hip, knee, ankle and toe/gripper pivots**. A functioning visual walk does not establish whether toes can pinch. Do **not** tear up the existing working rig to force toe independence; save a versioned duplicate and do that later.

For every motion:
- One authored animation/action or an explicit note that it is **procedural/IK** or a **transition** rather than a dedicated clip.
- Record action name, duration, frame rate, looping vs one-shot, root-motion policy, required IK/foot contact events, collision changes, attach points, and a captured preview.
- Keep joint topology/names identical across reusable locomotion assets; note the coordinate conventions used when exported to Unreal.
- Use **root motion or in-place movement deliberately**. Never apply both mesh translation and gameplay actor translation without a contract.
- For traversal, the **controller handles collision, surface normals, attachment and movement authority**. Animation alone does not grant wall/ceiling navigation.
- For harvest/attacks, hit frames and attach/detach cues must align with server-validated interaction events; don't animate a detached component before the server authorizes it.
- Avoid arbitrary new creature powers: motions here describe a production capability inventory, not automatic additions to the GDD.

## Full animation inventory
**Stage legend:** R = owner-reported Blender demonstration; ? = unverified/not cataloged; P1 = immediate import/movement priority; P2 = vertical traversal/combat priority; P3 = broader gameplay and polish. This is **sequence**, not cutting scope.

### A. Ground locomotion
| Action / candidate Unreal ID | Priority | Status | Notes |
|---|---|---|---|
| Walk cycle — `HX_Walk` | P1 | **R** | Blender demo only; verify real Action and export |
| Idle mechanical micro-adjustments — `HX_Idle` | P1 | ? | Keep relatively still for stalking |
| Run / fast scuttle — `HX_Run` | P1 | ? | Faster alternating tripod gait, distinct timing |
| Slow stealth crawl — `HX_Stalk` | P1 | ? | Lower silhouette, softer contact |
| Accelerate / decelerate — `HX_Start`, `HX_Stop` | P1 | ? | Can be additive/transition driven |
| Turn in place L/R — `HX_TurnL`, `HX_TurnR` | P1 | ? | Distinct from actor yaw |
| Strafe L/R — `HX_StrafeL`, `HX_StrafeR` | P1 | ? | Footplant / clearance test |
| Reverse scuttle — `HX_Backward` | P1 | ? | Avoid naive reverse footage if feet slide |
| Sharp 180-degree turn — `HX_Turn180` | P2 | ? | High speed vs careful turn options |

### B. Adhesion and facility traversal
| Action | Priority | Status | Notes |
|---|---|---|---|
| Ground-to-wall attach; wall-to-ground detach | P2 | ? | Controller/capsule rotates onto a verified climbable surface |
| Wall crawl and wall idle | P2 | ? | Contact points follow wall normal |
| Wall-to-ceiling and ceiling-to-wall | P2 | ? | Explicit change of gravity/contact frame |
| Ceiling crawl and ceiling hang | P2 | ? | Camera/hitbox and player orientation constraints |
| Convex/outside and concave/inside corner | P2 | ? | Space and leg clearance |
| Squeeze/fold for maintenance gap; unfold | P2 | ? | Different collision envelope validated in UE |
| Traverse rafter/catwalk underside; perch | P3 | ? | Surface placement and clearance |
| Enter/exit a damaged vent/hatch | P3 | ? | Vent animation may share squeeze/unfold |

### C. Jump, drop and recovery
| Action | Priority | Status | Notes |
|---|---|---|---|
| Jump wind-up, launch, air pose, land | P2 | ? | Prefer modular phases and readable grounded timing |
| Run-to-leap / pounce initiation | P2 | ? | Separate combat/gameplay movement |
| Jump onto wall / attach; jump onto ceiling / attach | P2 | ? | Surface validation and safe miss behavior |
| Leap from wall to floor | P2 | ? | Preserve world-space orientation and landing |
| Ceiling detach/drop | P2 | ? | Controlled falling rather than instant teleport |
| Soft landing | P2 | ? | Normal contact and quick recovery |
| Hard landing, stumble, recovery | P2 | ? | Falls / missed pounces / impact response |
| Right self from side/back | P2 | ? | Avoid collision tunneling |

### D. Combat / predation
| Action | Priority | Status | Notes |
|---|---|---|---|
| One-leg claw strike L/R | P2 | ? | Keep support legs stable |
| Forward lunge + recovery | P2 | ? | Proposed first attack montage |
| Pounce, connect and miss | P2 | ? | Server hit window and recovery |
| Alternating two-claw combo | P3 | ? | Do not assume available foot-digit grip |
| Wall / ceiling ambush opener | P3 | ? | Attack from above with fair audiovisual tell |
| Grab/latch, hold/pin, release | P3 | ? | Requires authority, target attachment and counterplay |
| Finishing attack, victim release | P3 | ? | Only after human death rules/kill-camera policy |

### E. Harvest, sabotage and manipulation
| Action | Priority | Status | Notes |
|---|---|---|---|
| Scan/inspect selected component | P2 | ? | Link to targeted rack part |
| Reach a service panel with front leg | P2 | ? | IK may align to per-object socket |
| Two-digit pinch open/close | P3 | ? | **Future independent toe separation**; no claim present rig supports it |
| Grip item; carry; release/drop | P3 | ? | World socket ownership and replication |
| Pry open armor/door/panel | P2 | ? | Add prop hinge or separate movable panel |
| Remove PSU/fan/logic-board or actuator | P2 | ? | Physical actor/component removal after authorization |
| Install salvage into module socket | P3 | ? | Pending module recipe and socket contracts |
| Sabotage controls/camera/power junction | P2 | ? | Tied to visitable actual device |
| Remove jammed vent cover / unlock hatch | P3 | ? | Alternate entrance animations |

### F. Stealth, reactions, damage and chassis changes
| Action | Priority | Status | Notes |
|---|---|---|---|
| Sensor pan and target acquisition / alert | P2 | ? | Readable for humans, not ESP |
| Freeze / shutdown-looking concealment | P2 | ? | Preserve counterplay |
| Compact idle/perch | P2 | ? | Motionless ceiling/rafter silhouette |
| Light hit and heavy knockback | P2 | ? | Hit direction and leg contact |
| Electrical stun/shutdown | P2 | ? | Distinct timed states |
| Limp/fault on damaged leg | P3 | ? | Optional procedural gait change |
| Repair/reconfigure, module reveal | P3 | ? | Explicit animation socket |
| Destruction / collapse | P2 | ? | Server match state drives destruction |
| Boot up and emergency movement burst | P3 | ? | No gameplay ability automatically implied |
| Chassis switch / reconstruction staging | P3 | ? | Full biped later uses a distinct rig |

## What can be done from the phone before Claude access returns
- Catalog the owner's known Actions/videos by **name and status** when available; no unconfirmed completion marks.
- Review and assign 1–2 reference clips for ground-to-wall, a lunge, and a component-removal movement; purely reference, no generation charge necessary.
- Prepare human first-person locomotion, ADS, reload, flashlight and interaction requirements in [Saturday production queue](../production/SATURDAY_ASSET_FPS_QUEUE.md).
- Decide whether the six toes are currently fixed decorative meshes or independent moving objects **once the rig file is available**. No blind surgery on the walking rig.

## Claude re-entry checklist (when available, Saturday 2026-10-24 ~23:00 ET per owner)
1. Preserve a dated, working `.blend` backup **before altering rig hierarchy**.
2. Inventory existing Actions, NLA tracks, armature bone names, weight/parenting and Blender viewport animation. Do not regenerate existing clips.
3. Export a single walk Action and import into the actual Unreal target project using verified skeleton orientation/scale; check feet, speed, retarget policy, and asset naming.
4. Add **one** locomotion gap (run or stalking gait) and test on ground.
5. Prototype **floor -> wall -> ceiling -> drop** in a tiny isolated movement test **only after** a valid controller/surface traversal mechanism exists; keep old working gait.
6. Only then touch pinch/toe separation, combat, harvesting and full animation suite.

## Rig/animation acceptance proof (not yet satisfied)
- Source `.blend` path and version, action list, demonstrative video, exported `.fbx`/animation path.
- No floating feet or joint popping at movement speed; no root double-translation; foot IK states identifiable.
- Mesh/component origins, joint axes, hierarchy, collision proxies, performance, and Unreal skeletal import inspectable.
- At least one Unreal PIE loop clip verified in the *actual* project; peer clients to be tested later.
- Approval is separate from integration and verified gameplay.

**Never remove a feature because October 31 is near; that is only a checkpoint.**
