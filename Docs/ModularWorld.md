# Modular land and buildings (lessons from Project Zomboid), kept on our hexes

## What Project Zomboid does

This comes from reading the installed game's data files (`media/maps`, `media/*.tiles`). I didn't decompile any code or copy any art.

- **The world is a grid of cells.** Each town folder holds about 4,000 `world_X_Y.lotpack` + `chunkdata_X_Y.bin` pairs, with a `.lotheader` per cell. Each cell is a square block of 1 m squares, streamed in chunks, with up to 8 floors.
- **Each square holds a stack of tiles.** One square can carry a floor, a wall on its **north** edge, a wall on its **west** edge, a door or window in either edge, furniture, and overlays (grass, grime, cracks, snow). Walls only ever sit on the N or W edge, so every wall belongs to exactly one square and nothing doubles up.
- **Tiles are sprites in tilesheets.** There are 425 tilesets in `newtiledefinitions.tiles`. Each tile is a cell on a sheet (`file = fixtures_doors_01, size = 8,16`) plus a list of properties. The engine reads those properties, not the picture. The most common ones:
  - `wall`, `WallN`/`WallW`, `WindowN`/`WindowW`, `DoorWallN`, `Facing`: where it sits and which way it faces
  - `solid`, `solidtrans`, `BlocksPlacement`, `IsLow`: what blocks walking, sight or building
  - `IsMoveAble`, `PickUpWeight`, `PickUpTool`, `PlaceTool`, `CanScrap`, `CanBreak`: picking up, scrapping, breaking
  - `Material`, `MaterialType`, `BurntTile`, `SnowTile`, `GrimeType`: what it is made of, and its burnt, snowy and dirty variants
  - `ContainerType`, `Surface`: holds loot, or you can put things on it
  - `lightswitch`, `lightR/G/B`, `streetlight`: light sources
  - `SpriteGridPos`, `GroupName`, `CustomName`: multi-square furniture and display names
  - `WestRoofT` and friends: roof pieces
- **Buildings are rooms.** Cell headers list buildings as named rectangles (kitchen, bedroom, garage). Loot tables, ambience (`roomtones.lua`) and spawning key off the room name.
- **Zones are rectangles over the land.** `objects.lua` lays out `TownZone`, `Forest`, `Vegitation`, `Nav`, `Farm` and so on. They drive spawning, foraging, erosion and navigation.
- **The world changes over time.** `tiledefinitions_erosion` swaps tiles for overgrown or snowy versions.
- **The same tiles power player building.** Carpentry recipes place the same wall, floor and door tiles the map is made of.

Their tools for this (TileZed / BuildingEd / WorldEd) are built on **Tiled**, the free, open-source map editor.

## Fitting it onto Emberfall's hexes (the hexes stay)

| Layer | Zomboid | Emberfall |
|---|---|---|
| World | grid of cells | **the March hex map, unchanged** |
| Cell | 256×256-square block | **one hex island** = one cell |
| Square | 1 m square, 8 floors | **5-ft square** inside the island, clipped to the hex outline, 1–3 floors |
| Battle | — | the existing **5-ft battle hexes**, which read blocking and cover from the squares underneath |
| Building | walls on the N/W edges of squares | the same: walls, doors and windows on N/W square edges |
| Rooms and zones | named rectangles | named rectangles in a hex's lot data: loot from the pf2e library by room type and hex level, encounters and foraging by zone |

Squares inside a hex keep buildings sane (right angles, real walls). The hex stays the unit of travel and the battle board. Because the battle grid already does line-of-sight traces, walls built from squares give cover and block sight with no extra work.

**Lots (prefab buildings),** as Zomboid uses for random houses:
- Author a building once as a small square-grid "lot" (floors, walls, doors, rooms, furniture). Use Tiled with our own tilesets, or later an in-game editor.
- Export it to JSON. The island builder stamps lots onto the hex: town hexes get several, wild hexes get a ruin or a farmhouse.
- The same lot files become the player-building system: place a wall square, a door square, a bed.

**Because our camera rotates,** walls can't be fixed isometric sprites like Zomboid's. They become **flat painted panels on real geometry**: a 5 ft × 10 ft wall quad with a painted facade. Floors are painted tiles, and furniture and props are camera-facing paper cutouts. That keeps the paper look from every angle.

## Code to write (once the art spec is agreed)

1. **Tile definitions:** a small JSON file, like Zomboid's properties, listing each tile's art, edge, solid, blocks-sight, material, container, light, breakable, burnt variant and snow variant.
2. **Lot format:** squares with stacks (floor, wallN, wallW, object, overlay), plus named rooms and zones. JSON exported from Tiled (`.tmj`) or our own.
3. **Island builder:** stamp lots onto the hex, then generate the island's own squares (paths, fields, ruins) from terrain and seed.
4. **Rendering:** floors as instanced quads, wall panels on N/W edges, cutouts for objects, and cutaway (hide roofs and the walls nearest the camera when you're inside), building on the occlusion hiding that already exists.
5. **Gameplay hooks:**
   - containers loot by room
   - doors open and lock (thieves' tools)
   - breakable walls and doors (the battle grid uses the same blocking)
   - burnt and snowy variants
   - lights from `light` tiles feeding the stealth system
6. **Building:** carpentry recipes that place the same tiles, with costs from the item library.

## Asset list for the art

### Spec for every tile

- **Square** = 5 ft. **Floor tile** = 256×256 px, seamless.
- **Wall panel** = 256 px wide × 512 px tall (5 ft × 10 ft), painted straight-on (front view, no perspective), with transparency where a window or door opening is.
- **Furniture and props** = front and back cutouts, pivot at the feet. The scale is the same as the characters (human ≈ 170 px).
- Light comes from the top-left, with one outline style and the shared palette.
- **Naming:** `<family>_<material>_<variant>_<state>.png`, for example `wall_plaster_window_broken.png`.

### 1. Floors (seamless 256 px)

- **Inside:** wood planks (2), stone flags, flagstone with straw, carpet/rug (3), tiled temple floor.
- **Outside:** grass (3), dirt, mud, cobble road, gravel, sand, swamp water edge, fen moss.
- **Blend edges** (grass to dirt, dirt to road): straight and corner pieces.
- **Overlays:** cracks, grime, blood, leaves, snow cover.

### 2. Walls (256×512 front panels), per material

**Materials:** timber & plaster, rough stone, dressed stone (temple), log, brick, ruined stone.

**Each material needs:**
- plain
- window (open)
- window (shuttered or boarded)
- door frame (empty)
- half-height / low wall
- broken
- burnt
- interior face (plain plaster or wood)
- **corner post / trim**

### 3. Doors and windows

- **Doors:** wood door (closed, open, locked look, broken), iron-banded door, gate (wood, iron), temple double door, cellar hatch.
- **Window inserts:** glass, broken glass, shutters (open, closed), bars, curtains.

### 4. Roofs

- Thatch, slate and shingle tiles (seamless), plus ridge, eave edge and gable-end pieces.
- Chimney, and a roof hole (damaged).

### 5. Stairs and levels

Wood stairs (straight, 3 steps per square), stone stairs, ladder, railing, balcony edge, trapdoor.

### 6. Furniture (front + back cutouts, "searched/empty" state where it holds loot)

- **Bedroom:** bed, bedroll, wardrobe, chest
- **Tavern and kitchen:** table (small, long), chair, bench, stool, hearth/fireplace, stove, counter/bar, barrel, keg, shelves, crate, sack pile
- **Temple:** altar, pew, candle stand, lectern, offering bowl, statue
- **Workshop:** anvil, forge, workbench, grindstone, loom, bookshelf, desk
- **Graveyard and crypt:** sarcophagus, coffin, urn, bone pile

### 7. Outdoor props (cutouts)

- **Fences:** wood (straight, corner, gate, broken), iron (straight, gate).
- Well, cart, wagon, hay bale, crops (3 growth stages), scarecrow, signpost, lamp post (lit, unlit), market stall, grave markers, woodpile, troughs.

### 8. Lots to design first (buildings as square grids)

cottage, tavern (2 floors), temple, smithy, farmhouse, barn, crypt, watchtower, ruined chapel, mill.

The pack's `Town_Buildings`, `Walls_Gates_Landmarks`, `Roads_Foundations` and `Town_Props` already cover some of this: whole-building art can be cut down into wall panels and props.

### 9. States and seasons

Burnt and broken versions of walls, doors and furniture; snow caps for roofs, walls and props; moss and vine overlays for ruins (our version of Zomboid's erosion).

## Smallest set to see a building on a hex

1. One floor (wood planks) and one outdoor ground (grass).
2. One wall material (timber & plaster): plain, window, door frame, corner post.
3. One door (closed and open).
4. One roof (thatch) with ridge and eave.
5. Five furniture pieces: bed, table, chair, chest (closed and open), hearth.
6. A wood fence: straight and gate.

With that I can build the lot format, the island builder and one cottage you can walk into, search and fight in.
