# Optional aid — modular town kit path (Nick → Mike)

**Status:** Optional reference only. Mike owns Azeroth Reborn. Use this kit only if you want small buildings/huts; do not treat this as the project identity or required pipeline.

## What this is
Nick’s Dark Fantasy Village Tripo modular town pieces (walls, openings, thatch roof bits, paths, props) already generated for an earlier village-kit experiment. Shared so you can snap little buildings if useful.

## What this is not
- Not a mandate to build Azeroth Reborn as a modular village
- Not lore/canon for Azeroth Reborn
- Not Claude’s full Unreal project (VooDoo1 offline when this was written — no BP/PCG dump yet)

## How Nick built the pieces (mechanics so far)
1. **Break a small fantasy hut into modules** — solid wall, half wall, doorway wall, window wall, foundation, door, shutters, thatch roof/eave, ridge cap, gable, chimney, post, stair, path pieces.
2. **Reference art** — front + side (or front + mirrored back for solid pieces) on empty background; one object per image; never upload a combined front|side sheet as one Tripo image.
3. **Tripo settings (Nick’s lock)** — Multi-view when front+side exist; Smart Mesh; ~10–15k polys; 4K texture; one generate + one texture pass (no stacked HD/ultra). Openings: front+side only (leave back empty so openings stay open). Solid walls: front + mirrored back so they stay solid.
4. **Export** — FBX + maps into a kit folder (`tripo_modular_village/` in this pack).
5. **Unreal / Opus target (intent)** — after Tripo, straighten join edges, set shared dimensions and bottom-corner pivots on a ~50 cm grid in Blender so pieces can snap in Unreal (Opus or hand placement). Exact Claude UE wiring was not on this box when packed; Phase 2 can add Diablos Content path notes when VooDoo1 is online.

## Suggested use (optional)
- Grab 3–4 wall pieces + door + thatch roof for a Barrens hut block-in
- Use Poly Haven dry_ground / rocky_trail for arid ground under it
- Ignore the rest of the kit until you need it

## License note
Tripo pieces: Nick’s paid commercial Tripo plan — for Azeroth Reborn co-dev aid with Mike; don’t resell as a standalone asset pack. Poly Haven: CC0. Fab: not in this pack.
