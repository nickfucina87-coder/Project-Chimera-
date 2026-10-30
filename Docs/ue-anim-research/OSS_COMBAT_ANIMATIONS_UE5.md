# Open-source / open-licensed combat animations for UE5 (Unsoul)

_Researched 2026-10-28. Always re-check the license page before shipping._

## Short answer
**Limited, but real.** Most true open-source *games* (Veloren, 0 A.D., SuperTuxKart, Xonotic, etc.) are either low-poly/stylized, use non-humanoid or custom rigs, or ship art under **GPL / CC-BY-SA** (share-alike = trap for a closed commercial game). The useful stuff is **CC0 animation libraries and CC0 game templates**, which are commercial-safe. Expect good basics (sword combos, roll, dodge, hits, spell cast) — **not** Wukong-grade acrobatics or deep spear/bow sets. Fill those gaps with Fab free monthly packs, Mixamo, Epic's Game Animation Sample, and your own webcam mocap (see UE_ANIM_AND_MOCAP_OPTIONS.md).

## Candidates
| Name | Source | License | What anims | UE5 path | Commercial OK? |
|---|---|---|---|---|---|
| Quaternius Universal Animation Library (UAL1) | https://quaternius.itch.io/universal-animation-library | CC0 (verified on itch page) | 120+ (43 free): 8-dir locomotion, roll, jump, sword idle/attack, punches, spell enter/idle/shoot, hit chest/head, deaths | GLB export for UE5; rig uses UE5-mannequin-style bone names (65 bones) → IK Retargeter to Manny is quick. Root motion & in-place versions | **Yes** |
| Quaternius UAL2 | https://quaternius.itch.io/universal-animation-library-2 | CC0 (verified) | 130+ (43 free): 3–4 hit sword light/heavy combos with recoveries, sword dash/block, shield, melee hook, overhand throw, Hit_Knockback, slide, NinjaJump, climb | Same rig as UAL1 → same retarget setup | **Yes** (Pro $9.99 / Source $14.99 tiers also CC0) |
| Cat's Godot Souls-Like Template | https://catprisbrey.itch.io/cats-godot-souls-like-template · https://github.com/BlueBrainOrg/SoulsLikeDemo (fork) | Unlicense / CC0 per author; states no Mixamo content | 75+ anims: attacks, block/parry, dodge roll, sprint, ladder, ragdoll death; also good *reference* for a souls-like state machine | Godot project; export GLB/FBX from Godot or the source .blend → import to UE5 → retarget (standard humanoid rig) | **Yes** (credit appreciated) |
| atlas-09 / FORGE mechas | https://github.com/RamonLinares/atlas-09 | CC0 assets, MIT code | Mecha combat clips (combos, beam/cannon, backflip dodge, knockback/collapse) — partly derived from Quaternius | Non-human mecha rigs: use as-is for mech enemies; poor fit for humanoid retarget | **Yes** |
| Mixamo (not OSS, for comparison) | https://www.mixamo.com | Adobe royalty-free terms | Huge: flips, rolls, sword, staff, magic, hits | Retarget Mixamo→Manny (well-documented) | Yes in-game; **can't redistribute raw files** (don't commit to public repo) |
| Epic Game Animation Sample / Lyra / Fab free monthly | Epic launcher / Fab | Epic content license (UE-only) | Locomotion, traversal, some combat | Native UE5 Manny | Yes **inside Unreal** only |
| Veloren | https://gitlab.com/veloren/veloren | GPL-3 code; art mostly CC-BY-SA style | Voxel, procedural/custom animation in code | Not usable – not skeletal clips in a humanoid rig | **No** (share-alike + wrong format) |
| 0 A.D. | https://play0ad.com | Art CC-BY-SA 3.0 | RTS unit swings, bows, spears | COLLADA, low-detail RTS rigs; heavy retarget | **Trap**: share-alike on derived art |
| OpenGameArt.org items | https://opengameart.org | Mixed per item (CC0, CC-BY, CC-BY-SA, GPL) | Scattered rigs/anims | Varies | Only items marked CC0 / CC-BY |

## Top picks for Nick
1. **Quaternius UAL2** (pay $10–15 for full set) – best free/cheap CC0 sword combos, dash, block, knockback.
2. **Quaternius UAL1** – roll, jumps, spell cast loop, hit reacts, deaths; same rig as UAL2 = one retarget setup.
3. **Cat's Souls-Like Template** – extra CC0 dodge/parry/attack set plus a working souls-like logic reference to study (legally open).
4. **Mixamo** – fills flips, backflips, staff/spear-ish, magic; fine in a commercial game, just keep files out of public repos.
5. **Your own webcam mocap / Epic samples** – for Wukong-style staff/spear and acrobatics, nothing open-source matches; capture or buy (Mega Spear etc., see main doc).

## What NOT to use
- **GPL or CC-BY-SA** game art (Veloren, 0 A.D., many OpenGameArt items): share-alike can force you to open your assets/game.
- **CC-BY-NC** (non-commercial) packs, and "for personal/educational use only" Sketchfab/itch packs.
- **Engine-locked** licenses (Unity Asset Store, Epic/Fab content) outside their engine.
- **Rips** from commercial games posted on GitHub/Sketchfab/DeviantArt (e.g. "Souls/Wukong animations" models-resource dumps) – not licensed, regardless of what the upload says.
- Unknown-license GitHub repos (no LICENSE file = all rights reserved).

## Dead ends
- No open-source game found with a deep, high-fidelity spear/bow/acrobatics humanoid moveset under a commercial-safe license.
- Godot/Bevy combat demos mostly reuse Mixamo (can't redistribute) or Quaternius (just go to source).
- 0 A.D. / Veloren / license pages not individually re-verified this pass; treat as share-alike unless proven otherwise.
