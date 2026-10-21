# Project Chimera — human first-person build and Saturday work playlist
**Owner review draft — prepared 2026-10-20.** Proposed next available Claude session: **Saturday 2026-10-24, around 11 PM Eastern** (owner estimate, not a deadline or booked session).

Related: [Master GDD](../MASTER_GAME_DESIGN.md) · [Hexapod animation inventory](../animation/HEXAPOD_ANIMATION_MATRIX.md) · [Issue #1: architecture](../../issues/1) · [Issue #7: hexapod rig](../../issues/7) · [Character/audio/VFX research PR #12](../../pull/12) · [Environment research PR #13](../../pull/13).

**Direction interpreted:** "one-point perspective, working from the human forward" = **human first-person player view** (FPS) as the next practical camera/control test. This is provisional. Game still retains the planned human FPS/TPS and creature TPS experiments until competitive camera rules are decided.

## What we actually have vs what we don't
| Element | Source | Current status | Next evidence |
|---|---|---|---|
| Small six-leg creature mesh | Meshy, per owner | **Reported** Blender model exists; part separation/refinement through Claude | File, license/provenance, local Blender path |
| Hexapod walking animation | Claude in Blender, per owner | **Reported visually walking well** | Save/examine .blend, Action names and clip preview; Unreal import not yet established |
| Other hexapod animations | Owner says some basics may exist | **Unknown / not cataloged** | Inventory original Blender Actions first; do not recreate blindly |
| Robot pinch/toe articulation | Future task | **Not established** | Test existing toe independence before changing skeleton |
| FPS human operatives | GDD intended, no evidence of rigged playable human here | **Planned** | Prototype FPS camera/controller, compatible body/skeleton and weapon pose |
| Production props/animations | Candidates in unmerged research PRs #12–13 | **Researched, not acquired/imported** | Licensing, compatibility, quality and actual project test |

## Playlist A — Phone-only work (no Claude runtime / no Unreal editor required)
These are **discrete independent packets**; no need to pretend a phone can remotely see or modify Blender without an active accessible computer integration.

| Order | Packet | Practical deliverable | Needs PC? |
|---|---|---|---|
| A01 | Inventory saved hexapod animations | Names of existing Blender Actions + links/screenshots/videos, no guessed statuses | Only owner-supplied data |
| A02 | Human operative visual reference choice | Reference/appearance brief: industrial tactical gear, believable silhouette, 3-person variants later | No |
| A03 | FPS camera rules | Camera at eye line, crouch level, flashlight visibility, head/body clipping, aim offset, recoil, human peripheral fairness vs creature | No |
| A04 | Human first-person interaction list | Open/close door, operate power panel, pick up parts, repair circuit, distress uplink, revive/help, weapon/item handling | No |
| A05 | Weapon handling contract | Pistol and rifle: idle, equip, aim-down-sights, fire, reload, empty, swap, inspect; socket and animation montage hooks | No |
| A06 | Human movement requirements | Idle, walk, sprint, crouch, stop, turn, jump/fall/land, lean (only if approved), damage/death | No |
| A07 | First room reference packet | One realistic service corridor -> server bay -> breaker/terminal, with meaningful FPS sightlines and sound occlusion | No |
| A08 | Rigging batch plan | Separate reusable **robot leg**, human arms/gun, full human skeleton, interaction props; ordered by shared skeleton and dependencies | No |
| A09 | Asset shortlist review | Check listed Fab/Epic/Kenney pages and flag licensing, AI-use tags and actual target UE version | Phone browser |
| A10 | Codex independent review | Check FPS camera + input contract against UI/QA draft PRs #10–11; flag any mismatch, don't merge blindly | GitHub/Codex phone |
| A11 | Mechanical interaction priorities | Choose one removable PSU/fan component and one breaker door to prototype; no magical remote sabotage | No |
| A12 | Claude Saturday handoff review | Decide first measurable UE test; ensure all files/paths/versions are available | No |

## Playlist B — Asset shortlist to examine, not purchase
Sources below come from the previously prepared **unmerged research** and may have changed. Verify license, current price, UE compatibility, quality, and actual import. **AI-use restrictions matter**: do not send assets tagged "AI use: No" into generative modeling tools. Purchased binary files must not go into Chimera's public GitHub repository.

### Human / first-person / weapons
| Priority | Candidate | Why it helps | Limit / caution |
|---|---|---|---|
| 1 | [Game Animation Sample — Epic / Fab](https://www.fab.com/listings/880e319a-a59e-4ed2-b268-b32dac7fa016) | Large free UE locomotion reference for human run/jump/land/traversal | Research notes **AI use: No**. Not the full rifle/pistol interaction set; retarget/import untested |
| 1 | [Quantum Modular Character Free Sample — Fab](https://www.fab.com/listings/8e200050-3158-4762-b297-f785b5b1533d) | Test human full-body rig and modular gear without buying full cast | Free sample; character style, skinning and target-engine compatibility not tested |
| 1 | [Animation Starter Pack — Epic / Fab](https://www.fab.com/listings/98ff449d-79db-4f54-9303-75486c4fb9d9) | Backup prototype clips, good for rapid FPS controller placeholder | Legacy skeleton retargeting; **AI use: No** per research |
| 2 | [Modular Military Character — Fab](https://www.fab.com/listings/d39bebbc-2e92-4ef9-a5b1-831e04f6a51c) | Potential final tactical-body candidate | Pricing/version pending; license and skeleton checks |
| 2 | [The Locomotion Animation Pack — Fab](https://www.fab.com/listings/c9d28dc9-7730-468e-8bfb-f2b2c464d9be) | Paid candidate for FPS/TPS aiming, fire and reload | **AI use: No** per research; price, weapon sockets and target UE retarget unverified |
| 2 | [VanillaLoop Free Sample Animation Set — Fab](https://www.fab.com/listings/5b737d10-b4cd-4294-8f6b-757b9088039d) | Flashlight/pickup/interactions reference | Sample coverage partial; **AI use: No** per research |
| 2 | [Mixamo — Adobe FAQ](https://helpx.adobe.com/creative-cloud/faq/mixamo-faq.html) | Fallback humanoid animations and retarget experiments | Does not solve hexapod rig; review Adobe use/redistribution terms |
| 2 | Existing Unreal / starter weapon placeholders | FPS gun and arm placement test without paid model | Validate source/rights; replace placeholders later, no scope cut |

### One useful facility room to test human view
| Priority | Candidate | Why it helps | Limit / caution |
|---|---|---|---|
| 1 | [Jobsite VOL.15 Tech and Data — Dekogon / Fab](https://www.fab.com/listings/5c88b22b-14d3-4401-baef-156f637d311e) | Server-bay set dressing and realism | Research says UE5.1–5.8; **AI use: No**; rack internals may not detach; pricing may change |
| 1 | [Modular Bunker — Trapdoor Studios / Fab](https://www.fab.com/listings/4c25afae-8527-477e-8d86-e79a3bf6e4ee) | Tactical concrete corridor, doors and facility tone | Confirm actual target UE compatibility; trial layout only |
| 1 | [Data Center AI Rack and Liquid Cooling Equipment Kit — Fab](https://www.fab.com/listings/73496c2a-ce8a-40d4-8f1b-a9f0b48b549d) | Candidate separated doors/compute sleds and removable-looking machinery | AI-use tag reported Yes; not proof that harvestable internals exist |
| 2 | [Industrial Catwalks Set — Fab](https://www.fab.com/listings/522128db-02fa-44f3-8a18-359a6bc291f5) | Human visibility vs wall/ceiling creature routes | No creature adhesion or character traversal included |
| 2 | [Buildings VOL.6 Electrical Boxes — Dekogon / Fab](https://www.fab.com/listings/f2cc527e-f241-4d09-b6df-9464dd1462ce) | Reachable power breaker props | AI use: No reported; actual hinge sockets need implementation |
| 2 | [Poly Haven](https://polyhaven.com/) / [ambientCG](https://ambientcg.com/) | Free CC0 material reference if needed later | Not essential to rig or animation pipeline |
| 2 | [Modular Industrial Catwalk Kit Free — Fab](https://www.fab.com/listings/a78ee253-9182-4c4a-8520-30ebd1433e9c) | Cheap/free temporary traversal coverage | Research reported low rating; inspect before relying on it |

### Audio / VFX sources for the room (later integration)
| Candidate | Use / caveat |
|---|---|
| [Kenney Impact Sounds](https://kenney.nl/assets/impact-sounds) | Free CC0 basis for metal impacts / destruction; creature gait needs curated sound layers |
| [Kenney Sci-fi Sounds](https://kenney.nl/assets/sci-fi-sounds) | Free CC0 synthetic servo/alert ideas; avoid arcade audio |
| [Kenney Interface Sounds](https://kenney.nl/assets/interface-sounds) | Free CC0 tactical UI and machine acknowledgements |
| [Unreal Content Examples](https://dev.epicgames.com/documentation/unreal-engine/content-examples-sample-project-for-unreal-engine) | Niagara, animation, and effect-system references; licenses per sample |
| [Free Niagara Muzzle Flash — Fab](https://www.fab.com/listings/435b3bcb-d7f5-467d-99aa-2edc97a6c5fd) | Potential free muzzle flash; actual import and visual fit untested |

### Separate/custom asset slots we must not forget
- **Hexapod:** master .blend/armature, six leg chains, existing walking Actions, two-digit foot/claw (future), removable armor, mount sockets; future upper chassis and module mount development.
- **Human:** FPS camera and hands/arms rig, full-body third-person silhouette for opponents, rifle/pistol meshes, magazine, flashlight, functional gadget mounts; one common human skeleton with variants rather than independently bespoke incompatible skeletons.
- **Interaction:** rack door, breaker cover, an **individually detachable** fan/PSU, a repairable power connector, and actual socket transforms for in-world reach/grab animations.
- **Environment:** one server aisle, maintenance access and an overhead route with authored collision/contact surfaces.

## Playlist C — Saturday Claude sessions: runnable, evidence-driven
No existing Unreal project modifications until owner authorizes them; these are queued instructions, not background tasks. **Each job must have a separate input/output and proof.**

| Order | Job | Required output | Pass criteria |
|---|---|---|---|
| C01 | **BACK UP** existing Blender hexapod first | Dated original .blend duplicate + list of Actions | Current walking rig survives unchanged |
| C02 | Audit existing armature and saved clips | Name list, active Actions, skeleton hierarchy, montage export plan | No phantom clips / no lost animation |
| C03 | Export one actual walk | Blender export + UE-compatible FBX/animation with known units/axes | Unreal import, no broken pivots/foot sliding; report engine version |
| C04 | Build a tiny **human FPS** test | Simple playable human character + eye-height camera, existing starter body as placeholder | Responsive W/A/S/D, mouse look, camera collision, crouch/sprint |
| C05 | Fit human full body / FPS hands | Single compatible skeleton and gun/hand sockets | Other players can see full human; local camera doesn't clip face/body |
| C06 | Add rifle/pistol prototype states | Idle, ADS, fire, reload montages and socket alignment | View remains stable; no unsupported 'finished gunplay' claim |
| C07 | Add first physical interaction | Reach panel -> open door -> remove part -> update world state | Animation action aligns with component attach/detach; not remote hacks |
| C08 | Creature run + slow stalk | Two separate gait Actions or procedural derivation | At intended speeds no contact sliding; walker remains intact |
| C09 | Creature traversal feasibility | Tiny isolated floor-wall-ceiling test (controller + rig) | Foot orientation and body collision correct on transition; no teleport |
| C10 | Creature lunge / ceiling drop | Montages with marked hit/landing frames | Recovery, safe miss, and gameplay validation documented |
| C11 | Save verified outputs and integration proof | Screenshots/video, FBX, notes, paths, README, UE PIE test | Mark **Verified** only after evidence, not after a render |

> If the session is short: prioritize **C01–C04**. This selects the next test, not a reduction of the agreed long-term feature list. Avoid burning hours generating a new mesh if the existing one is salvageable.

## Intended contract for first-person vs creature camera
- Humans need authoritative movement and real-world positional firing/interaction. Camera and hand animations are visual; server controls damage and harvesting.
- A local FPS arm/weapon presentation must coexist with a full-body human actor for other clients, without separate competing gameplay state.
- Third-person and optional isometric competitive fairness remain deliberate design tests, not automatically removed.
- Creature climbs, wall jumps, ceiling drops and leg IK must respect collision, known anchor/surface geometry, camera rotation comfort and multiplayer prediction.

## Phone-only work is not Blender access
Claude's ability to use a computer, screen and Blender in the earlier live session does not imply this ChatGPT conversation can inspect Claude's local unsaved .blend, or that the same computer will be remotely reachable from a phone. Until files/screen recordings are uploaded, **Claw/Claude's actual implementation cannot be merged or verified here**. GitHub source-control entries cannot substitute for unsaved local Blender assets.

## Approval and provenance
- Research PRs and this playlist are **drafts**; no unreviewed merges, paid purchases, UE modifications or model rewrites authorized here.
- Store paid mesh/audio packs outside public GitHub, with private project references and provenance. Observe Fab AI-use restrictions.
- Preserve all long-term gameplay, evolution, voice, crew roles, and missions. **October 31 is a checkpoint, not a scope-cut deadline.**
