# PROJECT CHIMERA — Master Game Design Document
**Version:** 0.7 — shared-component direction  
**Date:** 2026-10-24  
**Status:** Preproduction. Owner authorized the shared-component direction, all seven build families, and the bounded T01 prototype on 2026-10-24. Runtime readiness remains unverified.  
**Milestone:** October 31 is a **progress checkpoint**, not a release deadline or reason to remove features.

## Revision 0.7 — authoritative additions
- [Shared-component design](SHARED_COMPONENT_DESIGN.md): physical parts, seven builds, acquisition routes and machine vision.
- [First test plan](COMPONENT_TEST_PLAN.md): three rooms, three donors, six parts, two upgrades and both playable roles.
- [Asset acquisition checklist](ASSET_ACQUISITION_CHECKLIST.md): full breakdown and evidence-based local inventory.
- Offline human play against AI and co-op PvE are now product goals alongside the original asymmetric PvP.
- Shared components are physical instances; broad material families below remain categorization/future recipes, not an automatic conversion of every part into generic currency.
- T01 sequencing supersedes the broad phase order for the next implementation task; the whole-game scope remains intact.
- Detailed numerical recipes/ranges/cooldowns are provisional tuning proposals. Time manipulation remains exploratory.
- This revision authorizes inspection and the bounded test; it does not certify any local implementation or asset ownership.

## 1. High concept
Project Chimera is a four-player asymmetric survival-horror game: **three human operatives versus one player-controlled synthetic entity** in a damaged, classified data-center, robotics, and military-research complex. The operatives must restore power, transmit a distress signal, and extract. The creature begins as a small six-legged mechanical scavenger, selectively dismantles machinery for resources, sabotages systems physically, assembles unpredictable but learnable abilities, and may remain small or reconstruct into a tall skeletal biped.

**Signature conflict:** humans rebuild the facility to escape; their component and access decisions shape what the entity can become. The entity may preserve/tap live infrastructure or harvest it for onboard hardware. Changes must have visible and audible consequences.

## 2. Design pillars
1. **Quiet tactics become escalating chaos.** Opening minutes reward investigation, stalking, concealment, and restraint; player decisions intensify the endgame.
2. **Skillful hunter versus skillful hunter.** Humans have lethal but ammo-constrained weapons; creature has subterfuge, mobility, limited backup bodies, and terrain control. Neither side automatically dominates.
3. **Physical, selective dismantling.** Target separate machine components; harvesting changes machine state and may influence nearby power, security, and sound.
4. **Choice, not forced evolution.** Staying small is viable. Growing changes routes, silhouettes, vulnerabilities, and attacks—not simply health.
5. **Atmosphere conveys information.** Metal taps, fans stopping, alarms, power transitions, rack lights, and shadows provide useful clues rather than arbitrary jump scares.
6. **Repeatable complete matches.** One match must have setup, opposing objectives, consequences, conclusion, and rematch.
7. **Preserve the vision.** Incremental implementation is sequencing, not feature cancellation.

## 3. Audience and play format
- Initial match: four human players total, **3 operatives vs. 1 entity**.
- Long-term configurable population: potentially five or six total players; balance and rules TBD.
- Desired full match: **15–20 minutes**; variable by play and objectives.
- PC / Unreal Engine target; exact engine version, platforms, input support, and shipping online backend need verification.
- Experimental FPS/TPS switching for player roles; competitive camera restrictions and optional isometric mode remain testable design variables.
- Gameplay feel: deliberate survival-horror gunplay, scarce ammunition, tactical tools, intimidation, and emergent player stories.
- Visuals: grounded photorealistic horror, realistic industrial architecture against unsettling synthetic engineering.
- Voice vision: proximity speech that the creature may overhear, plus possible equipment/radios; initially can use external voice while integrated voice is developed.

## 4. Teams and player loops
### Operatives (three players)
Deploy > scout > preserve/restore infrastructure > secure objectives > deal with sabotage and stalking > transmit signal > escape.
- Pistol and rifle planned, low ammunition, flashlight and a limited utility gadget set.
- Gadgets can ultimately include scanning, hacking, distractions, alarms, door overrides, traps, and surveillance tools.
- Death: shift to **remote operator** participation via working security cameras, threat pings, and limited facility actions; limitations and counterplay preserve competitive fairness.
- Victory is **objective completion and extraction**, not automatically a creature kill.
- Persistent progression envisioned, but unlocks must preserve competitive parity.

### Synthetic entity (one player)
Spawn as small hexapod > locate machinery > select individual components > harvest while exposed/noisy > assemble modules > physically sabotage facility > stalk and ambush > block extraction or eliminate team.
- Primary weapons are traps, environment, deception, positional advantages; limited physical attacks remain possible.
- Early body can fit server gaps and maintenance routes.
- Overhead routes, rafters, support structures, and ceilings are significant map and animation goals.
- Monster can stay small or construct / select a large skeletal biped chassis as a strategic tradeoff.
- Limited backup chassis allow survival after body destruction at a cost; exact life count and penalties are balance decisions.
- Abilities are usually concealed until use; effects should offer subtle readable tells and counters.
- Creature victory: eliminate all operatives or prevent extraction until match expires (proposed; review pending).

## 5. Mission 001 — LAST TRANSMISSION
**Location:** Containment Facility 01, a compact interconnected complex.
**Premise:** Research facility communications fail following the release of a synthetic intelligence.
**Human objectives:**
1. Enter through staging/loading perimeter and locate distribution controls.
2. Restore emergency power through a solvable set of circuits/panels.
3. Access communications suite and activate a distress uplink.
4. Hold or maintain uplink while it transmits.
5. Reach an extraction zone and extract at least one operative.

**Creature counterplay:** select components to harvest; interrupt or delay power and communications; manipulate shutters and lighting by physically visiting controls; isolate operatives; set traps near likely routes; stalk extraction.

**Proposed round results (not final balance):**
- Power restored + signal transmitted + at least one operative extracts: human win.
- All operatives eliminated, or timer ends before qualified extraction: entity win.
- Creature final body destroyed: humans retain objective obligations; emergency escape rules to be specified.
- Disconnects, stalemates, and partial extraction must have explicit server-authoritative handling.

**Mission replayability:** long-term randomized mission selection; different objective arrangements, scavenging opportunities, hazards and side tasks. Mission 001 is the first fully specified match, not the full content limit.

## 6. Containment Facility 01 — level design
Four interconnected sectors:
1. **Server Hall:** tight aisle occlusion, individual harvestable boards/PSUs/fans, service channels, rack lighting and sound.
2. **Robotics Workshop:** salvageable actuators and alloy, high ceiling, gantries and suspended industrial equipment.
3. **Power / Cooling:** restoreable breakers, UPS, generators, switchgear, emergency lights, hot or electrically hazardous zones.
4. **Communications / Loading:** uplink objective, extraction, shutters, surveillance consoles, access-controlled doors.

Navigation occurs at **service level** (small passages and maintenance cavities), **human level** (hallways, rooms, loading, stairs) and **overhead level** (selected cable trays, catwalks and rafters).
- Proposed first layout envelope ~80m x 60m; actual blockout must prioritize sightlines and travel times over an arbitrary footprint.
- Maintain multiple routes and emergency overrides so harvesting cannot unwittingly create permanently unwinnable objectives.
- Strong visual landmarks and audio zones for player orientation.
- Monster concealment must work through actual occluders and predictable visibility rules, not just dark postprocessing.

## 7. Creature anatomy and evolution
### Hexapod — initial chassis
- Six independently articulated insect-like legs, unnerving asymmetric rhythm, small central armored core, sensor cluster, detachable module sockets.
- Initial *proposed* bounding envelope: ~0.9m long x 0.75m wide x 0.55m high, to be validated in Unreal.
- Locomotion: idle, scuttle, directional turn, sprint, enter/exit narrow gap, selected overhead traversal, harvest, hit, shutdown, attacks, sabotaging.
- Rig: rigid hard-surface segments, semantic pivot hierarchy, three primary motion joints per leg; visually separated pieces/consistent scale; collision proxies distinct from visuals.
- Authored gait initially, with optional advanced IK/procedural foot placement in future.

### Harvester — selectable larger chassis
- Tall skeletal mechanical biped with elongated arms, salvage sockets, larger presence and direct action.
- Separate rig/animation blueprint and collision geometry, but shared authoritative player identity, modules, recipes and match state.
- Proposed scale ~2.4m tall, for review.
- Large chassis does not fit all maintenance paths; instead gains force, reach, overhead or heavy-system interactions.
- Physical body reconstruction and module attachment are long-term full-scope commitments, not removed because animation work is difficult.
- Switching may occur through assembly stations or another rule determined in review.

### Backup bodies
- Limited reserve chassis. A destroyed active body may transfer/reboot to a reserve at a known eligible site, potentially losing resources.
- Reserve count, vulnerability, spawn rules, penalties, anti-camping and score effects require playtesting.
- Humans should receive evidence a body was destroyed but that the entity may persist.

## 8. Salvage and crafting
**Interaction:** aim/select individual authored panel or component > align / begin dismantling > observable time-and-noise action > server validates detach > detached mesh/particle response > inventory update > dependent facility systems recalculate.

Proposed material families:
- Processing / logic boards: sensing, spoofing, electronic intrusion.
- Power assemblies / batteries: electrical and burst-energy abilities.
- Actuators / alloy / cooling mechanisms: locomotion, armor, physical interference.

Candidate combination examples, not final approved recipes:
- Power + Logic -> short-range ghost pulse/false telemetry.
- Logic + Mechanicals -> shutter control parasite at visited machinery.
- Power + Mechanicals -> temporary energized environmental trap.

Recipes can be discoverable; mechanics, counters and resource yields should be deterministic or transparently seeded for competitive fairness. Modules should affect movement and visible body configuration where practicable.

**Critical rule:** a salvage consequence may impede a human objective, but the dependency graph must guarantee repair paths or a deliberate, well-advertised loss condition.

## 9. Power, cameras, security, sound and horror
- Physically-visited creature sabotage: panels, circuits, shutters, cameras, alarms; no unrestricted magical map-wide remote hacking by default.
- States replicate: powered, offline, damaged, repairing, overridden, removed-part state.
- Audio: mechanical footfalls by material and gait, fan stop/start, switchgear clicks, gunfire, rack harvest, metal strain, sparks, alarms, ambient electrical rooms.
- Horror-first audio is **positional and truthful** when it signals actual events; deliberate false-signal abilities must be learnable/counterable.
- Lighting: normal, degraded emergency, blackout, with gameplay state replicated and cosmetic scalability.
- Human post-death operator: authorized CCTV, limited pings and restricted facility inputs; creature can counter by destroying power/cameras or planting decoys.
- Integrated proximity voice and monster overhearing are in the long-term design, with communication privacy and sabotage balancing to be specified.

## 10. Gunplay, deaths and progression
- Human firearms initially pistol + rifle, deliberate handling, reload/aim animations, constrained ammunition.
- Creature survival relies on movement, ambush, system interactions, and limited reserve bodies rather than being invulnerable.
- Human elimination changes role rather than forcing idle spectator time.
- Short-term match progression belongs to each team; long-term account progression should focus on fair customization, unlock options, achievements and non-pay-to-win breadth.
- Random missions, multiple modular monsters, advanced transformations, richer equipment, bigger sites, potential six-player format all remain in scope for iterative development.

## 11. Camera and information contract
- User wants first/third-person switching. Both controllers should support camera-policy configuration.
- Experiment: human FPS, human TPS, entity TPS and optionally isometric tactical mode; do not assume these are equally fair.
- Camera collision, corner-peeking, ceiling visibility, and networked target acquisition require playtests.
- Dead-player CCTV must not freely reveal unrendered or unauthorized map state.
- Clients may choose views; server authorizes abilities and all game-state changes.

## 12. UI / UX
Main menu; Host/Join; role/ready lobby; player settings; mission loading/briefing; operative HUD (health, ammo, objective, gear); creature HUD (resources, active chassis, modules); component targeting and harvest progress; objective world markers with restraint; dead-player remote console; debrief (winners, extraction, power restored, components harvested, chassis losses, kills, major sabotage); rematch; account progression; accessible control and audio options. All planned screens stay in product scope.

## 13. Network and integration contract
- GameMode: authoritative roles, rules, spawn, transition and win conditions.
- GameState: replicated phase/timer/objectives/power/extraction outcome.
- PlayerState: persistent-within-session team, role, observer status, result/score identity.
- PlayerController: local input, view policy, authorized UI / CCTV selection.
- Pawn/replicated components: motion, combat, interaction, chassis, inventory/recipe, sabotage.
- Server validates harvest target & range/duration, state transitions, damage, recipes, backup respawns, objective dependencies, spectator camera control, and match result.
- Gameplay state replicates; effects may render locally from replicated events. Use deterministic identifiers and event logging for troubleshooting.
- Network testing must include separate machines and actual internet connectivity, not just local editor clients.
- Backend, authentication, voice, hosting, NAT/relay choices and anti-cheat policy require technical review.

## 14. Asset and animation contracts
**Humans:** one humanoid compatible skeleton with outfit variations; idle/walk/run/crouch/aim/fire/reload/hit/death, flashlight and interactions.  
**Hexapod:** separate mechanical part meshes, root/leg segments/sockets, PBR, LOD strategy, animated locomotion and harvest.  
**Large predator:** independent compatible rig, biped locomotion and assembly configuration.  
**Environment:** modular walls/floors/ceilings, server bays and removable parts, robotics props, power equipment, doors/shutters, gantries, maintenance gaps, communications suite, loading extraction assets.  
**Audio/VFX:** realistic ambience, six-legged taps and joint servos, diagnostic beeps, alarms, crackle, sparks, flashlight/power transitions, gunfire, impacts, creature assembly, UI feedback.  
**Definition of game-ready asset:** scale in cm, useful pivot/root arrangement, appropriate topology/UV/PBR, collision approach, licensing/source, export format, rig or animation compatibility proof, integration owner, and version metadata. A static 3D mesh alone is not a rigged character.
- Asset IDs and skeleton conventions to be specified before production.
- Record reference, license, acquisition status, compatibility, owner, and QA result per asset.
- Use verified licenses; do not distribute unlicensed franchise-derived assets.

## 15. Agent ownership and coordination
**Owner/creative director:** approves gameplay and scope, chooses priorities, evaluates fun, retains final integration authority.  
**ChatGPT:** keeper of reviewable specs, cross-agent dependency review, issue planning, consistency and acceptance-criteria analysis.  
**Claude:** Unreal technical lead; server gameplay, replication architecture, runtime interaction, integration.  
**Codex:** UI screens, independently scoped systems/tests, build scripts, pull-request review; avoid concurrent ownership of Claude's runtime modules.  
**Grok/Grokbots:** asset licensing and sourcing, reference kits, audio manifest, environment specifications, balancing and content data.  
**Tripo / art tools:** riggable concepts and geometry; production readiness depends on valid pivots, mesh separation, actual rigging/animation tests.

**Handoff protocol:** a task has ID, upstream approved spec revision, named owner, deliverable path, prohibited edit areas, dependency list, validation evidence and acceptance criteria. Use branches/PRs for code; docs and Issue comments for review. No agent should overwrite another agent's unreviewed changes. Central canonical contract lives in this repo once approved.

## 16. Production phases and October checkpoint
Phases are a sequence, **not feature deletions**:
1. Contract review and implementation acceptance.
2. Multiplayer roles, input, match start/finish and reliable sessions.
3. Selective harvest, machine states and player inventory.
4. Human power/uplink/extraction objectives; sabotage and counters.
5. Creature locomotion, rigs, interactions, modules and chassis choices.
6. Facility kit assembly, lighting/audio, UI and remote operator role.
7. Additional recipes, missions, cameras, integrated voice, account progression, sophisticated traversal and multiplayer modes.
8. Network QA, packaging, continuous playtesting and polish.

**Oct 31:** report what is completed, functional but incomplete, in asset production, specified, untested, or blocked. Do not shrink the intended game to satisfy the date.

## 17. Key open design decisions / review gates
- Definitive host/backend and UE version based on actual connected project.
- Small hexapod gait details, ceiling traversal constraints, and rig export.
- Quantities and consequences of backup chassis; physical discovery of reserves.
- Module slot limits, resource yield tables, sabotage costs/cooldowns and counters.
- Human objective dependency fail-safe and extraction timing.
- Camera fairness policy (FPS/TPS/isometric) and spectator information leakage.
- Precise human gear, injury model, camera operator permissions.
- How randomized missions and fair persistent unlocks work.
- Voice architecture and whether creature listening is proximity-only or mediated by systems.
- Asset license, performance, target hardware, and financial budgets.

## 18. Acceptance criteria (whole-game)
A successful integrated slice demonstrates: four remote players connect; one entity/three operatives spawn; both teams have meaningful decisions; an individual mechanical component is selected, removed, and reflected in machine state; one sabotage changes human options; humans can repair power, transmit and extract; backup chassis work according to rules; combat and deaths resolve; spectators have appropriately restricted information and usable remote role; match awards correct victory and rematch; camera and audio behaviors remain coherent across clients.

**This draft is a comprehensive design target, not a claim that any of the systems have already been implemented or tested.**
