# Emberfall roadmap

## Saved for later: the Cathedral boss (Dark Souls-style set piece)
- **Stage:** a fixed, hand-built church interior, not an explorable hex. You walk the nave toward the altar, the doors seal behind you, and the arena is the transept.
- **One enemy, many mechanics:** a single boss with phases. For example:
  - Phase 1: sweeping melee and telegraphed slams.
  - Phase 2: summons and arena hazards (falling chandeliers, a violet rune floor).
  - Phase 3: the Star answers, and a desperation phase follows.
  
  Each phase has readable wind-ups, punishes greed, and leaves windows for your cards.
- **Uses what you carried:** your current character, deck, potions, holy water and items. No loadout screen, so preparation in the March matters.
- **Combat model:** the same PF2e + card rules, but with a boss timeline. It telegraphs its next 2-3 actions (intents) and can break your hand or tempo, plus positional hazards on a small grid inside the church.
- **Variants:**
  - **Secret caves:** entering a hidden cave can swap the normal event for one of these fixed boss stages instead (the same framework, a different arena and boss).
  - **Hollowmere Keep / the Choir's temple:** the final boss uses this format.
- **First milestone:** one sample boss (the Choir's Hierophant) in one church level, built end to end. Assets: cathedral kit (Fab), a boss model (Tripo), organ / choir music.

## Saved for later: a continuous world read through splat maps
- **One terrain gradient across the March,** driven by a splat map (biome weights per hex and per texel), so the land visibly changes as you move outward from Brindlemark:
  - Spooky lantern town.
  - Withering wilds: dead grass, then black soil and bone.
  - The eldritch cathedral and worship grounds.
  - **The Emberfall:** the impact site where the god is meant to land. Glassed earth, floating rock, violet light.
- **See the frontier from where you stand:** with a saved game, looking out from a hex shows neighbouring hexes you've unlocked as real terrain beyond your island's edge. Still-locked hexes appear as vague, nodal silhouettes in the fog, shapes that don't resolve into anything yet.
- **Implementation sketch:**
  - Generate a splat texture per hex from its terrain plus its distance and direction from the Emberfall.
  - A layered landscape material blends Megascans surfaces by splat channel.
  - Neighbour islands get low-detail impostor rings (HLOD-style), and locked ones are fog-card silhouettes.
  - Fog-of-war state persists in the save game.

## Near-term
- Translucent fog-of-war material (currently additive mist).
- Proper human models plus your animation set; replace the chibi stand-ins.
- Megascans terrain surfaces and cliff sides for the hex islands (see AssetWishlist.md).
- A warm town theme.
- Save/load, Pathbuilder import (ported from the web version), card evolutions using the frame sheet.
