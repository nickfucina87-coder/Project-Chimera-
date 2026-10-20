# Project Chimera — Agent Task Board
**Created:** 2026-10-20  
**Status:** Planning issues created, execution unassigned.  
**Important:** October 31 is a progress checkpoint; no agreed features are being removed.

## Single source of truth
- [Master GDD](MASTER_GAME_DESIGN.md)
- [Asset inventory](ASSET_MANIFEST.csv)
- [Asset and blockout guide](ASSET_MANIFEST_GUIDE.md)
- [GitHub Issue Queue](https://github.com/nickfucina87-coder/Project-Chimera-/issues)

## Task list
| ID | Proposed worker | Task | GitHub issue | Gate |
|---|---|---|---|---|
| CHM-001 | Claude | Unreal systems/replication architecture | [#1](https://github.com/nickfucina87-coder/Project-Chimera-/issues/1) | Review |
| CHM-002 | Claude | Facility blockout specification | [#2](https://github.com/nickfucina87-coder/Project-Chimera-/issues/2) | Review |
| CHM-003 | Codex | UI screen & data contract | [#3](https://github.com/nickfucina87-coder/Project-Chimera-/issues/3) | Review |
| CHM-004 | Codex | Multiplayer QA plan | [#4](https://github.com/nickfucina87-coder/Project-Chimera-/issues/4) | Review |
| CHM-005 | Grok/Grokbots | Environment asset/license research | [#5](https://github.com/nickfucina87-coder/Project-Chimera-/issues/5) | Review |
| CHM-006 | Grok/Grokbots | Animation/audio/VFX asset research | [#6](https://github.com/nickfucina87-coder/Project-Chimera-/issues/6) | Review |
| CHM-007 | Tripo/art | Hexapod model/rig contract | [#7](https://github.com/nickfucina87-coder/Project-Chimera-/issues/7) | Review |
| CHM-008 | ChatGPT / owner | Harvest + sabotage rules | [#8](https://github.com/nickfucina87-coder/Project-Chimera-/issues/8) | Review |
| CHM-009 | ChatGPT / owner | Manifest provenance and acceptance | [#9](https://github.com/nickfucina87-coder/Project-Chimera-/issues/9) | Review |

## Operating rules
- GitHub Issues are task records, **not** remote instructions delivered automatically to Claude/Grok/Tripo.
- Workers begin only when their issue has been handed to them in their connected interface or delegated tool. They may propose designs now; do not edit the working Unreal project until approved.
- Prefer one agent per implementation file/module; avoid simultaneous changes to shared Unreal assets.
- Track: Backlog → Ready → In progress → Review → Approved → Integrated → Verified.
- Owner approves integrations; verification requires evidence (UE import/compile/test, actual online multiplayer and gameplay).
- Link changes and final deliverables back to their GitHub Issue. Do not mark a task finished solely because an agent reported it complete.

## Recommended initial handoffs
1. Claude: #1 and #2, specifications only.
2. Grok: #5 and #6, cited asset sourcing and licensing only.
3. Codex: #3 and #4, independent design and test planning.
4. Tripo: #7, geometry/rig feasibility specification. No final production models until review.
5. Review outputs for shared dimensions and interactions before issuing implementation tasks.

This is an **issue-based task board**, not a GitHub Projects Kanban view.
