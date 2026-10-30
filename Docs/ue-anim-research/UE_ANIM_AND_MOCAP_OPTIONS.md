# UE5 Animation & No-Suit Mocap Options — for Unsoul / Diablos
_Researched 2026-10-28. Prices and license tiers change, so check each listing's Fab "License terms" before you ship. No game assets were ripped. Everything here is a marketplace, an official, or an open source._

## Executive shortlist (ranked)
| # | Pick | What it solves | Cost / license |
|---|------|----------------|----------------|
| 1 | **MetaHuman Animator Markerless Motion Capture** (UE 5.8+, built in) | Webcam/phone video → full-body animation inside UE, no suit | Free, commercial OK (Windows only) |
| 2 | **Game Animation Sample Project** (Epic) | 500+ AAA locomotion/traversal anims + Motion Matching base | Free, retarget/commercial use allowed |
| 3 | **ProcHitReact** (Vaei, GitHub) | Procedural physics hit reactions that depend on hit direction and bone | Free, UE 5.3+, beta |
| 4 | **Mega Spear Animation Pack** (Fab) | 219 spear anims, movement + attacks | Free on Fab (UE4 skeleton → retarget) |
| 5 | **Mixamo** | Huge free library: flips, rolls, dodges, hit reacts, casts | Free, royalty-free for commercial games. You can't redistribute the raw files |
| 6 | **Mage Collection Samples – Free** + **Archery Animation Pack** (Fab) | Energy blasts from the hands + bow shots | Free |
| 7 | **Paid upgrade:** Essential Magic Staff / Spear Animation Packs (space.bar.anim), Multi-Weapon Acrobatics, Rare & Advanced Parkour, Hitreact Pro | Wukong-tier staff, acrobatics and directional hit coverage | Paid, Fab Standard License |

---

## 1. Webcam → Unreal, no mocap suit
| Tool | Live or recorded | UE version | Difficulty | Cost |
|------|------------------|------------|-----------|------|
| **MetaHuman Animator Markerless Mocap plugin**: Live Link Hub → Capture Manager → Mono Video Ingest | Recorded mono video / webcam takes, processed locally | 5.8+ (Windows) | Easy–Medium | **Free, commercial OK** |
| **Kinemotion** (GitHub, vicvasper) | **Live** webcam → NNE pose model → Live Link 19-bone skeleton | 5.3+ | Medium (hobby plugin, jittery) | Free. Check the repo license |
| **Rokoko Vision 3.0** (Rokoko Create) | Upload video → FBX/BVH (UE5 Manny/Mixamo skeletons) | Any (FBX) | Easy | Free 30 s/month. Basic plan $10/mo for 600 s. Commercial OK. Live webcam mode was removed |
| Move.ai (Move One), QuickMagic, DeepMotion, Plask | Upload video → FBX | Any | Easy | Freemium or paid. Check each one's commercial terms |

**Recommendation:** UE 5.8 Markerless plugin first, because it's free, local, and lives in the engine. Then retarget to Unsoul with IK Retargeter and clean up in Sequencer/Control Rig. Fast flips are where single-camera AI breaks down, so use it for custom attacks, poses and taunts, and get the acrobatics from packs.
- Docs/news: https://www.cgchannel.com/2026/06/get-the-free-metahuman-animator-markerless-mocap-plugin/ · https://digitalproduction.com/2026/06/23/metahuman-gets-markerless-mocap/ · tutorial https://www.youtube.com/watch?v=b2i1aZbhxAU · https://addons.cgdive.com/tools/unreal-engine-markerless-mocap
- Kinemotion: https://github.com/vicvasper/Kinemotion_UE5.5-
- Rokoko Vision: https://www.rokoko.com/products/vision · https://create.rokoko.com/

## 2. Animation packs by move type
### Base / locomotion
- **Game Animation Sample (Epic, free):** https://www.fab.com/listings/880e319a-a59e-4ed2-b268-b32dac7fa016 · blog: https://www.unrealengine.com/blog/game-animation-sample
### Spear / staff / polearm (core of the Wukong feel)
- **Mega Spear Animation Pack (FREE):** 219 anims across 10 stances. https://www.fab.com/listings/a6f8de4a-b0a3-4b9b-8afc-bd6f1bf20bfa
- Essential Magic Staff Animation Pack (paid): 535 anims + 48 aim offsets, dodge/roll/dash, UE 5.0–5.7. https://www.fab.com/listings/0c633568-6fd6-429c-8746-5285c7a4b84c
- Spear Animations Pack (paid): air combos, 8-way dodge/roll, executions, knockdown/getup. https://www.fab.com/listings/02b4b496-4591-4cee-8970-e28c08026eae
- Spear Combat Animation V1 (paid, GDH): 170 anims, root motion + in-place. https://www.fab.com/listings/50dadbed-60d7-4940-a737-f512fb3803bd
- Essential Spear And Shield (paid): https://www.fab.com/listings/6278c094-b323-4173-9308-960737a7ef4e
### Bow / archery
- **Archery Animation Pack (free):** 6 mocap shots with fingers, on UE5 Manny. https://www.fab.com/listings/cbff5605-c8ac-4a1c-b368-5a9ced5e33f6
- Mixamo also has archer draw/shoot/idle sets.
### Energy blasts / hand magic
- **Mage Collection Samples – Free (Rapa Motion):** SuperBlast, Fire Volley, conjures, power-ups. https://www.fab.com/listings/78f85a52-4132-4ce1-bb87-0c9e3a45a42a · https://rapamotion.itch.io/magesamples
- **Human Spellcasting Animations FREE (Kevin Iglesias):** 90 files, one- and two-hand casts. https://kevdev.itch.io/human-spellcasting-animations-free (one user reports a male-model issue, so test it first)
### Gymnastics: flips, dodge, duck, roll
- **Mixamo (free):** backflip, front flip, side flip, roll, dodge, duck, kip-up. https://www.mixamo.com
- Rare & Advanced Parkour Animations (paid): 80+ flips, dives, rolls and air tricks. https://www.fab.com/listings/2b309c11-6273-4586-a36c-904ef4aeed07
- Ragdoll System & Backflip Animation Pack: 15+15 backflips. https://www.fab.com/listings/bd3fa245-f10c-4bbf-b975-33d444549085
- Acrobatic Combat Animations: https://www.fab.com/listings/10c84b52-43b8-45e4-9a10-fc137f49500e
- **Multi-Weapon Acrobatics:** 101 weapon + acrobatic anims, closest to the Wukong style. https://www.fab.com/listings/d90c8180-4334-4dbf-b23d-747fd913ef4d
### Impact hits / multi-hit / stagger
- Mixamo hit reactions (front/back/side, knockdown) are free.
- **Hitreact Pro:** 127 directional and body-part reactions plus a component. https://www.fab.com/listings/dd1558d5-ce68-4e93-a931-954edd89d974
- Most of the spear and staff packs above include hit, knockdown and getup sets.

## 3. Directional "kinetic" hit + ragdoll
The ragdoll can know the angle it was hit from. You compute it from the hit result: take the dot/cross product of the attacker direction against the actor's forward and right vectors to get a front/back/left/right sector, and read the hit bone name to get the body part. Then layer:
1. **Directional anim pick:** play a montage chosen for that sector and bone (Hitreact Pro, or Mixamo clips in a Chooser).
2. **Physical Animation Component** (built in, free): blend the physics weight on the hit bone chain and apply an impulse at the hit location along the hit direction.
3. **Physics Control Component** (built in, UE 5.1+ plugin): newer and more stable per-bone control. See Epic's Content Examples → Physics Control map. https://www.youtube.com/watch?v=audSnXP4qlE · BP example https://blueprintue.com/blueprint/4gcw794l/
4. **ProcHitReact (FREE):** ready-made procedural version of this, multiplayer-ready. https://github.com/Vaei/ProcHitReact/ · https://www.reddit.com/r/unrealengine/comments/1je8e96/free_procedural_hit_reaction_system_prochitreact/
5. **Physics-Based Hit Reactions System (Fab, paid):** Blueprint-only, built on Physics Control, per-bone directional impulses. https://www.fab.com/listings/0f0f63e5-5a85-434a-ae14-60c7120054f0
6. Multi-hits: queue the impulses, add stagger accumulation (poise), and switch to full ragdoll once a threshold is crossed. Get back up with a pose-snapshot blend into a getup montage.

## 4. Suggested stack for Nick
1. UE 5.8 project. Start from **Game Animation Sample** (Motion Matching locomotion) and retarget it to Unsoul with IK Retargeter.
2. Combat: **Mega Spear (free)** for the staff/spear base, plus **Mixamo** for flips, rolls, dodges and hit reactions, plus **Mage Samples** and **Spellcasting FREE** for energy blasts, plus the **Archery** pack.
3. Hits: **ProcHitReact** or Physics Control for directional physical reactions, layered over the directional Mixamo hit montages.
4. Webcam: **MetaHuman Markerless** for custom signature moves and taunts. Clean them up in Control Rig.
5. Paid upgrade when the budget allows: Essential Magic Staff Pack → Multi-Weapon Acrobatics → Hitreact Pro → Rare & Advanced Parkour.

## 5. License notes
- **Fab Standard License:** commercial use and retargeting OK. Personal tier covers ≤ US$100k revenue in the last 12 months. You can't redistribute the raw assets or let players extract them, and you can't use them for AI training unless the listing says "Allows usage with AI: Yes". Summary: https://awesomedog.com/mocap/licensing/
- **Mixamo:** royalty-free in commercial games. Don't redistribute or sell the raw animation files.
- **Epic samples:** free to use in UE projects.
- **GitHub plugins:** check each repo's LICENSE file before shipping.
- **itch.io packs:** read each page's license. Kevin Iglesias and Rapa Motion present theirs as royalty-free.
- **GitHub repo warning:** keep this doc and your own code public if you like, but never commit Fab or Mixamo files to a public repo. That counts as redistribution. Use a private repo or Git LFS, and .gitignore the asset folders.
- Never rip assets from Black Myth: Wukong or any other game. Use them only as visual reference.
