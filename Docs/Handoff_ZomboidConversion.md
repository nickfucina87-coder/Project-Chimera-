# Handoff: Emberfall as a Project Zomboid total conversion (mixed mechanics)

Written 2026-10-26, for the next session. Repo: `github.com/nickfucina87-coder/Project-Chimera-` (`main`). Local working folder: `C:\Users\nickf\Documents\Unreal Projects\Foundry`.

## Where to open the next session

- **Open Claude Code in the same `Foundry` folder.** The memory and this repo hold everything, so don't start a fresh Unreal project.
- The Zomboid mod is not an Unreal project. It's Lua plus Zomboid scripts, and it will live in this repo under `Mods/EmberfallPZ/` (our own files only; never Zomboid's art or data). For testing it gets copied or linked into `%USERPROFILE%\Zomboid\mods\`.
- The Unreal project (Emberfall / Hollowmere) stays as it is, as the eventual standalone and the rules reference. Unreal comes back into this only for the animation work (step 6).

## The idea

Use Project Zomboid's world as the playground: Kentucky towns, buildings, ~5,100 items, crafting, survival, line of sight, stealth and loot by room. Bring across everything that makes Emberfall Emberfall:

- Pathbuilder/Foundry pf2e characters and d20 rules
- our lore and names
- NPCs you can talk and trade with
- zones for dangerous or rare areas
- a turn-based layer that pauses the world while a fight resolves

Zombies are turned off at first; a few stay as placeholder monsters (lich, ghoul, the drowned). It's a private proof of concept, not for sale: it's a mod on their engine. Our design (rules, lore, item names, maps) stays portable back to the Unreal game.

## What is already done

| Piece | Where | What it gives the mod |
|---|---|---|
| Item rename sheet | `ExternalAssets/PZ/items.csv` (local) via `Tools/PZItems.py` | 5,105 items (4,542 with English names); fill in `emberfall_name`, `reskin_notes`, `keep_cut` |
| pf2e rules library | `ExternalAssets/Library/library.json` via `Tools/BuildLibrary.mjs` | classes, 45 subclass choices, 5,727 feats, 1,785 spells, 5,511 items, 662 monsters, rule elements, your world's NPCs, journals and tables |
| Pathbuilder import logic | `Source/Foundry/Hollow/HollowExplore.cpp` (`ApplyPathbuilder`) | the field mapping to port to Lua |
| Character builder rules | `HollowBuilder.cpp`, `HollowEffects.cpp` | pf2e boost, skill and feat rules, FlatModifier stacking |
| Turn-based card combat | `HollowCombat.cpp`, `HollowGrid.cpp` | the rules to port into the paused-world fight |
| Zomboid file readers | `Tools/ExtractPZ.py`, `Tools/PZLots.py` | tile properties and whole buildings (also used in the Unreal prototype) |
| Design notes | `Docs/MechanicsPlan.md`, `Docs/ModularWorld.md`, `Docs/ArtAssets2D.md` | earlier plans |

### Facts checked on this machine's Zomboid install

- It's Build 42. Scripts are in `media/scripts` (1,004 files), translations in `media/lua/shared/Translate/EN/*.json`.
- Maps live in `media/maps/<Town>/`: `.lotheader`/`.lotpack` cells, `objects.lua` zones, `spawnpoints.lua`.
- Animation:
  - 2,209 DirectX `.x` animation files in `media/anims_X` (`Bob` = the human rig).
  - 2,949 AnimSet XML state files in `media/AnimSets` (states, transitions, variables).
  - Models are in `media/models_X` (`.x` and some `.fbx`).
- The user's `Zomboid` folder exists (Saves, Lua, mods), and there are 469 Workshop items subscribed.

## Feasibility, piece by piece

| Want | How in Zomboid | Difficulty |
|---|---|---|
| No zombies, a few as monsters | Sandbox: zombie population 0; spawn specific "monster" zombies with custom outfits and models from Lua; tune speed, strength and senses | Easy / medium |
| Rename and reskin items | `media/lua/shared/Translate/EN/ItemName.json` override for names; item script overrides for icons and stats; new items as scripts | Easy, but a lot of it (5,100 items) |
| pf2e character | ModData on the player holds the sheet (abilities, proficiencies, feats, spells); professions and traits for creation; our own Lua character window | Medium |
| Pathbuilder import | Lua can't make web calls. The player saves Pathbuilder's JSON into `%USERPROFILE%\Zomboid\Lua\`; the mod reads it with `getFileReader` and a small Lua JSON parser, then ports the `ApplyPathbuilder` mapping | Medium |
| Do we need REA for Pathbuilder? | No. Its JSON export is the interface, and the mapping is already written (and Pathmuncher's MIT tables are in the library). REA only helps if we want to copy its builder's screens and flow | n/a |
| d20 checks everywhere | Lua helpers: d20 + modifier vs DC, four degrees of success; hook into actions (lockpicking, forcing doors, climbing, searching) through Zomboid's timed actions and events | Medium |
| NPCs you talk to and trade with | Zomboid has no human NPCs yet. The practical route, proven by community "bandit/companion" mods: spawn a zombie-type character, give it a human outfit and the player animation set, make it non-hostile with a custom Lua brain, add a right-click "Talk / Trade" menu, our dialogue window, and a shop panel. Idle "jobs" (shopkeeping, sweeping, guarding) are looping animation states | Medium / hard |
| Local AI voice for NPCs | No networking from Lua. A file bridge instead: the mod writes the question to a file, a small helper app beside the game asks Ollama and writes the answer back | Medium |
| Zones (rare areas, high-fantasy areas) | Our own zone layer: a Lua table of polygons or rectangles (or an image "splat" map sampled by world coordinates) defining danger level, monster table, loot tier and weather. Spawning and loot read it. Zomboid's own `objects.lua` zones still feed vanilla systems | Medium |
| Turn-based fights | When a fight starts, freeze the world (game speed 0, or a very slow speed so animations still read), open our battle UI, resolve the pf2e three-action turns and cards in Lua, play results back (damage, knockdown, death, animations), then resume. Monsters "register" their moves on their turn from our AI. The grid can be Zomboid's own squares around the fight | Medium / hard, and the most interesting part |
| Our animations later | Zomboid's rig is `Bob` (`.x`). Path: convert `.x` to FBX (Blender with a DirectX importer), retarget to the Unreal mannequin, or the other way round to bring our Unreal animations into Zomboid as new AnimSet states | Medium |

## Order of work for the next session

1. **Mod skeleton** at `Mods/EmberfallPZ/`: `mod.info`, `media/lua/{shared,client,server}`, `media/scripts`, and a sandbox preset with zombies off. Test it loads, and spawn in one town (start with Muldraugh or Riverside).
2. **Item names pass 1:** take ~50 items from `items.csv`, write the Emberfall names into a translation override, and check them in game. This shows the rename pipeline end to end.
3. **pf2e sheet:**
   - a ModData character sheet
   - a d20 check helper
   - a character window (abilities, proficiencies, feats)
   - Pathbuilder import from `%USERPROFILE%\Zomboid\Lua\pathbuilder.json`
4. **One NPC:** Tobb the innkeeper in a tavern in the starting town. He uses a human outfit, is non-hostile, has a Talk menu with our dialogue window (topics, skill checks, item gates) and a Trade window.
5. **Turn-based fight prototype:** one placeholder monster. On contact the world freezes, the battle UI opens, three-action turns run with Strike, Stride and a spell card, and the world resumes when it's over.
6. **Zones:** a first danger or rarity overlay for the starting town, which drives which monsters spawn.
7. **Animation notes:**
   - document Zomboid's AnimSet states for walk, attack and idle
   - try converting one `Bob` `.x` animation to FBX for Unreal

## Rules for this project

- **Never commit Zomboid's art, maps or scripts, and never commit the pf2e library** (Paizo). Only our own Lua, scripts and docs go in `Mods/EmberfallPZ/`. Anything extracted goes in `ExternalAssets/` (gitignored).
- **Their code:** don't decompile or copy Zomboid's Java. Use their public Lua and script modding interface, which is how mods are meant to work.
- **Keep the Unreal project building.** It is the fallback and the rules reference.
