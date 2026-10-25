# Emberfall: The March of Hollowmere

A dark-fantasy, cosmic-horror hex crawl in Unreal Engine 5.8. A sleeping eldritch star hangs over the March, and a cult is trying to call it down to earth.

- **Explore:** every hex on the map is its own fog-shrouded island diorama: winding roads, ruins, waterfalls, ritual circles, things to examine (E).
- **Talk to open roads:** ask townsfolk anything. What you talk about reveals routes on the hex map, and scouting a route costs silver and food. NPC voices can run on a local Ollama model, bound to each NPC's knowledge and `Content/Hollow/Lore/World.txt`.
- **Fight:**
  - Final Fantasy-style command battles on an FFX turn timeline.
  - Pathfinder 2e three-action turns, d20 rolls and degrees of success.
  - A hand of action cards that spend those same actions.

## Building
1. Unreal Engine 5.8 and Visual Studio 2022 (C++ game development workload).
2. Clone with Git LFS (`git lfs install` first). Then open `Foundry.uproject` and let it build the `Foundry` module, or run `Build.bat FoundryEditor Win64 Development -Project=...`.
3. Press Play. The game runs in `/Engine/Maps/Entry`, and the `HollowGameMode` builds the world at runtime.

## Code map (`Source/Foundry/Hollow`)
| File | What it does |
|---|---|
| `HollowGameMode.*` | Game director: state, flow, local areas, events, talk |
| `HollowMap.cpp` | Hex map generation, talk-driven leads, scouting/travel, 3D board |
| `HollowCombat.cpp` | Turn timeline, PF2e actions, cards, foe intents, battle arena |
| `HollowSystems.cpp` | Atmosphere, audio, sprite FX, Niagara, fog of war, title screen, Ollama brain |
| `HollowArt.*` | Runtime prop kit (CC0 Kenney meshes + primitives), lights, fog volumes |
| `HollowUI.*` | Slate UI: title, character sheet, HUD, talk, event cards, animated card hand |
| `HollowData.cpp` / `HollowRules.*` | Content tables and the d20 rules math |
| `HollowCheats.cpp` | `hollow <verb>` console commands for playtesting |

## Docs
`Docs/Roadmap.md`, `Docs/AssetWishlist.md`, `Docs/MechanicsResearch.md`, `Docs/Credits.md`, and concept art in `Docs/Concept/`.

All third-party assets in this repo are CC0 (Kenney, OpenGameArt). Fab / Marketplace assets must never be committed here, because the repository is public.
