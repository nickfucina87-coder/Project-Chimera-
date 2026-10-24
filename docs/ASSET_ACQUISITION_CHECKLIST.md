# Asset acquisition and local inventory
2026-10-24. No acquisition or local import is verified by this document. Existing manifest labels such as Sourced/Licensed describe historical plans, not evidence. ASSET_MANIFEST.csv preserves those labels under legacy_status; verification_status is authoritative.

## First test kit
| Group | Required content | Acceptance |
|---|---|---|
| Architecture | Cohesive wall/floor/ceiling kit, door/frame, corridor and small gap | Correct human/hexapod clearance, simple collision, navigable loop |
| Machine shells | Compute cabinet, communications cabinet, motor assembly, uplink | Separate housing/cover and component slots |
| Components | Accelerator, cooling module, RF board, servo, reusable power module | Separate meshes, usable pivots, collision and visible slot fit |
| Characters | Human placeholder and existing hexapod if ready | Verify scale, skeleton, animation assets, import and camera; do not overwrite original rig |
| Attachments | RF sensor and servo module | Stable sockets, visible installed state |
| Interaction | Cover opening, extraction, carrying, fitting | Simple animation initially, cancellable and network-correct |
| UI | Role selection, target/slot labels, inventory, objective, RF bearing, boost cooldown | Distinguish missing/incompatible/unpowered states |
| Effects | Target highlight, service LEDs, modest extraction sparks | Driven by actual machine state |
| Audio | Cover open, extraction, install, machine fan/stop, boost, RF mode | Positional event feedback; licensed sources |
| Debug | Beacon, recovery tray, reset control, state display | Clear test-only labeling |

## Full equipment catalog
Servers and DIMMs/accelerators; network switches and fiber modules; UPS and capacitor cabinets; robotic arms/controllers/encoders/heavy actuators; thermal inspection equipment; RF communications racks; structural vibration monitors; cooling pumps/exchangers; shielded secure terminals; military robotics and parts cages.
Every donor needs shell, access panel, removable parts, empty-slot visuals, powered/offline/fault feedback, compatible replacement positions and collision.
Retain all original environment, weapon, character, animation, audio, UI and network requirements. Extend entity modules for all seven build families. Full vision needs thermal-tagged materials, scan/diagnostic overlays, RF indicators, stale-map display and authorized CCTV UI.

## Catalog fields and evidence
For each actual candidate record: stable requirement ID, pack/product title, provider URL, creator, license evidence/reference, acquisition status, local source path, Unreal asset path, supported engine version, format, scale, pivot, separable parts, collision, skeleton/animation compatibility, import status, estimated modification work and verification evidence.
Status progression: Planned -> Candidate -> Owned -> Downloaded -> Imported -> Verified. A listing or purchase is not import verification. Private local paths/account evidence stay local; publish only safe project-relative paths and non-sensitive license references.
Inventory existing Content and downloaded sources before acquisition. Reuse owned assets where suitable. Do not bulk import unrelated packs.
Decorative monolithic meshes can serve as shells with custom removable proxies. Do not assume purchased art includes functional harvesting or replication.
User budget: primarily free, at most one or two paid assets each under $20; identify a concrete gap and obtain purchase authorization before spending. No paid acquisition has been requested in this revision.
Do not commit proprietary third-party asset binaries into this public repository; use acquisition references and project source appropriate to redistribution terms.

## First local inspection
Record .uproject location/version, project source/Blueprint structure, enabled MCP plugin and transport, source-control/unsaved state, Content inventory and existing rig/animations. Verify readonly access first. Work on a test map/checkpoint. Coordinate shared binary assets with Claude; do not edit the same map/Blueprint concurrently.
