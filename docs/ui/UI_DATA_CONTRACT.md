# CHM-003 — UI data contract

Version 0.1 · 2026-10-20 · Review draft · Owner: Codex · [Issue #3](https://github.com/nickfucina87-coder/Project-Chimera-/issues/3)

## Contract status
Companion: [screen specification](UI_SCREEN_SPEC.md). Source baseline: GDD v0.6 §§4–13, 15, 17 and manifest UI-001–008, INT-009, AUD-005, NET-001–010 at main commit `19ca87f2427dddf0a97755e428a1059c7112b68b`.

**No implementation schema or replicated fields are asserted here.** Labels in tables describe information the UI needs. They are not C++ properties, RPC names, serialized structs or approved event names. Every runtime binding is UNBOUND pending CHM-001. GDD §13 supports responsibility categories only. GameMode is server authority, not a client-readable replicated data source.

CHM-001 cross-check on 2026-10-20: issue #1 requests architecture, authority, role transitions, CCTV/camera policy and events; no comments and neither `docs/architecture/UNREAL_SYSTEMS.md` nor `docs/architecture/NETWORK_EVENTS.md` exists in baseline main. Therefore no event-schema compatibility can yet be verified. This is an explicit review dependency, not permission to create Claude's contract or implement surrogate fields.

## Ownership, visibility and widget map
All widget groups in S01–S13 are covered below. Proposed transport/source ownership is separated from authoritative decision ownership. Claude owns runtime architecture; Codex owns presentation in these two files. Local labels/layout/focus/loading spinners are UI-owned; they never grant gameplay authority.

| Screen / widgets | Required information (semantic, not field names) | Authority / proposed readable source | Visibility and reconciliation |
|---|---|---|---|
| S01 title/actions/notice | App navigation and service availability | Local UI; session provider adapter TBD | Local. Service absence disables connection action with reason. |
| S02 browser/details/setup/status | Supported session metadata, joinability, request outcome | Backend/session layer TBD under NET-001; never UI authority | Only discoverable/join-authorized metadata. Refresh/late response must not replace a newer operation. |
| S03 roster/role/ready/start/countdown; S04 capacity/accepted role | Accepted identities, role slots, readiness, start eligibility, phase | GameMode decides; GameState session aggregate + PlayerState team/role proposed by GDD §13; countdown source TBD | Lobby-approved audience only. Requested role stays local pending; server assignment replaces it. Start permission cannot be inferred from local roster count. |
| S04 role descriptions; S05 mission summary/tips | Authored mission/role instructions | Versioned approved design content; selection and match phase from authority | Do not hardcode candidate rules as approved. Loading progress is local and separate from authoritative start. |
| S05 loading/connection | Load status, match transition and failure | Local loader + session adapter; GameMode phase via proposed GameState | Local readiness cannot advance all players. Reconnect support TBD. |
| S06 health/ammo/weapon/tools/flashlight | Own combat and equipment state | Server-validated Pawn/replicated combat/equipment components; exact binding TBD | Owner-visible detail unless CHM-001 approves shared subsets; ammo/health not inferred from animation or PlayerState alone. |
| S06 objectives/timer/markers; S07 objectives/timer; S10 objectives/timer | Permitted objective status, phase, extraction and time basis | GameMode rules → GameState phase/timer/objectives per GDD; target markers from authorized objective/interactable source | Server-filtered role visibility required. Timer interpolation presentation only; local zero cannot end match. |
| S06 prompt | Target label, allowed interaction, accepted progress/reason | Local aiming candidate; server-authorized interaction component outcome | Only relevant visible/authorized target. Clear invalid or stale candidate. |
| S07 resources/modules/chassis/abilities/reserve; S08 inventory/eligibility | Own materials, equipment, active chassis, available actions, eligible backup status | Server-authoritative inventory/recipe/chassis components; PlayerState preserves player identity, GameMode authorizes transitions | Owner-only sensitive detail. Never expose entity recipes/reserve sites to humans by hiding widgets over broadly replicated data. |
| S08 recipe catalog/requirements | Discovered recipes, requirements, known tradeoffs | Approved content plus server-authorized discovery/eligibility from recipe component | Candidate GDD combinations are examples, not final recipe IDs/costs. No undiscovered recipes leaked. |
| S08 request/status/close | Composition/chassis outcome and reason | Server validates via controller/component route TBD; local panel focus | Reject leaves confirmed inventory/chassis intact; body change rebinds to new authorized Pawn. |
| S09 label/slot/outline | Machine and individual component identity plus targeting anchor | Authored interactable identity; local aim selection constrained by authority | Stable identity scheme TBD; never select by display name or mutable array position. No global component inventory. |
| S09 consequence/eligibility/progress/begin/cancel | Disclosed consequence, allowed action, accepted progress, termination/rejection | Server interaction + machine/power dependency components | Range/duration/availability/yield validated server-side. Cosmetic progress must not imply detach/reward. Confirmed component removal and inventory update reconcile together; atomicity/order guarantee TBD. |
| S10 role banner/feed list/image/status | Confirmed observer role and permitted working feeds | GameMode role policy + PlayerState observer status; controller-authorized CCTV service/components proposed | Permission must gate data/feed delivery, not merely UI. No hidden transforms, unauthorized map state or retained images after revocation. |
| S10 pings/facility actions/restrictions | Allowed targets/actions, outcome, recipient scope | Server CCTV/permission and facility components; controller requests only | Ping audience, cooldowns, bounds and available actions unbound; default unavailable until authorized. Revalidate every request. |
| S11 controls/text/contrast/audio/apply/revert | Local preferences and supported options | Local settings subsystem; persistence adapter TBD | Local device scope. Applying UI/audio preference does not mutate shared gameplay rules. |
| S11 camera/voice/mute | Allowed modes, preferred mode, supported voice/privacy capability | Server camera/voice permission policy with local preference/controller; backend TBD | Local preference intersected with allowed policy. No client grant of overhead vision or voice overhearing permissions. |
| S11 leave; S03 leave; S12 return/leave | Session departure outcome | Session layer + server lifecycle rules TBD | Local confirmation does not fabricate host migration/abandonment result. |
| S12 winner/reason/statistics/personal results | Final outcome, extraction, power, salvage, chassis losses, kills, sabotage | GameMode resolves; GameState final aggregate and PlayerState identity/result proposed; statistics producer/visibility TBD | Immutable authorized result snapshot proposed; exact scope pending. No winner inferred from killfeed, timer or client objective counters. Unknown stats are unavailable. |
| S12 rematch control/status | Request acceptance and next session/phase | GameMode/session lifecycle → GameState/PlayerState proposed | Clear prior-match subscriptions and sensitive cached values on accepted transition; no local restart. |
| S13 account/catalog/requirements/equipped/save/status | Authorized progression and parity-safe choices | NET-010 account/save service TBD (Codex+Claude ownership boundary requires review); local selection preview | Authenticated own account only; remote save permission and persistence undecided. A preview is never a confirmed unlock/equip. |

## Proposed request intents for CHM-001 review
These IDs are review references only. All are **UNBOUND / not approved network events**. Request payload encoding, concrete IDs, owner routing, delivery guarantees, rate limits, sequence handling, error taxonomy and response schemas belong to CHM-001. UI supplies intent; server decides and returns authorized current state/outcome.

| Intent | UI origin / semantic input | Required validation and accepted presentation |
|---|---|---|
| I01 Discover/host/join/cancel/leave session | S01–03, S11–12; supported session selection/options | Backend/session authorization, capacity and lifecycle. Match attempt response to originating operation; stale result ignored. |
| I02 Request role/readiness/start | S03–04; preferred role or readiness/start intent | Role capacity, player identity, phase, host/start permission and readiness prerequisites. Reflect accepted roster; no optimistic role grant. |
| I03 Begin/cancel component harvest | S09; exact target/component identity; ongoing interaction identity TBD | Actor existence, component availability, role, range/duration, interruption and objective dependency policy. Duplicate completion cannot reward twice. UI observes accepted status and inventory/component state. |
| I04 Compose/equip/use module | S07–08; authorized recipe/module choice and action target if needed | Discovery, materials, slot/equip rules, target, cooldown and phase. Rules/costs not invented. Display pending until accepted; refresh on ambiguous result. |
| I05 Request chassis change/reserve transfer | S07–08; eligible choice/site if rules require | Active body/role, reserve availability, reconstruction site, cost, spawn safety and transition rules. No client-controlled respawn or forced evolution. |
| I06 Operative interaction/tool action | S06; target/action intent | Gear ownership, target permission, range, objective dependency, cost and phase. Weapon/gameplay input routing belongs to runtime, not UI code here. |
| I07 Select CCTV/ping/perform facility action | S10; permitted camera or authorized action/target | Observer role, per-feed permission, working power/camera, recipient and facility-action bounds; revoked permission invalidates queued actions. |
| I08 Request camera mode | S11; preferred supported view | Competitive camera policy, role/chassis eligibility and collision/visibility constraints. UI shows allowed effective view. |
| I09 Request rematch | S12; rematch preference | Lifecycle, participant/session policy and result finalization. Next phase comes from server. |
| I10 Save progression/equipped choice | S13; authorized selection | Account ownership, entitlement, parity and save conflict handling. Service-owned result; no speculative XP/grant. |

Local settings/navigation are local operations. They need no gameplay RPC unless an approved server policy is involved. Session and account intents may use a service rather than Unreal replication; architecture must decide.

## State and recovery requirements (proposed)
1. Bind by authorized session/player/match/body/target identity; unsubscribe before rebinding. New match or body invalidates incompatible cached data.
2. Initial snapshots must precede enabled actions. Unknown/unbound/unauthorized values remain absent; disable affected controls with a plain reason.
3. Requests need an agreed correlation and deduplication strategy. Do not automatically retry harvest/craft/transfer/save after an ambiguous disconnect; obtain current state first. Concrete mechanism TBD.
4. Authority may change a state without a UI request. Apply phase/role/permission updates even with a menu open; stale callbacks cannot resurrect a dead Pawn HUD or revoked CCTV feed.
5. Confirm server time basis, correction behavior and progress interruptions. Local animation can smooth approved progress but cannot confirm completion.
6. Define coherent ordering of component detach, inventory reward, facility consequences, death/backup transitions and final result. UI must tolerate partial arrival without double reward, false success or hidden-state inference.
7. On connection loss, mark data stale and disable mutating controls. Show reconnect only if supported; successful reconnect uses a fresh permission-filtered snapshot.
8. Exclude sensitive payload values from generic client diagnostics. Logging identifiers/event schema and retention require CHM-001 review.

## Binding acceptance checklist
| Dependency / decision | Current evidence | Required before implementation |
|---|---|---|
| CHM-001 architecture and NETWORK_EVENTS | Issue scope checked; files absent; no issue comments at review baseline | Approved source revision; concrete producer, reader, visibility, payload, validation, ordering and rejection mapping for every row/I01–I10. |
| CHM-008 harvest/sabotage rules | GDD §8 consequences and fail-safe requirement only | Approved consequence disclosure, repair/loss paths and cancellation rules; Codex does not author gameplay authority. |
| Camera/operator policy | GDD §§9,11,13 high-level restrictions | Feed/role revocation semantics, action scope, ping recipients, permitted camera modes. |
| Match rules | GDD §5 outcomes explicitly proposed | Timer, partial extraction, disconnect, stalemate, final body destruction and rematch decisions. |
| Recipes/chassis | GDD §§7–8 candidates and open questions | Discovery/yield/cost/slot/cooldown, reconstruction/reserve rules and owner-visible details. |
| Session/account/voice services | GDD §§3,10,13,17 undecided | Backend/auth, reconnect, persistence conflicts, voice/privacy support and supported input platforms. |

Review evidence for this draft: all S01–S13 widget groups mapped; UI-001–008 covered; no concrete replicated field/RPC names claimed; CHM-001 dependency explicitly checked. Usability exercises are listed in the screen spec but not executed. This draft may enter Review, but cannot be marked Approved, Integrated or Verified from document creation alone.
