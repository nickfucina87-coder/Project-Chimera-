# Mechanics research: installed Steam library → Hollowmere

Source: the 30 games installed under `C:\Program Files (x86)\Steam\steamapps` (2026-10-25).
Only the games with a mechanic that fits a top-down, turn-based, card-assisted PF2e hex crawl are listed.
Mechanics are studied from public play knowledge and reimplemented from scratch in our own C++, so no code or assets are lifted from these games.

| Game | Mechanic worth borrowing | How it lands in Hollowmere | Status |
|---|---|---|---|
| Vampire Survivors, Megabonk | Pick 1 of 3 random upgrades on level-up; two maxed items evolve into a stronger one | Post-battle card draft (1 of 3) and Pip's card packs. Next: **card evolution**, e.g. 2× Feint + a Rogue level = Twin Feint | Draft done, evolution planned |
| Phasmophobia, Demonologist | Speech recognition: the world reacts to keywords you say out loud | Talk system matches typed keywords to topics, and topics reveal routes. Next: push-to-talk voice input feeding the same matcher, then an optional Claude-backed NPC brain that returns topic tags | Typed done, voice planned |
| Core Keeper | Biome rings around a core; boss kills gate the next ring | Map rings: ring 1 open by talk, ring 3 holds the Keep. Next: each ring's mini-boss unlocks deeper leads | Rings done, gates planned |
| Project Zomboid | Positive and negative traits with point costs at character creation; "moodles" (stacking status effects) | Traits at creation (e.g. Night-Owl: +2 Perception at camp, −1 Will); moodles like Hungry, Tired and Frightened shown on the HUD | Planned |
| ARC Raiders, R.E.P.O. | Push-your-luck runs: go out, grab loot, extract before it goes wrong | Each step costs a ration and a day; loot caches in every hex; camping risks night ambushes | Done (basic) |
| Lethal Company | Quota deadlines that force risk | Optional "Varek grows stronger every 10 days" clock: his regen climbs | Planned |
| Blasphemous | Guilt or penance modifiers stacking on death | Optional "Scars": dying returns you to Brindlemark with a permanent card curse | Idea |
| Among Us, Funnel Runners, co-op horror titles | Social deduction, physics co-op | Not a fit for single-player combat; revisit if co-op is added | — |

## Combat mash-up (what we built)
- **FFX-style turn timeline.** Each fighter's delay is 1000 / Speed, so faster creatures act more often. Tempo cards (Quicken, Hamstring, Sudden Charge) bend the timeline, and the next 9 turns are always visible.
- **PF2e core.** Three actions per turn, a d20 against AC or DC, four degrees of success (crit at ±10, nat 20/1 shift a step), and the multiple attack penalty (0 / −5 / −10).
- **Cards as the sub-game.** Draw up to 4 cards each turn. Cards cost the same three actions as Strikes, so every turn trades a reliable swing against a trick. Auras (Bless, Battle Hymn, Riposte) persist like Queen's Blood board effects.
- **Visible enemy intents** (Slay the Spire style), so players can plan Guard, Shield or Riposte.
