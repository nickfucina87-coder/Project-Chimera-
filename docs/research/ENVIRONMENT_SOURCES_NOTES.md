# CHM-005 Environment Sources: Research Notes (2026-10-20)

Research only. Nothing was downloaded or bought, and no Unreal project files were touched.

## Method
- I read the repo docs: MASTER_GAME_DESIGN.md, ASSET_MANIFEST.csv (ENV-001..007, INT-001..008), ASSET_MANIFEST_GUIDE.md (blockout guidance lives here; there is no separate blockout file), and issue #5.
- The GDD says the **exact UE version still needs verification**. Every row records the listing's stated range so we can filter once a version is locked.
- Specs come from Fab listing pages, plus vendor pages where noted. Fab's API is behind Cloudflare, so some technical-detail blocks didn't render. Those fields say "not stated".
- Sale prices were observed on 2026-10-20 and can expire. A crossed-out list price is the Fab search-card price for that listing URL. A "license selector up to" figure is the top tier on the listing, not the pre-sale list price.

## Link check (2026-10-20)
Ten rows pointed at a Fab search page or the old Unreal Marketplace. Each listing page was opened. Direct URLs replaced the search and Marketplace links. Price, version, and format were changed only when that page (or, for one price, the Fab search card and seller page for the same listing URL) showed the value.

Fixed to a direct Fab listing:
- Jobsite VOL.15 - Tech and Data (Dekogon): `5c88b22b-14d3-4401-baef-156f637d311e`. UE 5.1-5.8, Unreal asset package. Sale $23.09 (list $32.99) still matches the search card. Allows usage with AI: No.
- Industrial Electrical Utility (Universal Constructor): `394c0e91-5429-4b7c-8484-abb6d08d96fd`. UE 4.23-4.27 and 5.0-5.4, Unreal asset package. From $10.99. Allows usage with AI: No.
- Server Room Kit (FastTrack Studio): `34f4694a-555d-42f3-92d3-17171b2d90bb`. This replaces `unrealengine.com/marketplace/.../server-room-kit`. UE 4.20-4.27 and 5.0-5.4, Unreal asset package, collision and 2 LOD stages stated. From $19.99. Allows usage with AI: No.
- Data Center AI Rack and Liquid Cooling Equipment Kit (Amer04): `73496c2a-ce8a-40d4-8f1b-a9f0b48b549d`. Formats are Blender, FBX, OBJ, and GLB. No Unreal Engine version is stated. From $29.99. Allows usage with AI: Yes.
- Buildings VOL.6 - Electrical Boxes (Dekogon): `f2cc527e-f241-4d09-b6df-9464dd1462ce`. UE 5.1-5.8, Unreal asset package. Sale $13.99 (list $19.99). Allows usage with AI: No.
- Modular server racks (Gorodenkoff): `6c2562d1-da28-4851-b181-9864c6f7b58f`. UE 5.6-5.7, Unreal asset package. From $129.99. Allows usage with AI: No.
- Standby Diesel Generator - PBR Animated Doors (Amer04): `a2163270-a407-4b0b-97d9-46b4aaee1ca3`. Formats are Blender, FBX, OBJ, and GLB, with LOD0 and LOD1. No Unreal Engine version is stated. From $27.99. Allows usage with AI: Yes.
- Retro Mainframe & Control Room Props (Clean & Wrecked) (6173 Studio): `c78cdc2c-684a-4419-bfef-d652ba2e561d`. UE 5.3-5.8, Unreal asset package, collision yes, LODs no. From $38.99. Allows usage with AI: No.
- Sci-Fi Blast Door – Rigged & Animated (Animaster Studio): `fb146e96-3dd1-4807-b8ac-6073c399df99`. UE 5.3-5.8, Unreal asset package. From $14.99. Allows usage with AI: Yes.

Partly unverified:
- Bunker Builder: Modular Bunker Building Kit (Hivemind), `e41cae71-f7a9-4640-a2a4-b6aa40d65a00`. The description rendered (modular kit, Lumen for UE 5.0+, Nanite with SM6). The price widget, included formats, supported-version range, and AI-usage flag did not render. The $39.99 sale (list $79.99) is from the Fab search card and the Hivemind seller page for this same URL. A previous 5.5.4+ figure was not on the rendered listing and is not confirmed.

No row was removed.

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

Jobsite VOL.15 remains the data-center pick on price and content, and its listing says **Allows usage with AI: No**. Use it as a purchased Unreal kit. Do not feed those meshes into Tripo or another AI tool. The harvestable internals stay custom geometry.

## Gaps that need custom Tripo geometry
- **Harvestable rack internals** (PSU, logic board, fan module; INT-001..003). No kit says it ships separately detachable components. The opened AI-rack listing separates sleds, doors, and cooling pieces, and the diesel listing separates service doors and internal assemblies. Neither states a harvestable PSU, logic board, or fan module.
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
- AI-usage flags read on the opened listings: **No** for Jobsite VOL.15, Industrial Electrical Utility, Server Room Kit, Buildings VOL.6, Modular server racks, and Retro Mainframe. **Yes** for the AI-rack kit, the standby diesel generator, and the Sci-Fi Blast Door. Bunker Builder's flag did not render. Other rows were not re-opened in this pass. If a Tripo workflow would send a purchased mesh to an AI tool, check that listing's flag first.
