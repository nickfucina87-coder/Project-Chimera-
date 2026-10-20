# Project Chimera — Asset Manifest & Blockout Contract
**Version:** 0.1 (review draft, 2026-10-20)
**Canonical dataset:** [ASSET_MANIFEST.csv](ASSET_MANIFEST.csv) — 66 tracked assets, systems, and blockout requirements. No files were acquired or imported; no implementation has begun.

## What counts as an asset?
The manifest intentionally includes geometry, rigs, animations, UI, sound, FX, *and* blockout / interactive / networking dependencies. These aren't interchangeable: a mesh may exist while its animation, replication and gameplay integration do not. Status values are **planned definitions**, not completion claims.

## Blockout must come first, but art can work in parallel
Claude can build a graybox from primitive geometry only after the team approves layout and gameplay interfaces. It is **not** necessary to wait for finished models. Early blockout should validate:
- 3v1 spawn safety and the objective route from entry → power → comms → extraction
- Human FPS/TPS sightlines, chokepoints, door clearances, service shortcuts and sound propagation
- Six-legged creature traversal proxy (proposed bounding envelope 90×75×55 cm), ramps/gaps, overhead routes
- Placement of individually targeted harvestable server panels and power nodes
- Distance from creature harvest areas to human objectives, alternate paths and fair counterplay
- Lighting pockets for sightline/occlusion checks and camera tests

Use named placeholders and replace them later with mesh actors preserving gameplay anchors, collision requirements and interaction sockets. Do not attach vital gameplay logic directly to a placeholder StaticMesh asset.

## Recommended blockout work packet for Claude — specification only pending approval
**Input:** docs/MASTER_GAME_DESIGN.md and this manifest.  
**Output:** a top-down playable-zone plan with measurable corridor/door/gap dimensions, a room/route adjacency table, designated extraction/start and backup-chassis locations, human and hexapod traversal graphs, lights, cameras and power dependency map, plus Unreal actor naming guidelines. Do not create gameplay code or alter a working Unreal project until explicitly approved.
**Design envelope:** ~80m x 60m is provisional, not mandated. Four sectors: Server Hall, Robotics Workshop, Power/Cooling, Communications/Loading. Multiple traversal elevations, loops not just a linear corridor, actual hiding positions and viable counters for sabotage.

## Asset acceptance fields
For each acquired or generated item we will eventually add: exact source URL or generation provenance; usage license (including redistribution); approximate cost; artist/tool/owner; actual version/file; dimensions and triangle/LOD/memory expectations; pivot and rig; collision; animation preview; gameplay sockets; import test screenshot or log; approval status; git-lfs or external-storage location. None of those have been verified in this first inventory draft.

## Priority order is dependency order, not scope reduction
1. Level/layout & gameplay dimensions; technical import naming conventions.
2. Rig prototype for the six-legged creature; one interactive rack and detachable module proof.
3. Objective/interactable actors and replicated state; human/creature controls.
4. Remaining environment, human animation, audio/lighting, menu and UX; playable match.
5. Further chassis, recipes, voice, missions, progression and polish.

All features remain in the roadmap. **October 31 is a progress checkpoint.**

## Initial visual direction: provisional
Photorealistic industrial/military research complex with damaged data center, robotics warehouse and service zones. Naturalistic wear, grime and physical machine details, dramatic practical lighting and true hiding spaces. We need user feedback on the extent of structural collapse, architecture age, color cues, and the facility's vertical proportion.

## Open questions for layout and production
1. Aboveground warehouse or primarily buried bunker? What exterior arrival space?
2. How destroyed: 10%, 40%, 80%? Local outages versus full wreck?
3. Horizontal maze vs tall open industrial void with suspended walkways?
4. Hexapod smallest passable gap, climbing freedom and camera view?
5. Human main objective sequencing: 2 breakers then uplink, or dynamic alternate repair paths?
6. Expected visual range / reference games / target hardware.
7. Preferred build source: existing Unreal project or separate Chimera project?

## Responsibility split
- Creative director: aesthetic, fit and fun approval.
- ChatGPT: manifest/specification maintenance and risk/dependency reviews.
- Claude: graybox plan, Unreal gameplay/integration after approval.
- Codex: UI/UX and independent validation contracts.
- Grok/Grokbots: licensed environment, motion/audio/VFX sourcing with URLs and compatibility.
- Tripo/art/rigging: meshes, separated parts, pivots and real animation viability.

**Review status:** proposed, not approved; no assets purchased, generated or implemented via this manifest.
