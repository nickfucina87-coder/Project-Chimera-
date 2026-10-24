# Shared infrastructure and creature builds
Date: 2026-10-24. Approved direction; numerical tuning remains provisional. Supplements Master GDD v0.7. No runtime implementation is claimed.

## Central conflict
Humans rebuild a facility to escape. The entity decides which recovery to exploit and which equipment to consume. Working machines, loose components, and network services are distinct resources. Opening a sector creates specific readable opportunities, never an unexplained global enemy buff.

A rack can be kept running for human objectives, tapped by the entity for connected services, or dismantled for hardware. Humans taking one part leave other authored parts behind. The entity also has independent sources; it is not limited to human scraps.

## Physical interaction contract
Inspect -> open housing -> select slot -> extract (timed, interruptible, audible) -> carry/store -> install in compatible machine or upgrade socket -> update dependent services.
Housings persist open; removed slots remain empty. Opening labor benefits subsequent users. Intact machines take longer to access than exposed ones. Prototype access uses simple cover animation, not screw physics.
Track housing access separately from powered/damaged/functional state and component occupancy. Loss of a required part recalculates the machine's services. Replacement restores only supported services. Trace effects through explicit power/network/access dependencies.
Each component instance has one unique identity and one owner/location: machine slot, world pickup, inventory, or installed upgrade. Transfers are atomic server-authorized transactions; never duplicate or award a part twice. Cancellation before completion transfers nothing.
Upgrade installation reserves actual parts. Prototype modules retain those parts and can release them on removal/reset; no refund-and-keep duplication. Full-game destruction, salvage retention, fitting duration, capacity and module slots require tuning.
Players see target, compatibility, action duration and service consequences. Atmospheric sounds identify real events. No permanent invisible objective softlocks.

## Acquisition routes (all retained)
1. Scavenge opened equipment: quick access, remaining selection.
2. Dismantle intact equipment: slow/noisy, chosen parts, service loss.
3. Tap live equipment: keep connected benefits, discoverable removable tap.
4. Intercept a human delivery: recover the same physical component.
5. Enter newly powered/unlocked sectors: military robotics, chip lab, storage.
6. Recover installed hardware from wrecked chassis subject to salvage rules.
7. Raid spare-parts cages using routes, credentials or appropriate bodies.

## Component catalog
Game abstractions inspired by equipment, not engineering claims. Processing improves interpretation/coordination; actuators supply physical movement; cooling supports sustained load.

| ID | Component | Source | Human purpose | Entity purpose |
|---|---|---|---|---|
| CMP-001 | FPGA accelerator | Compute server | Decode access / stabilize uplink | Sensor interpretation and reacquisition |
| CMP-002 | Memory modules | Compute server | Restore workstation / map | Remember scanned evidence |
| CMP-003 | Network switch | Network cabinet | Reconnect sector | Live surveillance or intrusion hardware |
| CMP-004 | UPS power module | UPS / communications cabinet | Backup objective power | Stored ability energy |
| CMP-005 | Supercapacitor bank | Industrial drive | Restart heavy machinery | Burst power and recharge |
| CMP-006 | Servo controller / encoder / actuator assembly | Robotic machinery | Calibrate or repair mechanism | Precision and movement burst |
| CMP-007 | High-torque actuator | Heavy robotics | Repair lift | Force doors / heavy limbs |
| CMP-008 | Thermal sensor | Inspection equipment | Find overheating faults | Thermal vision |
| CMP-009 | RF receiver | Communications cabinet | Locate distress relay | Detect active transmissions |
| CMP-010 | Piezoelectric sensors | Structural monitoring | Diagnose vibration | Stationary surface vibration sensing |
| CMP-011 | Cooling module | Server / cooling plant | Stabilize equipment | Sustain demanding abilities |
| CMP-012 | Shielding mesh | Secure enclosure | Protect repaired systems | Reduced electronic detection |
| CMP-013 | Secure access module | Restricted terminal | Authenticate sector entry | Limited credential spoofing |
| CMP-014 | Fiber transceiver | Network rack | Bridge communications | High-bandwidth access-point link |

## Build families
Emergent combinations, not mandatory lobby classes. All seven remain full-scope goals.

| Build | Hardware direction | Playstyle | Counter / tradeoff |
|---|---|---|---|
| Wiretap | RF + processing + network tap | Information and destination prediction | Radio silence, disconnected switches, misleading signals |
| Ghost | Precise servos + low-light + shielding | Quiet small-body approach | Lighting and physical observation; fitting costs |
| Bloodhound | Thermal + vibration + memory | Follow and reacquire evidence | Heat clutter, different surfaces, misleading trails |
| Breacher | Heavy actuators + capacitors + armor | Force barriers | Noise, recharge, loss of narrow routes |
| Overclock | Processing + movement + cooling | Rapid repositioning | Heat, cooling noise, downtime |
| Saboteur | Access + electrical + taps | Isolate humans through infrastructure | Manual overrides, remove connections |
| Scavenger | Dismantling tools + cargo + diagnostics | Harvest and rebuild efficiently | Exposure during carrying and assembly |

## Vision / HUD
Creature supports close first-person machine vision and configured third-person view; camera fairness remains testable. Cycle modes, show current mode and dependency status. No default omniscience.

| Mode | Information | Limit |
|---|---|---|
| Optical | Normal scene / labels / motion | Light and line of sight |
| Low-light | Dark passage detail | Bright-light overload |
| Thermal | Humans and hot/recently used equipment | Heat clutter; no default wall vision |
| RF | Active phones/radios/tools/transmitters | Approximate bearing/area; silent devices vanish |
| Network | Connected cameras/doors/servers/routes | Only accessible functioning nodes; no global reveal |
| Active mapping | Authored nearby geometry / moving returns | Revealing scan cue; range and occlusion rules |
| Diagnostic | Parts, slots, faults, compatibility | Close inspection; no human location reveal |

Mapping wave creates a fading reconstruction. Remembered geometry is stale until rescanned. Powering a sector only improves sensors through its actual connected nodes. Harvesting a switch may improve onboard hardware while losing live CCTV access.
Prototype implements optical, basic part highlighting and Signal Sense only; the other modes stay in the catalog.

## Game modes and AI
Preserve 3-operatives-vs-1-entity PvP. Add complete offline human missions against an AI entity and co-op PvE. AI needs explicit stalking, harvesting, upgrade choice, sabotage and objective pressure; this is separate implementation work.
Development role switching and creature practice do not constitute a completed solo game. Both sides must be directly playable for testing. Solo objectives are sequentially solvable without mandatory companion AI.
Preserve chassis choice, backup bodies, weapons, remote operator, progression and full existing scope. No features are cut for October 31.
Time anchors / split timelines remain exploratory, not part of this implementation baseline.

## Readability and balance
Leave running vs harvest must be a real choice. Human restoration grants useful benefits and specific opportunities rather than automatic punishment. Visible missing parts, fan stops, shutter cycles and terminals explain changes.
Base entity remains capable without upgrades. Humans cannot win simply by deleting every source; mission services and destruction costs create tradeoffs.
Provide alternate repair paths or an explicit advertised defeat condition. Prototype guarantees completion parts remain recoverable and includes reset.
