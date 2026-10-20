# CHM-005 Environment Sources: Research Notes (2026-10-20)

Research only. Nothing was downloaded or bought, and no Unreal project files were touched.

## Method
- I read the repo docs: MASTER_GAME_DESIGN.md, ASSET_MANIFEST.csv (ENV-001..007, INT-001..008), ASSET_MANIFEST_GUIDE.md (blockout guidance lives here; there is no separate blockout file), and issue #5.
- The GDD says the **exact UE version still needs verification**. Every row records the listing's stated range so we can filter once a version is locked.
- Specs come from Fab listing pages and Fab search pages (for prices), plus vendor pages where noted. Fab's API is behind Cloudflare, so some technical-detail blocks didn't render. Those fields say "not stated".
- Rows whose URL is a Fab *search* page are listings I saw in search results but didn't open. Open them before buying.
- Sale prices were observed on 2026-10-20 and can expire.

## Top pick per area
| Area | Pick | Price |
|---|---|---|
| Bunker | Modular Bunker (Trapdoor Studios) | from $64.99 (UE 4.16-5.7, collision yes, animatable doors) |
| Data center | Jobsite VOL.15 Tech and Data (Dekogon) + Tripo rack internals | $23.09 sale |
| Data center (alt) | Data Center Modular Kit (LAYA) | $69.99 (collision + LODs stated) |
| Warehouse | Modular Industrial Catwalks Set + Industrial Hall (Dexsoft) | $14.99 + ~$29.99 |
| Power/Comms | Gen Corp (Dexsoft) + Rusty Electrical Switchboards (hinge pivots) | $19.99 + price not stated |
| Damage | Decals VOL.8 Urban Decay + Broken Concrete bundle + Spline Cable Kit Industrial Vol.02 | $11.99 + $24.99 + $20.99 |
| Materials | Poly Haven / ambientCG | free (CC0) |

## Gaps that need custom Tripo geometry
- **Harvestable rack internals** (PSU, logic board, fan module; INT-001..003). No kit says it ships separately detachable components.
- **Robotics workshop machines and salvageable actuators** (ENV-005/INT-008). The Industrial Robot Pack is animated but isn't designed for detaching parts.
- **Service-level maintenance cavities** sized for the creature's early body. These are layout-specific.
- **Breaker panel with a sabotage/repair state** (INT-004). Rusty Switchboards is a modifiable base.

## License cautions
- **Fab Standard License** (https://www.fab.com/eula): commercial use, modification and use with any tool are allowed. Standalone resale or redistribution is **not** allowed. Sharing with collaborators in a **private** repo is allowed.
  - **The Chimera repo is public, so don't commit purchased source assets to it.** Use private LFS or external storage.
  - The Personal and Professional tiers grant the same rights. The Reference-Only tier doesn't give source files.
- **Megascans:** new Megascans on Fab fall under the Fab license and many are now paid. Legacy Megascans claimed through Quixel/Bridge stay under the older Epic content terms, which may be Unreal-only. Record each asset's provenance.
- **CC0** (Poly Haven, ambientCG): no restrictions, and attribution is optional.
- Some listings bundle third-party content (for example, Gen Corp uses ambientCG and Unsplash textures). Keep notes.
- The GDD forbids unlicensed franchise-derived assets. Avoid unofficial "free download" mirrors of Fab packs, which turned up in search results.
- Several listings say "Allows usage with AI: No". If the Tripo workflow feeds purchased assets into AI tools, check that flag per listing.
