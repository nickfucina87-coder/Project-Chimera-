# Emberfall mechanics plan

The working plan for the deep-mechanics phase. Last updated 2026-10-25 22:56.

The goal: bring the heavy mechanics of Foundry VTT pf2e into Emberfall's style of play. You explore inside one hex at a time and fight turn-based, but the rules underneath are Foundry's.

## Ground rules

- **Mechanics first.** The look will be redone later, so don't polish visuals now.
- **Future art direction:** 2D top-down "paper" style, like Paper Mario. Sprites always face the camera, and the camera can still rotate.
- **Keep:**
  - the main menu
  - the hex/map system
  - zoom
  - the dialogue/pause/inventory frameworks, which can be reskinned
- **Repo hygiene:** Paizo content and Fab assets never go into the public repo. The rules library lives in `ExternalAssets/Library` (gitignored) and is rebuilt with `node Tools/BuildLibrary.mjs`.
- **Editor workflow:**
  - Header and UCLASS changes need the editor closed, a FoundryEditor build, then a relaunch.
  - `.cpp`-only changes can be hot-reloaded with Live Coding (type `LiveCoding.Compile` into the editor console box).
  - While the user is away and the editor is open, do `.cpp`-only work through Live Coding and queue the header changes for the morning.

## 1. Character creation: every class, every option (top priority)

Every option in the Foundry pf2e compendium must be buildable at the start of the game.

- **What to cover:**
  - all 27 classes (Magus, Sorcerer, Kineticist, Inventor and so on)
  - subclasses: Magus hybrid studies, Sorcerer bloodlines, Wizard schools, Cleric doctrine and deity, Rogue rackets, Barbarian instincts, Champion causes, Druid orders, Witch patrons, Bard muses and others
  - ancestries and heritages
  - backgrounds
  - ability boosts, using the free-boost rules for ancestry, background, class and the four level-1 boosts
  - skills (trained skills plus Int-based extras)
  - level-1 feats (ancestry, class, and skill where the class grants one)
  - spells (cantrips and level-1 repertoire or spellbook)
  - deity
  - starting gear (a kit or 15 gp of shopping from the library)
- **Library gaps:** the subclass choices live in `classfeatures` items with ChoiceSet rule elements, so `BuildLibrary.mjs` must export each feature's rule elements (choices, GrantItem, ActiveEffectLike) alongside its text.
- **Study Pathbuilder 2e's builder flow.** Check whether a desktop or web build is installed and inspect it with REA. Look at its step order, the "choices pending" model, validation (prerequisites, rarity, level gates) and the summary sheet.
  - Use Pathbuilder only as a UX reference. Its data isn't ours; our data comes from the Foundry compendium.
- **Creation UI:** a step wizard (Ancestry → Heritage → Background → Class → Subclass → Boosts → Skills → Feats → Spells → Gear → Review), with a live sheet beside it. A "pending choices" list blocks Confirm until it's empty.
- **Pathbuilder import stays** as a shortcut, and should resolve to the same data model.
- **Generic class model in C++:**
  - `FClassBuild`: key ability, HP, proficiencies by level, feature list by level, spellcasting type (prepared / spontaneous / none), tradition, slots per rank per level.
  - Replace the four hand-coded starter kits with it. Kits become presets.

## 2. Battle mechanics (deep)

The goal is to move from FF-style turns toward Foundry's pf2e encounter rules while keeping the card layer.

- **Camera:** orbit, zoom and pan freely during combat, including on the enemy's turn. Use RMB drag or Q/R to orbit, plus Shift+click for smooth free rotation (see 4).
- **Grid and positioning:** a hex battle grid inside the arena, or the current hex island itself.
  - Moving is the Stride action: speed in feet, 5 ft per hex. Step and Strides spend actions.
  - Reach, flanking (off-guard), cover from the line-of-sight system, and difficult terrain.
- **Real action economy:**
  - 3 actions plus 1 reaction per turn.
  - Action types: single, activity, free, reaction.
  - Multiple attack penalty, which already exists.
  - Attacks of Opportunity as reactions for classes that have them.
- **Foundry-style rule elements as the engine:** make items, feats and spells data-driven by implementing a subset of pf2e rule elements:
  - FlatModifier (bonus/penalty by selector)
  - DamageDice
  - Strike
  - Resistance / Weakness / Immunity
  - AdjustDegreeOfSuccess
  - RollOption
  - GrantItem
  - Aura
  - EphemeralEffect
  - Note

  The library already has the item data. Export `system.rules` too.
- **Conditions engine:** use the full pf2e conditions from the library (frightened, sickened, clumsy, enfeebled, stupefied, drained, off-guard, prone, grabbed, restrained, slowed, quickened, stunned, persistent damage, dying/wounded, and so on). They need values, durations, end-of-turn decrement and how they stack with modifiers.
- **Enemies from the library:** use the bestiary packs, plus your world's 57 NPCs, with their strikes, abilities, reactions, resistances, weaknesses and immunities. The boss fight will later split into its own style, so not now.
- **Degrees of success everywhere,** with crit effects and the weapon crit-specialization groups.
- **Cards:** spell cards already work. Add:
  - feat cards (class feats with actions become cards)
  - weapon trait cards (agile, sweep, forceful, deadly and so on, as modifiers)
  - consumable cards (potions and scrolls)
- **Hero Points:** 1 per session; reroll, or avoid death.
- **Recovery checks, dying and wounded.**

## 3. Asking / social mechanics

- A per-NPC attitude track (hostile → unfriendly → indifferent → friendly → helpful), changed with Diplomacy Make an Impression, Request, Lie, Coerce and Gather Information, using pf2e DCs by level.
- Topics unlock leads; asking has a cost (time and silver); rumor tables come from your world (the Rumors table).
- Influence-style encounters for key NPCs (pf2e subsystem: influence points, discoveries, resistances and weaknesses).
- The LLM free voice stays, bound to lore.

## 4. Exploration in the hex

- **Smoother camera:** interpolate a free yaw while Shift+drag or Shift+click is held, on top of the 60° Q/R snaps. Add easing.
- **Bump-into interactions:** walking into doors, crates or NPCs prompts an action.
- **Edge travel:** test it. You exit through a side and enter the next hex facing the same way. Blocked sides show why: a cliff, fog, or "nobody told you".
- **Line of sight:** already in place. Next:
  - place your Foundry scene walls and lights (13 scenes, up to 267 walls) as invisible underlays for town and set-piece maps
  - darkvision and low-light vision by ancestry
  - Sneak, Hide and Seek actions that use the vision state
- **Exploration activities, as Foundry has them:** Avoid Notice, Search, Scout, Detect Magic, Defend, Hustle. Each affects encounter start and initiative (Perception vs Stealth).

## 5. Foundry compendium in game

- **Done:** classes, ancestries, heritages, backgrounds, feats, class features, spells, equipment, conditions, actions, deities, world journals, tables, NPCs and scenes.
- **Next:**
  - rule elements (`system.rules`)
  - the bestiaries
  - hazards
  - spell effects and equipment effects (as buffs)
  - rollable tables wired to loot and rumors
- **Icons:** pf2e/Foundry icons are not free to redistribute. Use them only through local paths, or make our own.
- **Effects:** JB2A is NC-licensed, so only the idea. Spell VFX come from damage-type families using our own Niagara.

## Decisions (user, 2026-10-25)

1. **Combat on a fine hex grid.** One hex is 5 ft. You move in and out freely (Stride / Step), which replaces the front-line / back-line layout.
2. **One hero for now.** Later the hero recruits companions on the road.
3. **Level cap 20, slow grind.** XP stays slow, so it's a long build-up.
4. **Uncommon and rare are in, with pf2e-style availability:**
   - **Settlements have a level,** for example Brindlemark is a level 2–3 village. Shops stock common items up to the settlement level. They rarely carry anything above it, and only from big towns or cities.
   - **Uncommon** items show up in a shop's stock only now and then, roughly 10–15% per restock, and more often in larger settlements. They also come from quest rewards, NPCs who grant access, and loot.
   - **Rare** items almost never appear in shops (about 2%, cities only). They come mainly from treasure tables, bosses and secret caves.
   - **Loot** follows pf2e's Treasure by Level budget: permanent items at level and level+1, consumables, currency. Rarity is weighted so that common items dominate.
   - **Data:** item level and rarity already come from the library. A settlement level and size goes on each town hex.

## Questions for the user (answered above)

1. Should combat run on a grid: hexes inside the arena, with movement in 5-ft steps? Or stay a front-line/back-line formation, with movement as an abstract "close / far" range?
2. Is party play wanted (several PCs, like your 6 level-4 characters), or one hero plus companions?
3. Should the level cap stay at 20, with XP following pf2e's 1000-per-level?
4. For character creation, should rarity follow pf2e (common free, uncommon and rare need GM access), or is everything unlocked?

## Overnight progress (2026-10-26, 12:36 am, no editor restart)

**Rules library rebuilt (12.1 MB, local only).** It now includes:

- **Rule elements** (`rules` arrays) on feats, class features, equipment, conditions and creature abilities. The most common keys give the build order for the C++ rules engine:

  | Rule key | Uses |
  |---|---|
  | ItemAlteration | 890 |
  | ActiveEffectLike | 671 |
  | GrantItem | 554 |
  | FlatModifier | 552 |
  | RollOption | 511 |
  | ChoiceSet | 261 |
  | Note | 238 |
  | AdjustDegreeOfSuccess | 131 |
  | DamageDice | 126 |
  | Strike | 114 |
  | BaseSpeed | 110 |
  | Resistance | 107 |

- **Build choices** (`choices`, 45), resolved from ChoiceSet `item:tag:` filters and UUID lists. Examples:
  - Magus hybrid study (8)
  - Sorcerer bloodline (18)
  - Witch patron (16)
  - Wizard school (14) and thesis (5)
  - Oracle mystery (11)
  - Druid order (9)
  - Barbarian instinct (10)
  - Champion cause (7)
  - Rogue racket (6)
  - Bard muse (5)
  - Cleric doctrine (3)
  - Kineticist gates
  - Inventor innovations
  - Thaumaturge implements
  - Psychic minds
  - Gunslinger ways
  - Swashbuckler styles
  - Investigator methodologies
  - Ranger edge
  - Alchemist field
  - Animist practice
  - Exemplar ikons and epithets

  Fighter, Monk, Guardian and Commander have no level-1 subclass, which is correct. **Summoner's eidolon still needs handling** (its choice uses a config/predicate form), and so does the Cleric deity (it comes from `deities`).
- **Bestiary** (662 creatures from Monster Core plus legacy Bestiary entries that weren't remastered). Each has level, rarity, size, traits, HP, AC, Perception, senses, speeds, saves, immunities, weaknesses, resistances, strikes (bonus, damage, traits, effects), abilities (with their rules) and spells.
- **Hazards** (54): Stealth, disable, reset, AC, HP, actions.

### Morning queue: needs the editor closed (header changes)

1. **`HollowLibrary.h`:** add `FLibChoice` (feature, class, level, flag, options), `FLibCreature`, `FLibHazard`, and a `Rules` field (raw JSON object array) on entries; parse them in `HollowLibrary.cpp`.
2. **Character builder wizard** (`HollowUI.h` + `HollowGameMode.h` state):
   - Steps: Ancestry → Heritage → Background → Class → Subclass (from `choices`) → Boosts (pf2e free-boost rules) → Skills → Feats → Spells → Gear → Review.
   - A pending-choices list blocks Confirm.
   - Replace the 4 hand-built kits with a data-driven `FClassBuild`; the kits become presets.
3. **Rules engine v1** (new `HollowEffects.h/.cpp`): apply FlatModifier, DamageDice, Resistance/Weakness/Immunity, AdjustDegreeOfSuccess, RollOption predicates (a simple subset), BaseSpeed and Note, from feats, items, conditions and creature abilities, to checks, attacks, damage and AC.
4. **Hex battle grid** (`FCombatState` / `FFighter` get hex coordinates):
   - 5-ft hexes on the arena floor; Stride and Step cost actions using Speed; reach; flanking gives off-guard; cover comes from the line-of-sight system.
   - Free orbit camera in combat: right-mouse drag plus Q/R, Shift for smooth free rotation.
   - Monsters come from the bestiary, scaled to the hex's level.
5. **Shops and loot by availability:**
   - a settlement level on town hexes
   - stock is common items up to the settlement level
   - uncommon items appear in roughly 10–15% of restocks
   - rare items roughly 2%, in cities only
   - loot follows pf2e's Treasure by Level

## What happens while you're away

The editor is open, so only Live Coding (`.cpp`-only) changes are possible.

- Export rule elements and subclass choices in `BuildLibrary.mjs`.
- Draft the creation-wizard data model in `.cpp`.
- Write the conditions engine as `.cpp` helpers.
- Commit and push the docs.
- Header changes wait for the morning restart.
