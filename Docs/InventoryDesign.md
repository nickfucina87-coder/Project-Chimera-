# Inventory, field abilities, dialogue and pause screen

## What we learned from Nivalis Nights

Nivalis Nights is a Unity IL2CPP game, so its C# class and field names ship in plain text inside `global-metadata.dat`. We read them with REA's `inspect_il2cpp_metadata`. This was a static read of names only: no game code was decompiled or copied. Their inventory lives in `Nivalis.InventorySystem` and splits into four layers:

| Layer | Their class | What it holds | Ours |
|---|---|---|---|
| Item type | `ItemType` | Static data: guid, icon, prices, tags, flags such as `isEquippable`, `isIngredient` and `decayTimeInDays` | `FItemDef` |
| Stack | `ItemStack` (+ `ItemInstanceData`) | One type plus its instances. Per-instance data covers creation day, decay and freshness | `FItemStack` (count, charges, pickup serial) |
| Container | `ItemContainer` + `ItemContainerRestriction` | `_items` (display order), `_typeMap` (type to stack, for O(1) "do I have X"), a restriction (`MaxItems`, `MaxSlots`, `TagFilter`) and an `onContentsChanged` event | `FItemContainer` (slots + refused kinds) |
| Inventory | `PlayerInventory`, `InventoryData`, `InventoriesManager` | Money kept separately (`IMoneyContainer`). Several containers are addressed by id, and `GetDestinationContainerForItemType` routes new items to the right one | `FInventory` (pack, plus the bag of holding when carried) |

Other patterns worth copying:

- **Operations report how far they got.** `ItemCollectionOperationResult` is `Failed`, `Partial` or `Succeeded`, so "your pack is full, 2 left behind" falls out naturally (our `EAddResult`).
- **Saves store types and counts, not screen positions.** `InventorySave` keeps `PerTypeInstances`, `MaxSlots`, `MaxItems` and a `TagFilterGuid`. A container rebuilds itself from "type id and how many" plus its restriction. Items are identified by GUID, never by pointer or slot index. That is what "where things are" means in the binary: a lookup table from type id to stack, inside a container that has its own id.
- **Dialogue reads the inventory.** `ArticyGlobalInventoryLinker` mirrors item counts into articy:draft global variables. Conditions such as `PlayerHasItemChecker` gate choices, and instructions such as `ModifyItems`, `GivePlayerItem` and `TakePlayerItem` change the inventory from inside a conversation. Our `FTalkOption` does the same job (`NeedItem`, `bGive`, `GiveItem`).
- **Choices remember being explored.** `DialogueUiLinePartChoices.exploredChoices` marks lines you've already picked; ours dim. `ChoiceCondition.removeChoiceOnFalse` chooses between hiding a choice and greying it out. We grey it out and show the reason.
- **Dialogue can pause time.** `DialogueWithChoices.pausesTime` and `DialogueUILinePartPauser` stop the game during a conversation.
- **The in-game menu is one tabbed screen.** `InGameMenuTab` has Inventory, Journal, Characters, Skills, Achievements, Recipes, and so on. `WindowToggleController` swaps the panels, and `GamepadTabSwitching` cycles them.
- **Inventory UI.** A list plus a details panel with a separate "nothing selected" state (`PlayerInventoryUI`), with filters and sorts by name, last update, count, price and usage (`InventoryItemFilteringUi`).

## How Emberfall uses it

- **Pause screen** (Esc, I, C or J). The world really pauses (`SetGamePaused`), and the UI and input keep ticking. Tabs: Inventory, Character, Deck, Journal, System.
- **Inventory.** A 12-slot pack. A Bag of Holding adds a 16-slot container that refuses quest items. Filters by kind and sorts by kind, name or recent. Select an item, then click an empty slot to move it. Actions: Use, To bag / To pack, Drop.
- **Field abilities come from what you carry**, not from your class:

  | Ability | From |
  |---|---|
  | dig | shovel |
  | light | torch, lantern |
  | fire | torch, tinderbox |
  | cut | hatchet |
  | climb | rope and grapnel |
  | pick | thieves' tools |

  Torches have 3 uses each. The HUD shows which abilities you currently have.
- **Field spots** on wild hexes (2–3 per hex, persisted per hex):

  | Spot | Needs | Notes |
  |---|---|---|
  | Loose earth | dig | |
  | Bramble | cut or fire | |
  | High ledge | climb | or Athletics DC 18 |
  | Strongbox | pick | Thievery DC 15 +2, or Athletics DC 18 to smash it |
  | Black hollow | light | 30% chance a ghoul wakes |
  | Cold beacon | fire | lighting it shows the hexes within 2 on your map |

  A ring glows under a spot only while you carry something that can deal with it. That ring is the map overlay.
- **Dialogue box.** A letterbox, a portrait medallion, a Cinzel nameplate, and the line typed out in IM Fell English with [topics] in gold. Then numbered responses (keys 1–9), each with a tag chip:
  - skill check with DC and success chance
  - `Give:` / `Show:` for an item
  - silver cost
  - `Requires X` when locked
  - `New route` when it reveals a road

  Lines you've already heard are dimmed. Also: Trade, "Say something of your own..." (the free-voice LLM), Log, and Leave.
- **Trade window.** The vendor's wares on one side, your goods on the other. Valuables sell at 75%, anything else at 50%.
- **Level up.** Pick one ability boost (above +4 a boost counts as half, as in PF2e) and one skill increase (expert from level 3, master from level 7). Each level grants "Fresh Resolve": heal 5 HP at the start of your next fight. Buffs show as an aura on the hero: a ring, orbiting motes, a tinted light and the Fab buff Niagara.
- **Pathbuilder 2e import.** Paste the build id from Pathbuilder's Menu > Export > Export JSON, or the JSON itself, on the character screen. Level, ability scores, proficiencies, HP, AC, weapon, coin and recognised gear come across, and the gear brings its field abilities.

Fonts (OFL, Docs/Credits.md): Cinzel for display and labels, Alegreya for body text, IM Fell English for spoken lines.
