# Asset wishlist: grab these and drop them in `ExternalAssets/` (or tell Claude where they landed)

Target look: `Docs/Concept/hex_wilds_concept_v2.png`. That's a dark, grounded fantasy hex diorama: cliff-sided island, misty river and waterfall, ruined castle with torches, dense pines and autumn trees, a lone caster with a blue light. Cosmic-horror accents come from the Sleeping Star.

Already in the project (all CC0, see `Docs/Credits.md`): Kenney Graveyard, Fantasy Town, Nature, Castle kits; Kenney RPG / Impact / Interface audio and Music Jingles; Kenney Fantasy UI Borders; Kenney Particle Pack; two CC0 dark-ambient loops from OpenGameArt.

## 1. Fab (you download; in the editor open Window > Fab, add to library, then "Add to project")
Free and Quixel Megascans items are what will move us from "toy" to "concept art":
- [ ] **Megascans surfaces:** mossy forest floor, muddy path, wet cobblestone, mossy rock cliff, forest litter / pine needles. These are for the hex tops and the cliff sides of each island.
- [ ] **Megascans 3D:** mossy boulders (3-5 variants), cliff faces, tree stumps, dead branches, ruined stone walls and arches, broken pillars.
- [ ] **Megascans foliage / Fab free trees:** European spruce / pine (dark), birch or beech in **autumn orange**, ferns, dry grass clumps. The concept leans hard on spruce + orange autumn trees.
- [ ] **Free fantasy packs on Fab:** any free "medieval village", "modular medieval town" or "castle ruins" kit in a realistic-stylized look. Medieval Village / town packs from the monthly free Fab rotation are ideal.
- [ ] **Water:** UE's built-in Water plugin is already in the engine (for the river and waterfall). Nothing to download; tell Claude to enable it.
- [ ] **Fog / VFX:** free Niagara packs (fire, embers, mist, magic glyphs), e.g. Fab's free monthly VFX packs.

## 2. Tripo (generate; export FBX or GLB, ~10-20k tris, PBR textures)
Characters (A-pose, for later Mixamo rigging):
- [ ] Hero variants: hooded caster with a staff topped by a blue crystal (concept), armored dwarf fighter, rogue in a dark leather hood, cleric with a star-pierced halo.
- [ ] Townsfolk: Maren (retired gate captain, grey cloak, old breastplate), Tobb (stout innkeeper, apron), Sister Ysolde (hooded priestess, violet stole embroidered with a many-pointed star), Pip (goblin trader with an overloaded backpack).
- [ ] Foes: grey wolf (lean, mangy, red eyes), masked bandit, skeleton monk, ghoul, drowned knight (barnacled armor, weed), **Varek the Hollow Lord** (gaunt noble, cracked crown, star-shard in chest).

Props and landmarks:
- [ ] **The Fallen Shard:** black obsidian monolith with glowing violet runes.
- [ ] **Starfall Altar:** stone altar with a floating star-orb.
- [ ] Ruined castle tower with an arched doorway and brazier; stone bridge; rickety wooden bridge.
- [ ] Gnarled dead oak, broken pilgrim wagon, giant ribcage, ritual circle stones.
- [ ] **The Sleeping Star:** a huge celestial body with tendrils and a corona, for the sky.

## 3. Animations
- [ ] **Mixamo** (free with an Adobe account, OK for commercial games): idle, walk, run, attack (sword, staff), cast, hit, death. Upload the Tripo characters, download FBX "with skin".

## 4. Audio (needs a human ear; CC0 or CC-BY with credit)
- [ ] **Warm town theme:** lute or folk loop for Brindlemark so it feels like home. Search OpenGameArt "medieval town" / "tavern" (CC0), or Tallbeard / Alexander Nakarada (CC-BY).
- [ ] Night ambience: crickets, wind, distant wolves, creek / waterfall loop (freesound.org CC0 filter).
- [ ] Combat stingers: card swish, magic whoosh, heavy hit, crit shimmer.

## 5. UI and fonts
- [ ] Fonts (Google Fonts, OFL licence): **Cinzel** (titles), **IM Fell English** or **Alegreya** (body). Drop the .ttf files in `ExternalAssets/fonts`.
- [ ] Card frame art: ornate gold/iron card border PNG with a transparent centre, plus per-kind card backgrounds (Attack / Skill / Spell / Tempo / Aura).
- [ ] Parchment and dark-leather panel textures (seamless), and a menu background painting (a wide shot of the concept, for the title screen parallax).

## 6. Parallax / backdrop layers (for the title screen and battle backdrops)
- [ ] 3-5 layer painted backdrop: sky with the Sleeping Star, far mountains, mid forest silhouette, near fog, foreground branches. Each layer is a transparent PNG at 3840 px wide.
- [ ] A battle backdrop per terrain (forest, swamp, ruins, keep) in the same layered format.

## 7. Optional
- [ ] An Ollama model better at role-play than llama3.2 3B, e.g. `ollama pull qwen2.5:7b` or `llama3.1:8b`. The game picks it up automatically.
