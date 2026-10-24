# CHM-T01: Shared-component test
Date: 2026-10-24. Authorized first implementation target, after local project inspection. Full catalog remains in SHARED_COMPONENT_DESIGN.md. This is not a complete match or a promised same-day deliverable.

## Map
Create L_CHM_ComponentTest in an isolated development map; preserve existing maps and rigs.
Three rooms: Server, Maintenance, Control. Connect by loop corridor, one controllable door and one small creature passage.
Server room contains compute cabinet. Maintenance contains communications cabinet and motor assembly. Control contains recipient uplink and exit trigger.
Use existing usable assets or labeled primitives before purchasing.

## Donors and parts
- Compute cabinet: CMP-001 accelerator + CMP-011 cooling module.
- Communications cabinet: CMP-009 RF receiver + CMP-004 power module.
- Motor assembly: CMP-006 servo assembly + CMP-004 power module.
Exactly 3 donor machines, 6 physical parts, 5 component types. Uplink is a recipient, not a fourth harvesting system.
Removing the accelerator disables compute service. Removing cooling stops fan and marks cooling fault (no lethal heat simulation yet). Communications power loss disables its receiver service; removing RF loses reception. Motor missing servo or power stops movement.
All removals are visible. Both roles can inspect and extract. Human can carry/install; entity can fit upgrades.

## Human test objective
Install the accelerator and one compatible power module in uplink, transmit, reach exit. Two power modules allow human completion plus Servo Boost simultaneously.
In this sandbox, mission hardware cannot be permanently destroyed; dropped/installed hardware is recoverable and out-of-bounds parts return to a visible recovery tray. Future contest/defeat rules are separate.
Cooling is removable and reinstallable for machine-state feedback but is not an extra objective requirement.

## Exactly two entity upgrades
- Signal Sense: fit one RF receiver. Toggle RF mode to see approximate bearing to a deliberately transmitting test beacon or human radio. Silence removes the signal. Prototype range 15m, update interval 1 second, bearing-only with no exact through-wall outline; these are tunable defaults.
- Servo Boost: fit servo assembly plus one power module. Prototype +25% movement speed for 2 seconds with 8-second cooldown measured after burst. Clear feedback and visible attachment. Tunable defaults, not final balance.
Both can coexist; no additional recipes in T01. Reset restores all starting components, inventories, timers and machine state.

## Roles and networking
One-player development menu chooses Human or Entity; debug swap preserves separate role inventories and prevents duplicate pawns/items. Test each independently using beacon.
Two-player editor session assigns one role each. Server owns transfer completion, upgrades, boost timing, radio state, machine slots and uplink completion. Clients request actions. Never trust client-reported inventory/range.
Template locomotion is not proof of replicated custom interaction. Full 3v1 lobby, online backend and AI are later integration stages.

## Suggested implementation order
1. Inspect project/version/source/dirty state and existing MCP capabilities; create checkpoint or isolated copy.
2. Inventory assets and verify rig before modifying it.
3. Build map and shell/cover/slot placeholders.
4. Implement one complete open/extract/drop/install interaction.
5. Apply shared interaction to all donors and uplink.
6. Add entity fitting and the two upgrades.
7. Verify both roles, then two-player authority.
8. Replace proxies selectively and record evidence.

## Acceptance
- Player can navigate all three rooms as either role.
- Target selected slot, open housing and interrupt extraction without gaining part.
- Two players contest one part; exactly one acquires it.
- Open cover and empty slot agree across clients; correct service stops/restores.
- Drop/recover/install preserves one component instance.
- Signal appears only for active in-range transmitter; no cached permanent location reveal.
- Servo Boost returns to base speed and cannot stack via repeated input/role swap.
- Human and creature can both complete their test actions in one session.
- Reset and repeat twice without duplicated parts or stale state.
- Record compile/import status, engine version, screenshots and results; label editor-only evidence honestly. Internet multiplayer remains unverified until tested remotely.

## Offline follow-on
AI entity must use the same component and upgrade rules. Implement a small readable decision loop before expanding builds; no separate cheat inventory. Solo completion is a later acceptance gate, not fulfilled by role swapping.
