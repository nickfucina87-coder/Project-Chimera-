# 2D paper-style art: asset list

**The direction:** keep the 3D world, the hex islands, the rotating camera, zoom and the battle grid, but draw everything that stands up as flat 2D "paper" cutouts that always turn to face the camera, like Paper Mario. Combine that with a Diablo 2-style dark palette and a Zelda-like reading of the space. Ground, roads, water and hex tops stay as flat textured surfaces.

## How the sprites work in the engine

- **Billboards rotate on the vertical axis only.** A sprite turns to face the camera but never tilts, so it keeps standing on the ground when the camera pitches.
- **Facing:** each character needs a **front** (¾ toward camera) and a **back** (¾ away) view. The engine mirrors these for left and right, which gives four directions from two drawings. This is the Paper Mario approach. Diablo-style 8-direction sprites are optional; supply them later if you want them.
- **Held items are separate sprites** attached to a hand point, so one weapon picture works for every character.
  - Your single-angle weapon renders are fine as long as they share an orientation: blade or head pointing **up-right at about 45°**, grip at the lower left, with the grip point marked (or always at the same spot).
  - Swings rotate the weapon sprite in the picture plane, which is the Paper Mario feel.
- **Scale:** one battle hex is 5 ft and about **128 px across** at 1×. A human stands about 1.1 hexes tall, roughly 160–180 px of body inside a 256 px frame.
- **File format:**
  - PNG with transparency, straight alpha, no baked background or shadow (the engine adds a soft contact shadow).
  - **Pivot at the feet,** bottom centre. All frames of one character share the same canvas size and pivot.
  - Sprite sheets in a grid, or numbered frames (`wolf_walk_00.png` …).
  - Light comes from the top-left everywhere, one outline style, and a shared palette.
  - Optional: a **normal map** per sprite so torches and lanterns light the paper.

## 1. Hero (paper doll, layered)

| Layer | Need |
|---|---|
| Base body per ancestry | **MVP:** Human, Elf, Dwarf, Orc, Goblin, Halfling, Gnome, Leshy.<br>**Later:** the rest of the 49 ancestries, or reuse a base with recolours, ears and horns. |
| Outfit / armor overlays | Clothes, light (leather), medium (chain), heavy (plate), robes. Each must follow the body animation frames. |
| Head extras | Hair (5–6 styles × colours), hats and hoods (5), helmets (3). |
| Held items | Your weapon renders plus shields (round, kite, tower), staff, wand, lantern and torch (a lit flicker pair). |

**Animations** are needed for both the front and back view; frames per animation are in brackets:

- idle [4–6]
- walk [6–8]
- run [6–8]
- melee swing [5–6]
- thrust [5]
- ranged draw / shoot [5]
- cast (one-hand and two-hand) [6]
- hit / flinch [3]
- guard [2]
- dodge / step [4]
- interact or dig [6]
- death [6–8]
- victory [4]

## 2. NPCs

- **Named townsfolk:** Maren Holt, Tobb Alderwick, Sister Ysolde, Pip Quickfingers. Each needs idle and talk-gesture frames [4 each], front only.
- **Your Foundry world NPCs** as needed: Brother Casimir Holt, Father Aldren Vale, Innkeep Rova, Kess the Hunter, Old Hobb, Marta Gell, Hadrik Mune, the Lantern-Bearers and so on.
- **Dialogue portraits:** bust, about 512×512 or 600×800, with 3–4 expressions (neutral, pleased, angry, afraid) for each talking NPC. These replace the initial-letter medallion.
- **Generic villagers:** 4–6 bodies × recolours, with idle and walk.

## 3. Monsters

Each monster needs idle, move, attack, hit and death; casters also need a cast frame. Front and back views.

- **MVP:**
  - Grey Wolf, Wolf pack leader
  - Skeleton Monk
  - Masked Bandit
  - Ghoul
  - Drowned Knight
  - **Varek, the Hollow Lord** (boss, bigger frame, plus a phase-2 look)
- **Next:** ten favourites from the bestiary (goblins, kobolds, zombie, giant rat, spider, cultist, will-o'-wisp, ogre, wight, eldritch thing for the Choir).
- Size classes: small 192 px, medium 256 px, large 384 px, huge 512 px frames.

## 4. Items and icons (inventory, shops, cards)

- **Icons** at 128×128, front-on, one light direction, on transparent backgrounds.
- **Our items:**
  - healing potion, trail ration, holy water
  - shovel, torch, tinderbox, hooded lantern, hatchet, rope & grapnel, thieves' tools, bag of holding
  - wolf pelt, grave trinket, star-glass shard, Odo's bone charm
- **Generic pf2e categories,** so every library item has a picture: one icon per weapon group (sword, axe, club, flail, hammer, knife, pick, polearm, spear, bow, crossbow, firearm, sling, dart, shield, brawling). Your weapon renders can double as these.
  - armor: unarmored, light, medium, heavy
  - consumables: potion, elixir, scroll, talisman, oil, bomb, ammunition, snare
  - worn items: ring, amulet, cloak, belt, boots, gloves, headband
  - other: staff, wand, gem/treasure, coins, book, tool kit, food

## 5. Environment (cutouts that face the camera, plus ground)

**Cutouts:** pine, autumn tree, dead oak, shrub, reeds, grass tuft (3–4 variants each); boulders and rocks (5); ridge and cliff face; waterfall flipbook.

**Town (Brindlemark):**
- house fronts (5), shown as layered flat facades in a diorama look
- tavern, temple, crypt
- the tentacle fountain, the Fallen Shard, rune obelisks
- banners, lanterns, fence segments, well, market stall, notice board, graves

**Wild set pieces:**
- ruined arch, pillars, broken wall
- ritual circle (as a ground decal), wagon, bones, campfire flipbook, tent, signpost, chest, barrow door
- shrine, keep gate, the Sleeping Star in the sky

**Field spots:** loose-earth mound, bramble thicket, high ledge, strongbox, cave mouth, cold beacon and lit beacon flipbook.

**Ground:** a top texture for each terrain (forest, plains, hills, swamp, ruins, river, town, keep, shrine), plus road, water, mud, and the hex rim edge.

**Atmosphere:** fog cards (3), god-ray strip, fireflies and embers (sprite pairs).

## 6. Battle

- **Hex overlays** (move, target, you, threat zone, cover), each 128 px, tintable white.
- **Condition icons, 48 px.** The 43 pf2e conditions, MVP first: frightened, off-guard, prone, slowed, quickened, sickened, clumsy, enfeebled, stupefied, drained, grabbed, restrained, stunned, persistent damage, dying, wounded, concealed, hidden.
- **Action glyphs:** 1, 2 and 3 actions, reaction, free action.
- **Damage-type icons:** fire, cold, electricity, acid, sonic, force, mental, poison, vitality, void, bludgeoning, piercing, slashing, spirit.
- **Turn-order chips:** head icons for the hero and each monster, about 64 px.

## 7. Effects (sprite-sheet flipbooks, 8–16 frames, 512 px)

- weapon slash (3 variants), impact, critical
- fire burst and projectile, lightning arc, frost, acid splash, force bolts, mental/void swirl, vitality/heal glow
- buff aura loop, debuff loop, level-up flare, teleport in/out
- footstep dust, blood/ichor puff, coin sparkle, loot pickup, dig dirt spray, bramble burn, beacon flare

## 8. Interface

- **9-slice panels:** parchment, leather, dark stone. Buttons in four states.
- **Card frames:** one per kind (Attack, Skill, Spell, Tempo, Aura), plus card art (about 300×200) for each starter card.
- **Field-ability icons:** dig, light, kindle, cut, climb, pick lock.
- **Stealth readout icons:** bright/dim/dark plus seen/concealed/hidden.
- **Map:** a top-down illustrated hex tile per terrain, fog-of-war overlay, trail marker, hero token.
- **Other:** title logo (EMBERFALL), cursor set (default, talk, attack, move, use), HUD bars, frame for the turn order.

## Smallest set to see the new look in game

1. The hero: one ancestry, one outfit, front and back, with idle, walk, swing and hit.
2. A longsword from your renders, set up as a held item.
3. The Grey Wolf with idle, move, bite, hit and death.
4. Five props: pine, boulder, house front, lantern, chest.
5. A grass and dirt ground texture.
6. Twelve inventory icons from the list in section 4.
7. Five condition icons, the hex overlays, and a slash and an impact flipbook.

With that set I can build the billboard system (sprites facing the camera, a mirrored back view, a held-item socket and flipbook animation), and swap everything else in as it arrives.
