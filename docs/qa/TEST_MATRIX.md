# CHM-004 — Multiplayer and integration test matrix

Version 0.2 · 2026-10-20 · Codex · **Planning draft — no tests executed**

[Issue #4](https://github.com/nickfucina87-coder/Project-Chimera-/issues/4) · Branch: `codex/chm-004-test-matrix`

## Scope and source baseline
This plan preserves every agreed feature. October 31 is a progress checkpoint, never a reason to remove scope. This change creates only this document; no Unreal implementation, assets, working PC project, PR #10 content or merges are changed.

Read sources at main commit `19ca87f2427dddf0a97755e428a1059c7112b68b`:
- [Master GDD v0.6](../MASTER_GAME_DESIGN.md), including proposed outcomes and open decisions.
- [Asset manifest](../ASSET_MANIFEST.csv), [manifest guide](../ASSET_MANIFEST_GUIDE.md), [agent task board](../AGENT_TASK_BOARD.md), and Issue #4.
- [PR #10](https://github.com/nickfucina87-coder/Project-Chimera-/pull/10) UI drafts pinned to `b58d05badd535d045b320748155513137ebfbaee`: [screen spec](https://github.com/nickfucina87-coder/Project-Chimera-/blob/b58d05badd535d045b320748155513137ebfbaee/docs/ui/UI_SCREEN_SPEC.md) and [data contract](https://github.com/nickfucina87-coder/Project-Chimera-/blob/b58d05badd535d045b320748155513137ebfbaee/docs/ui/UI_DATA_CONTRACT.md).

PR #10 remains a draft dependency, not approved runtime behavior. Its S/I/U identifiers are document references, not implemented classes/events. This plan introduces no replication field names, RPCs, crafting values, life counts or outcome tie-break rules.

## Execution status and meaning
**All 52 cases below are Not run.** No pass, failure, screenshot, runtime timing, import, compile or remote-connectivity evidence exists from this work. Baseline main contains planning files rather than an Unreal project/test build. Rows requiring unapproved rules are additionally blocked from acceptance by their listed dependencies. A proposed expected result is an acceptance hypothesis, not proof that the system exists.

Record each execution as Not run, Blocked, Pass or Fail. Blocked requires the missing rule/build/fixture reference; Pass requires all server/client expectations and evidence; partial execution is not Pass. Never substitute a local editor session for actual internet validation.

Severity describes the impact if the tested invariant fails:
- **S0 Critical:** authority/permission bypass, duplicated persistent/resource state, incorrect victory, session unusable, irreversible match softlock, crash or loss of all control.
- **S1 Major:** substantial gameplay, integration, recovery or usability failure with narrower impact or workaround.
- **S2 Minor:** cosmetic/readability defect without tactical or functional effect; classify observed defects individually even when a case defaults to S0/S1.
These are proposed triage categories, not an approved release policy. Run lifecycle/authority/outcome/permission smoke first, then the complete matrix; execution order does not remove lower-priority scope.

## Machines, roles and fixtures
P1–P4 are four human testers on **four separate player machines**, using the same packaged build/content and actual internet connectivity. Three control operatives and one controls the synthetic creature. Include at least two independent networks for remote acceptance and record actual topology. A listen host is also one of the four players; rotate every player through host and entity roles. Do not count a dedicated server as a player: test that topology only if approved, adding a fifth server machine/process plus all four player machines. An optional fifth player probe tests capacity rejection, not a changed population target.

Each case inherits all four players unless explicitly expanded. During operator cases, an eliminated operative remains one of those four players. Record which player is authority/host, entity, operative, dead operator, shooter, target and observer for each run. Compare host-local, remote owner and remote non-owner behavior; host-only success is insufficient.

Before execution obtain:
1. Approved build/UE/backend configuration, compatible accounts, version identifiers and supported session topology.
2. Approved non-production test fixture with known health/ammo, recipe discovery/costs, reserve count/penalties, circuits, camera permissions, mission seed and outcome rules. Values come from versioned upstream contracts, never this document.
3. A four-sector route/circuit/occlusion fixture with reachable power/uplink/extraction, alternate repairs, individual PSU/logic/fan targets, shutters, working/failed cameras, service/overhead paths and both chassis.
4. Authorized instrumentation/fault controls supplied by runtime owners. This document does not authorize building hooks or modifying gameplay. Use approved inputs/hooks for invalid-intent tests, never real public sessions.
5. Synchronized or measured-offset capture clocks and matching server/client logs. UI screenshots alone cannot prove authority, atomicity or data filtering.
Reset fixture state before independent outcomes; retain exact seed/configuration. For boundary races, repeat with both orderings and simultaneous arrival, recording server order.

## Network profiles and variation
The following are **proposed lab inputs**, not shipping support promises or pass thresholds. Runtime/network owners must approve exact tool semantics, limits and budgets before execution. Measure actual RTT/loss/jitter in addition to requested settings.

| Profile | Proposed impairment | Required variation |
|---|---|---|
| N0 | No injected impairment; real internet baseline | Record natural RTT/jitter/loss. |
| N1 | Add 100 ms RTT | Confirm whether tool accepts one-way delay; avoid accidentally doubling requested RTT. |
| N2 | Add 200 ms RTT, up to ±50 ms delay variation, 2% independent packet loss | Apply to one remote human, remote creature, then authority link in separate runs. Record model/seed. |
| N3 | N2 plus a 5-second full traffic outage, then restore | Exercise before request, after authority commit/before reply, and during phase/role change. |
| N4 | Controlled bandwidth constraint, server stall, message delay/reorder/duplicate via approved hooks | Exact rates/stall durations/message boundaries TBD with CHM-001; transport may already order/deduplicate. Do not assume raw packet duplication reproduces application-event duplication. |

Run every case on N0. NET-01 names the minimum N1–N3 critical reruns; NET-03/05 use N4. Repeat the critical set with host as operative and entity, then rotate host/player assignments to cover all four machines. Unsupported topologies/input/camera modes require a documented blocked result, not silent omission. Proposed minimum is three independent runs per critical profile/role combination, with both race orderings where relevant; approve broader coverage from observed variability.

For timing tests, record request, authority decision, state receipt and visual/audio onset where instrumented. Measure reconciliation delay, timer skew, audio/animation offset and frame/network performance. Numeric acceptance budgets, timeout limits and target hardware are **TBD (D8/D9)**; quantitative acceptance stays blocked until approved. Zero duplicate grants, unauthorized actions and contradictory final outcomes remain qualitative invariants.

## Evidence bundles (required by each case)
Every row cites evidence codes; attach artifacts to its run record, never invent a path to an artifact that was not captured.

| Code | Required evidence |
|---|---|
| E0 | Run metadata: case ID, date/tester, build/content/contract revisions, seed/fixture, machine OS/hardware, topology/roles, graphics/input settings, network profile/tool/actual measurements, status and dependency disposition. |
| E1 | Correlated authoritative and all relevant client logs/state snapshots before/during/after action. Include accepted/rejected intent, player/match/body/target identity and ordering if instrumentation supports it; instrumentation schema awaits CHM-001. |
| E2 | Synchronized screenshot/video/audio from acting owner and at least one observing remote client; include all four views for cross-system/outcome disagreement and timing overlay/offset measurement. |
| E3 | Approved permission/data-delivery audit capture for each audience plus negative request rejection evidence. Do not put account credentials or hidden tactical payloads into public issue attachments; store restricted audit artifacts with access reference. |
| E4 | Impairment configuration, tool version/seed, observed network trace/metrics, outage start/end and recovery timeline. |
| E5 | Approved asset provenance/license reference, exact version, import/compile logs, scale/pivot/collision/socket/rig evidence and placeholder-to-final comparison. |

When authority logs/instrumentation are missing, mark the authority assertion Blocked even if presentation looks correct. Missing screenshots/logs are evidence gaps. File defects with reproducible steps, expected/actual authority and client state, severity, build/profile/seed, artifact links and affected case IDs.

## Test cases
Each numbered sequence is performed in order after fixture setup. Server expectations concern authoritative decisions; client expectations concern owner and permitted observers, not universal disclosure. Dependencies below are cumulative with the global need for a runnable approved build. All case statuses remain **Not run**.

### Session and lifecycle

Traceability: GDD §§3–5,12–13; NET-001/002; UI S01–05/S12, I01/02/09.

#### SES-01 — Four-player internet host/join

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D2.
- **Machines/roles:** P1–P4; rotate host through all four players; global four-machine setup applies.
- **Steps:** 1. Host an approved session. 2. Other three discover/join from separate internet-connected machines. 3. Compare roster, identities and session state.
- **Expected server:** Exactly four unique participants accepted; one authority and coherent lifecycle; no duplicate identities.
- **Expected clients:** All see the same accepted roster; pending differs from accepted; supported error/retry paths work.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### SES-02 — Role contention and 3v1

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D2.
- **Machines/roles:** P1–P4; two request entity concurrently; global four-machine setup applies.
- **Steps:** 1. Submit simultaneous entity requests. 2. Assign remaining players operative. 3. Toggle ready and attempt unauthorized/premature start.
- **Expected server:** Enforce one player-controlled entity and three operatives; start only under approved readiness/permission policy.
- **Expected clients:** Rejected role request preserves accepted role; no enemy loadout leak; all spawn with correct role HUD.
- **Evidence:** E0,E1,E2,E3; capture bundle contents defined above, not just an unchecked artifact label.

#### SES-03 — Join/cancel/full session

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D2.
- **Machines/roles:** P1–P4; optional fifth probe machine; global four-machine setup applies.
- **Steps:** 1. Cancel a pending join and join another supported session. 2. Deliver original response late. 3. Probe full session, stale listing, incompatible build/auth failure using supported controls.
- **Expected server:** No extra player admitted to initial 4-player configuration; canceled/stale attempt cannot corrupt active identity.
- **Expected clients:** No stale redirect or endless spinner; reason shown only when provided; retry does not duplicate join.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### SES-04 — Loading and lobby departure

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D2.
- **Machines/roles:** P1–P4; delay P3 load; global four-machine setup applies.
- **Steps:** 1. Ready group. 2. Delay one loader; leave/rejoin during countdown where supported. 3. Complete loading or trigger approved timeout.
- **Expected server:** Only authoritative phase advances; readiness/loading and departure policy applied consistently.
- **Expected clients:** Local loading completion does not launch other clients; briefing and failure states remain accurate.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### SES-05 — Rematch and role swap

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D2,D3.
- **Machines/roles:** P1–P4; rotate entity and host roles; global four-machine setup applies.
- **Steps:** 1. Complete round. 2. Request rematch concurrently/repeatedly. 3. Swap roles and start next round. 4. Run three consecutive rounds.
- **Expected server:** One next-match transition; approved reset/persistence rules applied; old requests cannot mutate new match.
- **Expected clients:** Clear old body, inventory, targets, CCTV, result subscriptions; correct new HUD; no sensitive prior-role data.
- **Evidence:** E0,E1,E2,E3; capture bundle contents defined above, not just an unchecked artifact label.

#### SES-06 — Host exit and unsupported operations

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D2,D5.
- **Machines/roles:** P1–P4; host as human then entity; global four-machine setup applies.
- **Steps:** 1. Host leaves normally and abruptly in lobby, harvest, uplink and results. 2. Attempt late join and reconnect.
- **Expected server:** Approved termination/migration/reconnect policy only; no assumed host migration or free win.
- **Expected clients:** Clear session-loss path; if feature unsupported show explicit result; if supported restore authoritative state without duplicate Pawn.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

### Combat, salvage and crafting

Traceability: GDD §§4,8–10,13; NET-003/005; INT-001–003/008; WPN-001–005; UI S06–09, I03/04/06.

#### AUT-01 — Authoritative gun damage

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D5.
- **Machines/roles:** P1–P4; P1 shooter, entity target, P2 observer; global four-machine setup applies.
- **Steps:** 1. Fire pistol/rifle with known approved damage/ammo fixture. 2. Reload/empty ammo. 3. Repeat near cover and with delayed fire input.
- **Expected server:** Validate shooter, ammo, damage, target and approved hit/lag-compensation rules; one accepted hit gives one damage application.
- **Expected clients:** Shooter prediction reconciles; owner health/ammo and permitted hit/death effects agree; animations cannot create damage.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### AUT-02 — Invalid combat intents

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D5.
- **Machines/roles:** P1–P4 plus approved isolated test harness; global four-machine setup applies.
- **Steps:** 1. Through approved hooks submit stale-body, other-owner, impossible-rate/ammo and invalid-target damage requests. 2. Repeat after death/end phase.
- **Expected server:** Reject invalid ownership/state/rate under approved rules; client cannot set health or force death.
- **Expected clients:** No unauthorized health/ammo changes or false confirmed kill; rejection does not desync valid play.
- **Evidence:** E0,E1,E2,E3; capture bundle contents defined above, not just an unchecked artifact label.


#### AUT-03 — Creature, trap and electrical hazard damage

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D5,D6.
- **Machines/roles:** P1–P4 on four separate machines; creature attacker, operative target, operative observer, and operative repairer. Repeat with host as attacker and target.
- **Steps:** 1. In separate reset fixtures apply each approved hexapod/biped attack, placed trap and energized environmental hazard to an operative. 2. Move target into/out of permitted range or hazard volume, disable/repair source, and repeat at the damage boundary. 3. Submit duplicate/stale activation through approved hooks; test disallowed team/owner targets under the approved friendly-fire policy. 4. Repeat lethal hit during extraction and death-to-operator transition on N0 and N2.
- **Expected server:** Validate damage source, ownership, range/contact, active hazard/trap state and approved target/friendly-fire rules. Apply only approved damage/tick counts and one death transition; no damage from an expired/disarmed/unpowered source or a stale body. Resolve extraction race by D5, without inventing a priority.
- **Expected clients:** Target health, permitted hit feedback, trap/hazard visual/audio state and operator transition reconcile to authority. Disabling source does not leave ghost damage or a client-only safe zone. Owner and remote observers see only permitted information.
- **Evidence:** E0,E1,E2,E4 for impaired runs; capture accepted/rejected source events, before/after health and source state, synchronized target/attacker/observer views and source-off boundary timing.

#### HAR-01 — Individual component selection

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D6.
- **Machines/roles:** P1–P4; entity at rack; humans nearby/far; global four-machine setup applies.
- **Steps:** 1. Aim separately at PSU, logic board and fan. 2. Harvest one. 3. Inspect adjacent components and machine state.
- **Expected server:** Resolve exact component; detach once; award approved yield once; recalculate only defined dependencies.
- **Expected clients:** Correct slot label/outline/progress; adjacent parts retained; detached visual, own inventory, power/audio agree.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### HAR-02 — Harvest interruption boundaries

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4.
- **Machines/roles:** P1–P4; entity harvester; human disruptor; global four-machine setup applies.
- **Steps:** 1. Begin harvest. 2. Cancel, leave range, lose target, take damage or die at start/mid/end. 3. Reacquire target.
- **Expected server:** Apply approved interruption and duration rules; no premature/duplicate yield; component and transaction converge.
- **Expected clients:** Progress clears/reconciles with reason; no completion implied by local animation; pending action does not silently cancel just by closing menu.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### HAR-03 — Stale/concurrent harvest mutation

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D5.
- **Machines/roles:** P1–P4 plus approved hooks; global four-machine setup applies.
- **Steps:** 1. Duplicate begin/complete/cancel. 2. Remove/destroy component via another approved interaction at completion boundary. 3. Replay old target after rematch.
- **Expected server:** At most one valid detach/reward; serialize conflicting operations; reject stale match/target identity.
- **Expected clients:** No duplicated mesh/resource or wrong-slot removal; response order cannot restore removed component.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### CRA-01 — Craft, equip and use modules

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D6.
- **Machines/roles:** P1–P4; entity crafter; humans observe tells; global four-machine setup applies.
- **Steps:** 1. Gather exact approved requirements. 2. Discover/compose/equip/use each approved recipe. 3. Exercise counters and visible configuration.
- **Expected server:** Validate discovery, resources, slots, target, cooldown; debit/grant exactly once using approved deterministic/seeded recipe rules.
- **Expected clients:** Owner inventory/module state reconciles; humans see only allowed tells/effects; counters work; no hidden recipe disclosure.
- **Evidence:** E0,E1,E2,E3; capture bundle contents defined above, not just an unchecked artifact label.

#### CRA-02 — Insufficient/stale resources and duplicates

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D5.
- **Machines/roles:** P1–P4 plus approved hooks; global four-machine setup applies.
- **Steps:** 1. Submit two crafts against same inventory. 2. Use insufficient resources, unknown recipe or invalid slot. 3. Delay success until panel closes/body changes.
- **Expected server:** Reject invalid requests; no negative balance, duplicate grant, or stale-body mutation; approved atomic transaction behavior.
- **Expected clients:** Confirmed inventory remains correct; unknown versus zero distinct; pending status resolves without optimistic reward.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### CRA-03 — Module counterplay and physical sabotage scope

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D4,D6.
- **Machines/roles:** P1–P4; entity deploys; three humans counter; global four-machine setup applies.
- **Steps:** 1. Deploy each approved trap/decoy/parasite. 2. Try out-of-range/unvisited machinery. 3. Apply approved counter and repeat after expiry.
- **Expected server:** Enforce physical-visit/ability scope, costs and counters; no unrestricted global hacking unless explicitly approved as an ability.
- **Expected clients:** Readable subtle tells; counter/expiry state consistent; no false general status confused with approved deception.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

### Chassis, evolution and traversal

Traceability: GDD §§6–7; CHR-002–005; ANM-002–005; BLK-002–004; NET-005; UI S07–08, I05.

#### CHA-01 — Small chassis remains viable

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D4,D6.
- **Machines/roles:** P1–P4; entity remains hexapod full round; global four-machine setup applies.
- **Steps:** 1. Complete harvest/craft/sabotage loop without evolving. 2. Traverse service and approved overhead paths. 3. Contest objectives.
- **Expected server:** No mandatory evolution gate; valid actions/routes follow approved chassis eligibility.
- **Expected clients:** Hexapod HUD, collision, view, modules and animation remain usable throughout match; balance observations recorded separately.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### CHA-02 — Optional larger body reconstruction

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D6.
- **Machines/roles:** P1–P4; entity at approved eligible site; global four-machine setup applies.
- **Steps:** 1. Choose approved larger chassis. 2. Observe assembly/transition. 3. Exercise reach/heavy interactions and attempt small-only gaps; reverse change if supported.
- **Expected server:** Validate site/cost/body transition; preserve approved identity/modules/state; one active controlled body; reject invalid passage.
- **Expected clients:** New collision/rig/camera/HUD agree on all clients; former body cannot act; no teleport exploit or forced switch.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### CHA-03 — Reserve body depletion

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D5,D6.
- **Machines/roles:** P1–P4; humans destroy active creature bodies; global four-machine setup applies.
- **Steps:** 1. Destroy body with eligible reserves. 2. Reboot/transfer. 3. Repeat to approved reserve limit then request another transfer.
- **Expected server:** Exactly one approved reserve/cost/penalty applied per destruction; eligibility/spawn rules upheld; exhaustion grants no extra life.
- **Expected clients:** Humans see body loss without unauthorized reserve site/count; entity sees confirmed reserve/penalty; final destruction does not infer human victory.
- **Evidence:** E0,E1,E2,E3; capture bundle contents defined above, not just an unchecked artifact label.

#### CHA-04 — Death/assembly/transfer race

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D5,D6.
- **Machines/roles:** P1–P4 plus approved hooks; global four-machine setup applies.
- **Steps:** 1. Damage or destroy body during assembly/transfer. 2. Duplicate transfer; make site ineligible or occupied. 3. Disconnect at commit boundary.
- **Expected server:** Single legal transition; no two active bodies, resource duplication, reserve bypass or unsafe placement; race precedence per approved rules.
- **Expected clients:** No stranded control/camera or old HUD; fresh snapshot rebinds to accepted body only.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### CHA-05 — Navigation, occlusion and rig clearance

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D6.
- **Machines/roles:** P1–P4; human and both creature bodies; global four-machine setup applies.
- **Steps:** 1. Walk/crouch/sprint through approved doors, aisles, gaps and overhead transitions. 2. Observe from other elevations. 3. Replace placeholder with approved asset fixture.
- **Expected server:** Collision and target anchors preserve approved traversal graph; disallowed body routes remain blocked.
- **Expected clients:** No camera clipping, foot/leg penetration that obscures tells, floating sockets or invisible occluders; both remote and owner silhouettes coherent.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

### Power, machinery and mission outcomes

Traceability: GDD §§5–6,8–9; INT-004/005/007/009; NET-004/006; UI S05–07/S12.

#### OBJ-01 — Complete LAST TRANSMISSION

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D5,D6.
- **Machines/roles:** P1–P4; all roles participate; global four-machine setup applies.
- **Steps:** 1. Stage entry. 2. Restore approved circuit set. 3. Access/activate uplink. 4. Maintain transmission. 5. Qualify extraction with at least one human.
- **Expected server:** Enforce approved dependencies and final adjudication; one final result; server time drives phase/progress.
- **Expected clients:** Authorized objectives, markers, timer, extraction status and result agree; no client-only completion.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### POW-01 — Electrical sabotage and restoration

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D6.
- **Machines/roles:** P1–P4; entity sabotages, humans repair; global four-machine setup applies.
- **Steps:** 1. Visit panel/machine and remove approved power component. 2. Inspect affected lights, shutters, CCTV and uplink. 3. Repair/override through approved path.
- **Expected server:** Recalculate circuit graph and powered/offline/damaged/repairing/overridden/removed-part states; no unrelated global outage.
- **Expected clients:** All relevant clients show coherent physical changes; prompts, audio and lighting track real machine state.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### POW-02 — Alternate recovery and no silent softlock

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D6.
- **Machines/roles:** P1–P4; approved dependency graph fixture; global four-machine setup applies.
- **Steps:** 1. Sabotage each critical dependency singly. 2. Enumerate approved combinations and walk alternate routes. 3. Exercise fallback repair; inspect any deliberate terminal state.
- **Expected server:** Each reachable sabotage state preserves an approved recovery path or explicit advertised loss condition; no accidental unwinnable state.
- **Expected clients:** Humans can locate/use intended fallback without hidden-map leaks; terminal state communicated; graph traversal evidence plus played path required.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### POW-03 — Repair/sabotage contention and shutters

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D6.
- **Machines/roles:** P1–P4; repair and sabotage simultaneous; global four-machine setup applies.
- **Steps:** 1. Repair while entity sabotages same circuit. 2. Operate shutter during power change and overlap with player. 3. Repeat with delayed state delivery.
- **Expected server:** Resolve competing actions once under approved precedence; door collision/hazard/override rules coherent; no invalid resource spend.
- **Expected clients:** Door visual and collision agree; interaction stays on correct component; clients converge without trapping invalid body states.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### OBJ-02 — Interrupt and resume uplink

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D5.
- **Machines/roles:** P1–P4; uplink active; entity interrupts; global four-machine setup applies.
- **Steps:** 1. Interrupt power during transmission at early/mid/final progress. 2. Recover circuit. 3. Reattempt approved resume/reset behavior.
- **Expected server:** Apply approved progress retention/reset and dependency rules; no early extraction unlock or duplicate completion.
- **Expected clients:** All see authoritative interruption/recovery; local progress cannot finish uplink while offline.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### WIN-01 — Human extraction and partial extraction

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D5,D6.
- **Machines/roles:** P1–P4; one human extracts, others alive/dead; global four-machine setup applies.
- **Steps:** 1. Attempt extraction before prerequisites. 2. Meet prerequisites. 3. Extract one, multiple and all humans in separate fixtures.
- **Expected server:** Reject unqualified extraction; apply approved team/individual partial-extraction and zone timing rules; one final result.
- **Expected clients:** S12 winner/reason/extraction matches authority; missing stats shown unavailable, not zero; no fabricated victory.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### WIN-02 — Elimination, timeout and final creature kill

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D5.
- **Machines/roles:** P1–P4; separate outcome fixtures; global four-machine setup applies.
- **Steps:** 1. Eliminate all operatives. 2. Let timer expire without qualified extraction. 3. Destroy final creature body before objectives complete.
- **Expected server:** Resolve approved entity-win/timeout rules; creature destruction alone does not satisfy human objectives; emergency escape/stalemate policy requires approval.
- **Expected clients:** Results use server decision, not local timer or killfeed; dead operators cannot block round conclusion.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### WIN-03 — Outcome boundary races

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D5.
- **Machines/roles:** P1–P4 plus approved clock/event controls; global four-machine setup applies.
- **Steps:** 1. Qualify extraction at timer boundary. 2. Kill last human during extraction. 3. Complete uplink during outage. 4. Deliver final result before delayed prior events.
- **Expected server:** Approved deterministic precedence; exactly one immutable final adjudication; late messages cannot award second win.
- **Expected clients:** All clients settle on same winner/reason; no replayed gameplay mutation after final phase; rematch remains functional.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

### Operator permissions and cameras

Traceability: GDD §§4,9,11,13; INT-006; NET-007/008; UI S06/S10/S11, I07/08.

#### OBS-01 — Death-to-operator transition

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D7.
- **Machines/roles:** P1–P4; P2 dies, P1/P3 alive; global four-machine setup applies.
- **Steps:** 1. Kill P2 while interaction/menu active. 2. Open remote console. 3. Attempt former living-body actions.
- **Expected server:** Confirmed role/observer transition; only permitted cameras/actions; living input authority removed.
- **Expected clients:** Living HUD/targets clear; feed list contains only permitted working cameras; no free spectator view.
- **Evidence:** E0,E1,E2,E3; capture bundle contents defined above, not just an unchecked artifact label.

#### OBS-02 — Power loss and permission revocation

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D7.
- **Machines/roles:** P1–P4; operator watches camera; entity attacks supply; global four-machine setup applies.
- **Steps:** 1. Select feed. 2. Disable/destroy camera or revoke access. 3. Queue selection/action immediately before revocation. 4. Disable all feeds then restore one.
- **Expected server:** Revalidate every camera/action request; stop unauthorized feed/state delivery; restoration does not grant wider permissions.
- **Expected clients:** On received revocation clear image, cached markers and controls; explicit no-feed/offline state; no frozen tactical view.
- **Evidence:** E0,E1,E2,E3; capture bundle contents defined above, not just an unchecked artifact label.

#### OBS-03 — Pings and restricted facility actions

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D7.
- **Machines/roles:** P1–P4 plus approved test hooks; one operator; global four-machine setup applies.
- **Steps:** 1. Use permitted ping/control. 2. Try wrong-role, unpermitted target/action, out-of-bounds and excessive-repeat requests. 3. Check recipients.
- **Expected server:** Enforce action list, power, bounds, cooldown and recipient policy; rejected input cannot mutate machinery.
- **Expected clients:** Only authorized audiences receive ping/info; restriction reason understandable; no global remote hack.
- **Evidence:** E0,E1,E2,E3; capture bundle contents defined above, not just an unchecked artifact label.

#### OBS-04 — Hidden-state delivery audit

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D7.
- **Machines/roles:** P1–P4; operator/human/entity captures separated; global four-machine setup applies.
- **Steps:** 1. Place entity/resources/reserves and unauthorized cameras outside permitted vision. 2. Inspect approved client diagnostics/data delivery. 3. Role-swap/rematch.
- **Expected server:** Filter sensitive data at authority boundary according to contract; not merely hidden widgets; diagnostic data does not leak tactical state.
- **Expected clients:** No unauthorized feed/transforms/recipe/reserve details or stale prior-role caches. Legitimate visible entities still render per approved relevance policy.
- **Evidence:** E0,E1,E2,E3; capture bundle contents defined above, not just an unchecked artifact label.

#### CAM-01 — FPS/TPS preference and policy

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D7.
- **Machines/roles:** P1–P4; both roles, both creature bodies; global four-machine setup applies.
- **Steps:** 1. Switch each supported FPS/TPS mode in play and settings. 2. Request disallowed mode. 3. Change role/body while request pending.
- **Expected server:** Server gameplay authority/view eligibility retained; view change cannot grant new ability/range/target permission.
- **Expected clients:** Effective mode matches policy; control/HUD/aim alignment coherent; rejection explained; no promise of unrestricted modes.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### CAM-02 — Corner, ceiling and gap peeking

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D6,D7.
- **Machines/roles:** P1–P4; approved occlusion fixtures; global four-machine setup applies.
- **Steps:** 1. Place targets behind corners, shutters, ceilings and service gaps. 2. Rotate/zoom/change mode while moving/crouching. 3. Fire/harvest through obstructed sightline.
- **Expected server:** Target acquisition/hits/interactions obey approved visibility and collision policy; camera cannot authorize through-wall actions.
- **Expected clients:** Collision prevents prohibited information advantage; owner/remote body and aim align; document differences for fairness review.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### CAM-03 — Optional isometric and camera failure recovery

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D6,D7.
- **Machines/roles:** P1–P4; approved experimental camera configuration; global four-machine setup applies.
- **Steps:** 1. If supported, enable isometric experiment. 2. Test overhead occlusion/selection. 3. Revoke mode or destroy controlled body during switch.
- **Expected server:** Apply approved experimental restrictions; invalid target/body requests rejected; unsupported experiment stays blocked, retained in scope.
- **Expected clients:** Return to permitted view without crash/stuck control; no expanded unauthorized map knowledge.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

### Network faults and replication recovery

Traceability: GDD §13; NET-001–008; UI recovery rules and I01–I09.

#### NET-01 — Latency/jitter/loss across critical actions

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D5,D8.
- **Machines/roles:** P1–P4; shape one client then host/server link; global four-machine setup applies.
- **Steps:** 1. Apply N0–N3 profiles. 2. Repeat SES-02, AUT-01, HAR-01/03, CRA-02, CHA-04, POW-03, OBS-02 and WIN-03. 3. Remove impairment.
- **Expected server:** Authoritative invariants hold; no duplicate reward/body/winner or permission bypass; measure convergence against approved budgets.
- **Expected clients:** Prediction/progress/pending status reconcile; no permanent stale HUD or missed terminal phase; collect observed timing, do not assume pass by appearance.
- **Evidence:** E0,E1,E2,E4; capture bundle contents defined above, not just an unchecked artifact label.

#### NET-02 — Disconnect at transaction boundary

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D2,D5,D8.
- **Machines/roles:** P1–P4; disconnect entity then human; global four-machine setup applies.
- **Steps:** 1. Break connection before request, after server commit/before reply, during transfer, and during result/rematch. 2. Retry only via approved recovery.
- **Expected server:** Approved retention/timeout/reconnect handling; exactly one commit, no duplicated yield/spend/body or false participant identity.
- **Expected clients:** Stale controls disabled; fresh snapshot on supported reconnect; no blind replay of non-idempotent action; clear unsupported path.
- **Evidence:** E0,E1,E2,E4; capture bundle contents defined above, not just an unchecked artifact label.

#### NET-03 — Replication delay/reorder/duplicate

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D8.
- **Machines/roles:** P1–P4 plus approved fault hooks; global four-machine setup applies.
- **Steps:** 1. Delay component/inventory/power updates independently. 2. Deliver stale/duplicate interaction outcomes and role/phase messages. 3. Restore normal delivery.
- **Expected server:** Approved sequencing/transaction design yields coherent current state; old match/body events cannot apply; do not invent RPC guarantees.
- **Expected clients:** No false success, reverted destroyed component, negative inventory, resurrected living HUD or revoked feed restored by late data.
- **Evidence:** E0,E1,E2,E3,E4; capture bundle contents defined above, not just an unchecked artifact label.

#### NET-04 — Relevance, snapshot and resynchronization

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D2,D6,D8.
- **Machines/roles:** P1–P4; move clients between sectors; global four-machine setup applies.
- **Steps:** 1. Leave relevance range during sabotage/assembly. 2. Return after state settles. 3. Reconnect/join in progress only if approved. 4. Drop a cosmetic event.
- **Expected server:** Current authoritative state reconstructs without relying on having seen historical events; permissions still filter snapshots.
- **Expected clients:** Correct component/machine/body/objective state after return; no permanently looping fan, sparks, wrong rig or stale power after snapshot.
- **Evidence:** E0,E1,E2,E4; capture bundle contents defined above, not just an unchecked artifact label.

#### NET-05 — Server stall, bandwidth pressure and version mismatch

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D2,D5,D8.
- **Machines/roles:** P1–P4; approved isolated server environment; global four-machine setup applies.
- **Steps:** 1. Introduce controlled stall/bandwidth restriction. 2. Resume. 3. Attempt incompatible content/contract version connection.
- **Expected server:** Approved timeout/timebase/version policy prevents inconsistent simulation and mutations; no unbounded duplicated transactions.
- **Expected clients:** Recover or terminate explicitly; timer/phase converge; rejection readable; report CPU/network measurements without invented shipping budgets.
- **Evidence:** E0,E1,E2,E4; capture bundle contents defined above, not just an unchecked artifact label.

#### NET-06 — Long session and fault recovery regression

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D2,D8,D9.
- **Machines/roles:** P1–P4; rotate host/roles each run; global four-machine setup applies.
- **Steps:** 1. Play three full matches/rematches including recovery from N3. 2. Revisit altered machines each round. 3. Inspect retained actors/subscriptions and resource usage.
- **Expected server:** No cross-match state/resource/permission accumulation; approved resets/persistence maintained.
- **Expected clients:** No growing duplicate audio/effects/HUD callbacks, stale bodies/feeds or crash; record memory/frame/network trends for review.
- **Evidence:** E0,E1,E2,E4; capture bundle contents defined above, not just an unchecked artifact label.

### Lighting, audio, animation and machinery integration

Traceability: GDD §§6–9,14; BLK/ENV/INT/CHR/ANM/WPN/VFX/AUD families.

#### INT-01 — Lighting and flashlight state

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D4,D6,D9.
- **Machines/roles:** P1–P4; all sectors; low/high supported scalability; global four-machine setup applies.
- **Steps:** 1. Toggle power normal/emergency/blackout through gameplay. 2. Use flashlight. 3. Compare occluded targets at each graphics setting; recover supply.
- **Expected server:** Replicated gameplay power/flashlight rules coherent; rendering settings cannot change authoritative interactions.
- **Expected clients:** Correct visible light state and truthful cues; no competitive visibility exploit at reduced settings; capture GPU/frame metrics for approved budget review.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### INT-02 — Positional machinery/harvest audio

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D4,D6,D9.
- **Machines/roles:** P1–P4; near/far/behind-wall/overhead listeners; global four-machine setup applies.
- **Steps:** 1. Harvest fan/PSU/board. 2. Cycle shutter, alarm, transformer and power repair. 3. Move through sector boundaries; repeat under N2.
- **Expected server:** Events originate from actual approved interactions; sound-relevant state and lifecycle remain coherent.
- **Expected clients:** Correct emitter/location/occlusion/material; fan loop stops/restarts with machine; no ghost replay or globally revealing audio; measure timing vs acceptance.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### INT-03 — Gunfire/footsteps/false signals

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D4,D6,D9.
- **Machines/roles:** P1–P4; human and entity listeners; global four-machine setup applies.
- **Steps:** 1. Fire/reload, walk/scuttle on approved materials/gaits. 2. Trigger approved false-signal module and counter. 3. Observe concealed/visible actor variants.
- **Expected server:** Real effects track accepted gameplay; deception occurs only via approved ability and counter rules.
- **Expected clients:** Positional tells consistent across clients; false signal remains learnable/counterable; footsteps match material/gait rather than arbitrary horror cues.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### INT-04 — Human animation and weapon integration

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D4,D6,D9.
- **Machines/roles:** P1–P4; shooter/observer/target; global four-machine setup applies.
- **Steps:** 1. Walk/run/crouch/aim/fire/reload/interact/hit/die in FPS/TPS. 2. Interrupt reload/interaction with death and phase change. 3. Repeat under N2.
- **Expected server:** Accepted combat/interaction state governs gameplay; montage timing cannot grant free ammo/damage.
- **Expected clients:** Remote and local poses, muzzle/light/gear sockets, impacts and death align; no stuck animation or duplicated notify effects.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### INT-05 — Hexapod/biped animation and VFX

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D4,D6,D9.
- **Machines/roles:** P1–P4; both bodies; relevant assets available; global four-machine setup applies.
- **Steps:** 1. Scuttle/turn/sprint/traverse/harvest/attack/hit/shutdown. 2. Assemble larger body and reboot backup. 3. Interrupt each at authority boundary.
- **Expected server:** Gameplay transition/harvest/damage remains valid independent of missing/delayed cosmetics.
- **Expected clients:** Six-leg articulation and separate rig/socket/collision alignment demonstrated; detach sparks, assembly and shutdown match accepted events; no cosmetic-only completion.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### INT-06 — Interactive asset replacement and import gate

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D6,D9.
- **Machines/roles:** P1–P4; approved placeholder and final candidate fixtures; global four-machine setup applies.
- **Steps:** 1. Record asset provenance/import version. 2. Compare scale/pivots/collision/sockets/LODs for racks, removable parts, robots, breakers, uplink, shutters, cameras and extraction. 3. Run HAR-01/POW-01/OBJ-01 on both fixtures.
- **Expected server:** Gameplay anchors/dependencies preserved after asset substitution; separate removable components retain identity; import/compile evidence required.
- **Expected clients:** Visual/collision/animation and prompts remain aligned; no monolithic mesh substituted for interactive slots or static model claimed as animated.
- **Evidence:** E0,E1,E2,E5; capture bundle contents defined above, not just an unchecked artifact label.

#### INT-07 — Cross-system end-to-end chain

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D4,D5,D6,D7,D8,D9.
- **Machines/roles:** P1–P4; human/operator/entity roles all exercised; global four-machine setup applies.
- **Steps:** 1. Entity harvests PSU. 2. Verify lights/fan/CCTV/uplink effects. 3. Human repairs while operator uses allowed feed/action. 4. Entity transfers body. 5. Complete transmission/extract/rematch.
- **Expected server:** Single coherent dependency chain, transaction outcomes and final result; every state originates from accepted authority.
- **Expected clients:** All relevant audio/light/mesh/animation/HUD cues agree over time and recover under N2; no information leak or residual effect in rematch.
- **Evidence:** E0,E1,E2,E3,E4; capture bundle contents defined above, not just an unchecked artifact label.

### UI, accessibility and retained roadmap

Traceability: GDD §§3,10–12,16–17; UI-001–008; NET-009/010; PR #10 U01–U12.

#### UX-01 — Screen flow and accessibility

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D3,D7,D9.
- **Machines/roles:** P1–P4; supported inputs and accessibility settings; global four-machine setup applies.
- **Steps:** 1. Navigate S01–S13 through role/phase changes. 2. Enlarge text/UI, use focus navigation/non-color cues and captions where supported. 3. Open settings during ongoing match.
- **Expected server:** Local settings cannot pause shared simulation or authorize gameplay/camera; data scope unchanged.
- **Expected clients:** No clipped essential controls, hidden focus, misleading zero/unknown, blocked return path or stale role panel; unsupported options labeled.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

#### UX-02 — Progression/results persistence parity

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D3,D5,D10.
- **Machines/roles:** P1–P4; approved test accounts/save service; global four-machine setup applies.
- **Steps:** 1. Finish match. 2. Save approved cosmetic/unlock choice. 3. Fail/retry save, reconnect account and rematch; replay result event.
- **Expected server:** Apply approved rewards once to authorized account only; save conflicts resolved; no unapproved competitive advantage.
- **Expected clients:** Preview differs from confirmed equip; failed save preserves confirmed choice; missing stats unavailable; no duplicate rewards.
- **Evidence:** E0,E1,E2,E3; capture bundle contents defined above, not just an unchecked artifact label.

#### RDM-01 — Integrated proximity voice and overhearing

- **Status:** Not run. **Severity:** S0. **Dependencies:** D1,D7,D10.
- **Machines/roles:** P1–P4; approved voice backend and privacy fixture; global four-machine setup applies.
- **Steps:** 1. Test near/far/obstructed speech, equipment/radio and creature overhearing per approved rules. 2. Mute and change living/operator roles. 3. Sabotage/disconnect voice path.
- **Expected server:** Approved audience/privacy/role/equipment scope enforced; external voice is not claimed controllable by in-game settings.
- **Expected clients:** Mute/privacy UI accurate; permitted speech intelligible/positional; no unauthorized dead-player tactical channel or hearing through forbidden systems.
- **Evidence:** E0,E1,E2,E3; capture bundle contents defined above, not just an unchecked artifact label.

#### RDM-02 — Random missions, larger populations and future content

- **Status:** Not run. **Severity:** S1. **Dependencies:** D1,D4,D5,D6,D10.
- **Machines/roles:** P1–P4 initially; expand to five/six player machines only after rule approval; global four-machine setup applies.
- **Steps:** 1. For each future approved mission/seed, monster/module/gear/site configuration repeat lifecycle, dependency and outcome cases. 2. Check alternate paths and role capacity for approved population.
- **Expected server:** Approved seeded objectives/resources, role ratios and parity rules maintained; do not assume initial 3v1 unchanged for larger population.
- **Expected clients:** Correct briefing/objectives/recipes/rigs/HUD/route cues for selected content; scope retained while fixtures await definition.
- **Evidence:** E0,E1,E2; capture bundle contents defined above, not just an unchecked artifact label.

## Dependencies and approval blockers
The baseline tree has no architecture, level, harvest-rule or test implementation deliverables. These requirements are requested from their owners; this plan does not edit or take ownership of those files.

| ID | Owner/dependency | Required decision or evidence / affected acceptance |
|---|---|---|
| D1 | CHM-001 / Claude: architecture and network events | Approved authority/read ownership, message identities, visibility/relevance, validation, atomicity, sequencing, rejection and instrumentation contracts; runnable network build. Needed across all runtime cases. |
| D2 | CHM-001 + creative director: sessions | UE/backend/auth/topology/version policy, readiness/start, capacity, join-in-progress, host loss/migration, cancellation and reconnect behavior. |
| D3 | CHM-003 / PR #10 UI review | Approved S01–S13 data bindings, pending/error/permission/transition behavior and accessibility targets. Pinned draft is input, not acceptance approval. |
| D4 | CHM-008 + creative director, CHM-001 | Damage/hit rules; harvest timing/cancel/yields; recipes/discovery/slots/costs/counters; chassis eligibility, reserve limits/penalties/site rules; circuit dependency recovery and conflict rules. No numeric balance silently decided here. |
| D5 | Creative director + CHM-001 | Extraction timing/eligibility/partial extraction, timeout/tie precedence, final-body destruction, all-human death with operators active, abandonment/stalemate, rematch/reset/persistence and server clock basis. GDD outcomes are proposed. |
| D6 | CHM-002, CHM-007 and runtime integration | Approved room/traversal/circuit graphs, target anchors, dimension/rig/collision fixtures, chassis/site placement and occlusion policy. Provisional dimensions are not test pass thresholds. |
| D7 | Creative director + CHM-001 | CCTV feeds/actions/ping recipients/cooldowns/revocation, FPS/TPS/isometric eligibility, view collision/aim fairness and role information policy. |
| D8 | Network/runtime owners | Approved fault tools and safe fixtures, ordering/timebase/correlation instrumentation, supported latency/loss/recovery envelope and numeric convergence budgets. |
| D9 | CHM-005/006/007/009 + art/audio/runtime owners | Licensed versioned assets, import/compile/retarget proof, supported hardware/scalability/input, performance/animation/audio synchronization budgets and accessibility targets. |
| D10 | Creative director + relevant service/content owners | Progression/save/reward/parity, voice/privacy/overhearing and future mission/population/monster/equipment rules and fixtures. Retained roadmap cases cannot pass until specified/implemented. |

## Coverage cross-check
| Requirement / source | Cases |
|---|---|
| Four players, 3v1, hosting/joining/roles/rematch | SES-01–06; NET-06 |
| Damage, harvesting, crafting authority | AUT-01–03; HAR-01–03; CRA-01–03; NET-01–03 |
| Backup chassis, optional evolution, traversal | CHA-01–05 |
| Electrical sabotage, alternate recovery, machinery | POW-01–03; HAR-01; INT-06–07 |
| Death operator, camera and facility permissions | OBS-01–04; NET-03 |
| FPS/TPS, peeking and optional isometric | CAM-01–03; CHA-05 |
| Power/uplink/extraction/victory | OBJ-01–02; WIN-01–03 |
| Latency, disconnects, replication failures | NET-01–06; SES-06 |
| Lighting/audio/animation/interactive integration | INT-01–07; POW-01; CHA-05 |
| UI PR #10 U01/U02/U03/U04 | SES-02 / OBJ-01 / HAR-01 / HAR-02–03 |
| UI PR #10 U05/U06/U07/U08 | CRA-01–02, CHA-02–04 / OBS-01–03 / NET-02–04 / WIN-01–03 |
| UI PR #10 U09/U10/U11/U12 | UX-01 / SES-05 / OBS-04 / UX-02, CAM-01 |
| UI S01–S13; intents I01–I10 | SES, AUT/HAR/CRA, CHA, OBS, CAM, WIN, UX groups collectively; I10 in UX-02 |
| Manifest BLK/ENV/INT | CHA-05; CAM-02; POW; OBJ; INT-06 |
| Manifest CHR/ANM/WPN/VFX/AUD | AUT; CHA; CRA; INT-01–07 |
| Manifest UI-001–008 and NET-001–010 | SES through UX plus RDM-01; progression NET-010 in UX-02 |
| Proximity voice, fair progression, randomized missions, future content/population | RDM-01–02; UX-02; retained and dependent, not removed |


## First execution runbook and deterministic role schedule

This is preparation only. No tester/machine is allocated and no run has occurred.

| Run | Listen host | Creature | Operatives |
|---|---|---|---|
| A | P1 | P2 | P1, P3, P4 |
| B | P2 | P2 | P1, P3, P4 |
| C | P3 | P4 | P1, P2, P3 |
| D | P4 | P4 | P1, P2, P3 |
| E | P1 | P1 | P2, P3, P4 |
| F | P2 | P3 | P1, P2, P4 |
| G | P3 | P3 | P1, P2, P4 |
| H | P4 | P1 | P2, P3, P4 |

This proposed schedule covers each machine hosting as both operative and creature, and each player controlling the creature. During a run kill one non-host operative for operator checks while retaining all four participants; separately test host-operative death. Repeat designated critical fault cases for each approved combination, with three independent executions as proposed above. Dedicated-server variants require their own approved schedule; do not silently substitute a listen host.

Before scheduling any runtime run, record these readiness checks as Pending until evidence is supplied:

| Check | Evidence needed | If unavailable |
|---|---|---|
| Build and access | Packaged build ID, exact UE/content revision, approved backend/session topology, four machines/accounts on independent networks | Block all runtime cases; do not launch guessed commands or services. |
| Rules and fixtures | Versioned D1–D7 decisions, authoritative expected values, map/seed, body/recipe/circuit/camera fixtures | Block affected assertions; a tester must not choose the game rule. |
| Capture and observability | Authority/client log correlation, capture clock offsets, four-role video/audio capture and permission audit access | Block assertions that cannot be proven; screenshots alone cannot establish authority. |
| Fault and timing setup | Approved shaping/hooks, measured baseline, D8/D9 acceptance budgets and recovery/timeout policy | Run only approved qualitative scenarios; quantitative/fault acceptance remains blocked. |
| Reset and cleanup | Approved fixture reset, account/save isolation, fault removal procedure and clean rematch/session exit | Do not execute mutations without a repeatable reset path. |

Proposed first runnable smoke order: SES-01 → SES-02 → HAR-01 → AUT-01/AUT-03 → POW-01 → OBS-01/02 → OBJ-01 → SES-05. Each arrow requires recorded outcome before continuing; a blocking setup/session/authority failure stops dependent steps and is reported, not counted as multiple observed failures. Full coverage, alternate outcomes and fault variants follow after smoke. This order sequences work and does not exclude any case.

For every D5-dependent outcome fixture, record approved answers before execution: prerequisite power state; uplink completion/interruption rule; eligible extraction zone/duration; minimum extractees and individual outcome treatment; match timebase; all-operatives-dead behavior while operators exist; final creature-body behavior; simultaneous-event ordering; disconnect/abandonment handling. If any answer needed by that fixture is absent, mark its adjudication assertion Blocked rather than choose a winner.

## Continuation review notes

Version 0.2 review found and corrected missing explicit visual evidence references: every case now requires E1 authority/client logs and E2 visual capture alongside E0 metadata. AUT-03 adds explicit coverage for creature melee, traps, electrical hazards and damage-source deactivation races. These are planning improvements, not executed tests.

Main was rechecked at `dc3495d6130cc2410c3ace85a089f7edff57d028`. The GDD/manifest/task-board blobs remain the original baseline. New [environment research notes](https://github.com/nickfucina87-coder/Project-Chimera-/blob/dc3495d6130cc2410c3ace85a089f7edff57d028/ENVIRONMENT_SOURCES_NOTES.md) identify detachable rack internals, salvageable robotics parts, service cavities and repairable breakers as custom/integration gaps. Treat those notes as research input to D6/D9 and INT-06, not verified vendor/license/import facts or completed assets. The architecture, blockout and harvest-rule deliverables and runnable Unreal build remain absent from main. PR #11 has no submitted reviews or inline comments at this continuation check; no approval is inferred.

## Execution record template
Copy per case/profile/role combination; do not overwrite prior failures with later passes.

```text
Case / run ID:
Status: Not run | Blocked | Pass | Fail
Build/content/contract/fixture revisions and seed:
Date/testers; P1–P4 roles; host/server topology:
Hardware/OS/input/scalability; impairment tool/config and measured profile:
Preconditions and approved values/rules:
Actual numbered steps / deviations:
Expected vs actual authority result:
Expected vs actual owner / observer / operator result:
Measured timing/performance and approved budget reference:
E0–E5 artifact links / missing evidence:
Defect links and observed severity:
Dependencies blocking assertions:
Retest build/run and reviewer:
```

## Review, verification and October checkpoint
1. **Plan review:** owner/technical reviewers check complete scope, testability, case steps, authority/client expectations, fixtures/evidence, severity and unresolved dependencies. This document can be reviewed while runtime tests remain Not run.
2. **Acceptance approval:** bind proposed expectations to approved upstream revisions, replace rule-dependent ambiguities with explicit oracles, approve lab profiles/budgets and provide runnable fixtures. Record changes in this QA file on its own branch; do not alter PR #10.
3. **Execution and defect triage:** run packaged four-machine internet smoke, then full matrix and fault/role variants. Record blocked assertions separately from actual failures; fix verification requires linked retest evidence.
4. **Integrated/Verified claims:** require owner-authorized integration plus actual import/compile, internet multiplayer, gameplay and permission evidence. A document or agent report is insufficient. Proposed gate: no unresolved S0/S1 in required acceptance coverage; owner must approve actual release/verification criteria. Never mark unimplemented retained scope as passed.
5. **October 31 report:** list counts and case IDs for Not run/Blocked/Pass/Fail with artifacts; distinguish specified, implemented untested, functional incomplete, asset production and verified evidence. Keep all 52 cases and future features in scope. A missed checkpoint changes progress reporting, not the game vision.

Document-only validation performed during preparation: source/coverage review, unique case IDs and required per-case fields, upstream dependency flags, and isolated GitHub diff/read-back. **No runtime, multiplayer, fault injection, asset import, audiovisual, usability or performance test has been executed under CHM-004.**
