# CHM-003 — UI screen specification

Version 0.1 · 2026-10-20 · Review draft · Owner: Codex · [Issue #3](https://github.com/nickfucina87-coder/Project-Chimera-/issues/3)

## Scope and source baseline
Planning only; no UMG, Unreal code/assets, purchases or integration. Based on main commit `19ca87f2427dddf0a97755e428a1059c7112b68b`: [GDD v0.6](../MASTER_GAME_DESIGN.md), [manifest](../ASSET_MANIFEST.csv), [manifest guide](../ASSET_MANIFEST_GUIDE.md), and [task board](../AGENT_TASK_BOARD.md). This is a proposed UI contract, not evidence of runtime functionality. October 31 remains a progress checkpoint; all planned screens remain in scope.

Only Codex assignments guide this work: CHM-003 now; CHM-004 multiplayer QA planning remains separate and unstarted. CHM-001 is read solely as a required upstream contract. Its architecture/network-event files are absent at the baseline; see [data contract](UI_DATA_CONTRACT.md) for unbound dependencies. Do not treat the proposals below as approved rules or replication fields.

## Inventory and widget coverage
Screen IDs and widget IDs below are document references, not Unreal class names. Every listed widget group maps to the same ID in the data contract.

| ID / manifest | Screen and widgets | Entry / exit and interaction |
|---|---|---|
| S01 / UI-001 | Main menu: title, Host, Join, Settings, Progression, Exit, connection notice | Application entry; Host/Join opens S02; settings S11; progression S13. Exit confirmation prevents accidental quit. |
| S02 / UI-001, NET-001 | Session browser/setup: session list, refresh, join detail, host setup, pending/error, cancel/back | Backend unspecified: no invented region/privacy/NAT features. Show only supported options. One pending connection attempt; success S03, cancellation S01. Retry keeps context; distinguish full session, unavailable session and generic failure only when service supplies reason. |
| S03 / UI-002 | Lobby: roster, connection state, role request, ready control, mission summary, start/countdown, leave | Initial target is 3 operatives + 1 entity. Role choice is a request; accepted roster determines availability. Ready/start prerequisites and host migration await CHM-001. Transition only on accepted match phase. |
| S04 / UI-002 | Role selection panel: operative/entity descriptions, capacity/eligibility, requested/accepted role, back | Subpanel of S03. Explain humans restore/transmit/extract; entity harvests/sabotages/prevents escape. Denied selection leaves accepted role intact. Do not reveal hidden enemy loadouts. |
| S05 / UI-001, NET-006 | Mission briefing/loading: mission title, ordered goals, role tips, loading status, connection failure | Lobby to match. LAST TRANSMISSION: restore emergency power → access/activate uplink → maintain transmission → extract. Local load completion cannot start the match. Failure offers supported return/reconnect path only. |
| S06 / UI-003 | Human HUD: health, magazine/reserve ammo, weapon, flashlight/tools, objective/timer, interaction prompt, restrained authorized markers, connection status | Operative gameplay. Objective win requires power, completed signal and at least one extraction under proposed GDD rules; killing a body alone does not display victory. Health/injury units await design. |
| S07 / UI-004 | Creature HUD: material inventory, active chassis, equipped modules, ability availability, objective/timer, backup eligibility/status, connection status | Entity gameplay in either chassis. Small hexapod remains a valid choice. Show only owner's authorized inventory/reserve information; no map-wide machinery scan implied. |
| S08 / UI-004 | Recipe/module/chassis panel: discovered recipes, requirements, available resources, modules, compose request/status, chassis request/status, close | Enter from S07. Candidate recipes are explicitly provisional. No fixed slots, yield, cost, cooldown, life count or assembly-location rule invented. Disable unbound actions with explanation. Pending requests do not spend resources or change chassis locally. |
| S09 / UI-005 | Component harvest overlay: machine/component label, selected slot outline, known consequence, eligibility/reason, begin/cancel, accepted progress/status | Entity targets an individual authored component, not a whole rack. Reacquire after aim loss; reject out of range/stale/removed target. On completion wait for server inventory + component state; no optimistic reward. Power/logic/cooling components remain distinct. |
| S10 / UI-006 | Dead operator console: role banner, permitted feed list, selected feed/status, authorized ping and facility controls, restriction reason, objective/timer, connection status | Enter only on server-confirmed eliminated operative/operator role. Replace living HUD and clear living-role prompts. Camera power loss/revocation clears image and controls immediately on received update; no free camera or cached enemy markers. Zero feeds has an explicit empty state. |
| S11 / UI-001, AUD-005 | Settings/pause: input bindings, text/UI scale, contrast/non-color cues, audio/subtitles options, camera preference and policy explanation, voice/mute capability, apply/revert, leave confirmation | Accessible from menus and match; opening it does not imply pausing online play. Present unsupported options as unavailable with reason. Camera preference cannot override allowed views. Integrated proximity voice is planned; external voice may be used meanwhile. No promise that external voice can be muted here. |
| S12 / UI-007 | Results/rematch: server winner/reason, extraction, power restored, harvested components, chassis losses, kills, major sabotage, personal results, rematch request/state, return/leave | Enter on authoritative result. Missing statistics show unavailable, never invented zero. Partial extraction is per-operative where authorized. Final-body kill does not infer winner. Rematch starts only after accepted lifecycle transition, resetting prior UI. |
| S13 / UI-008, NET-010 | Progression: authorized account identity/status, cosmetic/unlock/achievement list, requirements, equipped selection, save/pending/error, back | Out-of-match surface; backend/save schema unspecified. Preserve parity: no invented paid advantage, XP formula or unlock values. Failed save preserves confirmed selection and permits supported retry. |

## Flow and wireframes
Wireframes show hierarchy and information density, not final art. Controls must have text labels, visible focus and input prompts matching supported device.

```text
S01 → S02 → S03 [S04 role panel] → S05 → S06 human / S07 creature
 |            |                              |          |
 S11/S13      leave → S01                     S10        S08/S09
                                                \      /
                                             S12 results
                                             rematch → S03
```

Transitions above are conceptual: CHM-001 must confirm rematch destination, join-in-progress, disconnect/reconnect and abandoned-match behavior. Settings is an overlay during play.

```text
LOBBY S03                         ROLE PANEL S04
Mission: LAST TRANSMISSION         Operative: restore / transmit / extract
Roster | accepted role | ready    Entity: harvest / sabotage / hunt
[Role request] [Ready request]     [Request role] [Back]
Status / eligibility / reason      Requested ... / Accepted ...
[Leave]  [Start if authorized]     Capacity from accepted roster
```

```text
HUMAN S06                         CREATURE S07
Objective + phase/timer            Objective + phase/timer
                                  Chassis / own modules
       world view                         world view
       component/object prompt            S09 targeted component
Health | weapon/ammo | tools       Resources | ability availability
Connection notice                 Recipes / chassis / reserve status
```

```text
COMPONENT S09                      COMPOSER S08
Machine / selected component       Known recipe | requirements
[slot outline on that component]   Confirmed inventory / eligibility
Known local consequence            [Compose request] pending/reason
[Begin] / reason                   Chassis choice + tradeoff / eligibility
Accepted progress [Cancel]         [Request change] [Close]
```

```text
OPERATOR S10                       RESULTS S12
Remote operator / restrictions     Winner + authoritative reason
Permitted feeds | selected view    Extraction / objective outcomes
Feed status: live/offline/revoked   Authorized match statistics
[Ping] [Allowed facility action]   [Rematch request] accepted/waiting
Objective / timer / connection     [Return / Leave]
```

S01/S02/S05 use one primary action area and a persistent status area. S11 uses a category list, editable values, then Apply/Revert; S13 uses a catalog, detail/requirements, then selection/save status. No essential status relies on hover alone.

## Behavior shared across screens
- Unknown, loading, empty, pending, rejected, offline and terminal states have distinct text. Unknown health/ammo/inventory is not zero; unavailable result is not a loss.
- Use confirmed state as truth. A click displays a pending request; reject/timeout clears the spinner and explains retry without assuming whether a gameplay mutation committed. Refresh confirmed state before retrying non-idempotent actions.
- Disable duplicate pending submissions. Closing a panel changes focus only; it must not silently cancel an accepted gameplay interaction without the agreed cancel request.
- Role, phase and permission changes override local navigation. Clear stale target/feed/selection on entity destruction, new match, disconnect or relevant permission loss.
- Keep mission urgency readable without presenting global hidden state. World markers use authorized objective/interaction information, never hidden entity position. Known salvage consequences must not expose an unseen dependency graph.
- Camera options explain server restrictions; no unrestricted FPS/TPS/isometric promise. Display a coherent unavailable reason when a mode is disallowed.
- Propose scalable UI/text, remappable supported controls, visible focus, readable contrast, non-color eligibility cues, captioned critical cues and independent audio controls. Exact device support and numeric accessibility targets require owner/platform review.
- Alerts prioritize connection loss and role/phase transition over optional recipe/progression notices. Critical gameplay cues remain readable with menus open without expanding vision or revealing hidden data.

## Usability and review checks (planned, not executed)
| Check | Exercise | Pass observation |
|---|---|---|
| U01 | Four participants request roles concurrently | Accepted roster resolves to approved 3v1; denied requester sees reason; pending is distinct from accepted. |
| U02 | New player reads briefing then HUD | Can identify restore → transmit → extract and understands body kill alone is insufficient. |
| U03 | Aim at adjacent PSU, board and fan | Each prompt identifies the exact selected component; no whole-rack harvest ambiguity. |
| U04 | Begin harvest; move out of range; second player/system removes target | No reward before acceptance; progress clears/reconciles; rejection is understandable. |
| U05 | Compose with stale inventory; change chassis; active body destroyed | No optimistic spend/evolution/respawn; owner sees confirmed state and rule-dependent reasons. |
| U06 | Operative dies; CCTV loses power or permission | Only permitted feed/control information remains; frozen image is cleared; no free spectator camera. |
| U07 | Disconnect during request and reconnect if supported | Pending state resolves from current snapshot; no duplicate rewards/actions or stale HUD. |
| U08 | Results for partial extraction, body kill, timeout | UI displays server decision and available stats without deriving unsupported outcomes. |
| U09 | Navigate all surfaces using supported controls, large text and non-color cues | Focus is visible, labels remain legible, core actions/status do not depend on color/sound alone. |
| U10 | Rematch then swap role | Old target, camera, inventory display and result data cannot leak into new identity/match. |
| U11 | Inspect entity/human/operator views side by side | Hidden recipes, resources, reserves, cameras and positions reach only authorized viewers. |
| U12 | Save progression fails; camera preference is denied | Confirmed selection persists; failure/restriction is explicit; competitive rules remain enforced. |

These checks prepare CHM-003 review and later CHM-004 QA input; no multiplayer, accessibility or Unreal verification is claimed.

## Review gates
Owner: approve information hierarchy, accessibility goals, mission wording, and unresolved gameplay choices. CHM-001: bind every data-contract row and intent to actual authority, replication visibility and error/transition behavior. CHM-008: approve harvest consequence disclosure and sabotage rules before final S09 copy. Codex then revises only these UI files. No approval, integration, implementation or verification status is implied by this draft.
