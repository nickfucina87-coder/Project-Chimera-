// Builds ExternalAssets/Library/library.json, the game's rules library, from a local Foundry VTT pf2e export.
//
//   node Tools/BuildLibrary.mjs [exportRoot] [pathmuncherMainJs]
//
// exportRoot defaults to ExternalAssets/FoundryVTT. It holds pf2e/<pack>/*.json and world/<db>/*.json, written by
// ExternalAssets/FoundryVTT/unpack.mjs (the official @foundryvtt/foundryvtt-cli).
// Pathmuncher's MIT-licensed Pathbuilder->Foundry rename tables are read from its main.js when present, so a
// Pathbuilder build resolves to the same compendium entries Pathmuncher would pick. Credit: Jack Holloway (MIT).
//
// The output is gitignored: pf2e rules content is ORC/OGL Paizo material and stays local to this machine.
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const src = process.argv[2] ?? path.join(root, "ExternalAssets", "FoundryVTT");
const pmJs = process.argv[3] ?? path.join(process.env.LOCALAPPDATA ?? "", "FoundryVTT", "Data", "modules", "pathmuncher", "dist", "main.js");
const out = path.join(root, "ExternalAssets", "Library");

const load = (dir) => fs.existsSync(dir) ? fs.readdirSync(dir).filter((f) => f.endsWith(".json")).map((f) => JSON.parse(fs.readFileSync(path.join(dir, f), "utf8"))) : [];
const pack = (name) => load(path.join(src, "pf2e", name)).filter((d) => d.system && d.type);
const world = (name) => load(path.join(src, "world", name));

// Foundry rich text -> plain text: enrichers become their labels, tags go, whitespace collapses.
function plain(html, max = 700) {
  if (!html) return "";
  let s = String(html)
    .replace(/@\w+\[[^\]]*\]\{([^}]*)\}/g, "$1")
    .replace(/@(?:Damage|Check|Template)\[([^\]|]*)[^\]]*\]/g, "$1")
    .replace(/@UUID\[[^\]]*?([^.\]]+)\]/g, "$1")
    .replace(/\[\[\/\w+ ([^\]]*)\]\](?:\{([^}]*)\})?/g, (_, f, l) => l ?? f)
    .replace(/<\/(p|li|h\d)>/g, "\n").replace(/<br\s*\/?>/g, "\n").replace(/<hr\s*\/?>/g, "\n")
    .replace(/<[^>]+>/g, "").replace(/&nbsp;/g, " ").replace(/&amp;/g, "&").replace(/&quot;/g, '"').replace(/&#39;/g, "'")
    .replace(/[ \t]+/g, " ").replace(/\n\s*\n+/g, "\n").trim();
  return s.length > max ? s.slice(0, max - 1).trimEnd() + "…" : s;
}
const silver = (p) => { const v = p?.value ?? {}; return (v.pp ?? 0) * 100 + (v.gp ?? 0) * 10 + (v.sp ?? 0) + (v.cp ?? 0) / 10; };
const traits = (s) => s?.traits?.value ?? [];
const rarity = (s) => s?.traits?.rarity ?? "common";
const desc = (s, n) => plain(s?.description?.value, n);
// Foundry rule elements (FlatModifier, ChoiceSet, GrantItem, DamageDice, Resistance...), kept as data for the C++ rules engine
const rules = (s) => (s?.rules ?? []).filter((r) => r && r.key).map((r) => { const o = { ...r }; delete o.slug; delete o._migration; return o; });
const tags = (s) => s?.traits?.otherTags ?? [];

const lib = { version: 1, source: "Foundry VTT pf2e export + Pathmuncher rename tables", built: new Date().toISOString() };

lib.classes = pack("classes").map((d) => {
  const s = d.system;
  const feats = Object.values(s.items ?? {}).sort((a, b) => a.level - b.level).map((i) => ({ level: i.level, name: i.name }));
  return {
    name: d.name, hp: s.hp, keyAbility: s.keyAbility?.value ?? [], perception: s.perception, saves: s.savingThrows,
    attacks: { simple: s.attacks?.simple ?? 0, martial: s.attacks?.martial ?? 0, advanced: s.attacks?.advanced ?? 0, unarmed: s.attacks?.unarmed ?? 0 },
    defenses: s.defenses, trainedSkills: s.trainedSkills?.value ?? [], additionalSkills: s.trainedSkills?.additional ?? 0,
    spellcasting: s.spellcasting ?? 0, features: feats,
    skillIncreaseLevels: s.skillIncreaseLevels?.value ?? [], classFeatLevels: s.classFeatLevels?.value ?? [],
    generalFeatLevels: s.generalFeatLevels?.value ?? [], ancestryFeatLevels: s.ancestryFeatLevels?.value ?? [],
    skillFeatLevels: s.skillFeatLevels?.value ?? [], text: desc(s, 500),
  };
});

lib.ancestries = pack("ancestries").map((d) => {
  const s = d.system;
  return { name: d.name, hp: Number(s.hp), speed: Number(s.speed), size: s.size, vision: s.vision, rarity: rarity(s), traits: traits(s),
    boosts: Object.values(s.boosts ?? {}).map((b) => b.value), flaws: Object.values(s.flaws ?? {}).map((b) => b.value), text: desc(s, 400) };
});
lib.heritages = pack("heritages").map((d) => ({ name: d.name, ancestry: d.system.ancestry?.name ?? "", rarity: rarity(d.system), text: desc(d.system, 300) }));
lib.backgrounds = pack("backgrounds").map((d) => ({ name: d.name, rarity: rarity(d.system), skills: d.system.trainedSkills?.value ?? [],
  lore: d.system.trainedSkills?.lore ?? [], boosts: Object.values(d.system.boosts ?? {}).map((b) => b.value), text: desc(d.system, 300) }));

lib.feats = pack("feats").map((d) => {
  const s = d.system;
  return { name: d.name, level: s.level?.value ?? 1, category: s.category, rarity: rarity(s), traits: traits(s),
    action: s.actionType?.value ?? "passive", actions: s.actions?.value ?? null,
    prereq: (s.prerequisites?.value ?? []).map((p) => p.value), tags: tags(s), rules: rules(s), text: desc(s, 360) };
});
const cfDocs = pack("classfeatures");
lib.classFeatures = cfDocs.map((d) => ({ name: d.name, level: d.system.level?.value ?? 1, traits: traits(d.system), tags: tags(d.system), rules: rules(d.system), text: desc(d.system, 360) }));

// Subclasses and other build choices: every class feature with a ChoiceSet becomes a choice with resolved options.
// Foundry writes the options either as "item:tag:<tag>" filters (Magus hybrid study, Sorcerer bloodline...) or as UUID lists.
const byUuid = new Map();
for (const [packName, docs] of [["classfeatures", cfDocs], ["feats", pack("feats")]])
  for (const d of docs) byUuid.set(`Compendium.pf2e.${packName}.Item.${d._id}`, d);
const featDocs = [...byUuid.values()];
lib.choices = [];
for (const d of cfDocs) {
  for (const r of d.system.rules ?? []) {
    if (r.key !== "ChoiceSet") continue;
    let opts = [];
    const ch = r.choices;
    if (Array.isArray(ch)) opts = ch.map((c) => byUuid.get(c.value)).filter(Boolean);
    else if (ch?.filter) {
      const want = ch.filter.filter((f) => typeof f === "string" && f.startsWith("item:tag:")).map((f) => f.slice(9));
      if (want.length) opts = featDocs.filter((x) => want.every((t) => tags(x.system).includes(t)));
    }
    if (!opts.length) continue;
    if (lib.choices.some((c) => c.feature === d.name && (c.flag === (r.flag ?? "")))) continue;   // Foundry repeats some ChoiceSets
    const cls = traits(d.system).find((t) => lib.classes.some((c) => c.name.toLowerCase() === t)) ?? "";
    lib.choices.push({ feature: d.name, class: cls, level: d.system.level?.value ?? 1, flag: r.flag ?? "", prompt: r.prompt ?? "",
      options: [...new Map(opts.map((o) => [o.name, { name: o.name, rarity: rarity(o.system), text: desc(o.system, 400) }])).values()] });
  }
}

lib.spells = pack("spells").map((d) => {
  const s = d.system;
  const t = traits(s);
  const dmg = Object.values(s.damage ?? {}).map((x) => ({ formula: x.formula, type: x.type, healing: (x.kinds ?? []).includes("healing") && !(x.kinds ?? []).includes("damage") ? true : (x.kinds ?? []).length === 1 && x.kinds[0] === "healing" }));
  const def = s.defense?.save ? { save: s.defense.save.statistic, basic: !!s.defense.save.basic } : s.defense?.passive ? { vs: s.defense.passive.statistic } : null;
  return {
    name: d.name, rank: s.level?.value ?? 1, cantrip: t.includes("cantrip"), focus: t.includes("focus"), rarity: rarity(s),
    traditions: s.traits?.traditions ?? [], traits: t, cast: s.time?.value ?? "2", range: s.range?.value ?? "",
    area: s.area ? { type: s.area.type, size: s.area.value } : null, target: s.target?.value ?? "", defense: def, damage: dmg,
    heighten: s.heightening?.type === "interval" ? { interval: s.heightening.interval, damage: Object.values(s.heightening.damage ?? {}) } : null,
    duration: s.duration?.value ?? "", sustained: !!s.duration?.sustained, text: desc(s, 520),
  };
});

lib.items = pack("equipment").map((d) => {
  const s = d.system;
  const o = { name: d.name, type: d.type, level: s.level?.value ?? 0, price: silver(s.price), bulk: s.bulk?.value ?? 0, rarity: rarity(s),
    traits: traits(s), usage: s.usage?.value ?? "", rules: rules(s), text: desc(s, 300) };
  if (d.type === "weapon") Object.assign(o, { die: s.damage?.die, dice: s.damage?.dice, dmgType: s.damage?.damageType, group: s.group, category: s.category, range: s.range ?? null });
  if (d.type === "armor") Object.assign(o, { ac: s.acBonus, dexCap: s.dexCap, str: s.strength, checkPenalty: s.checkPenalty, speedPenalty: s.speedPenalty, category: s.category });
  if (d.type === "shield") Object.assign(o, { ac: s.acBonus, hardness: s.hardness, hp: s.hp?.max });
  if (d.type === "consumable") Object.assign(o, { category: s.category, uses: s.uses?.max ?? 1, damage: s.damage ?? null });
  return o;
});
const creature = (a, src) => {
  const s = a.system ?? {};
  const at = s.attributes ?? {};
  const list = (x) => (x ?? []).map((v) => typeof v === "string" ? v : `${v.type}${v.value ? " " + v.value : ""}`);
  return {
    name: a.name, source: src, level: s.details?.level?.value ?? 0, rarity: s.traits?.rarity ?? "common", size: s.traits?.size?.value ?? "med",
    traits: s.traits?.value ?? [], hp: at.hp?.max ?? 0, ac: at.ac?.value ?? 10, perception: s.perception?.mod ?? 0,
    senses: (s.perception?.senses ?? []).map((x) => x.type), speed: at.speed?.value ?? 25,
    otherSpeeds: (at.speed?.otherSpeeds ?? []).map((x) => `${x.type} ${x.value}`),
    saves: { fort: s.saves?.fortitude?.value ?? 0, ref: s.saves?.reflex?.value ?? 0, will: s.saves?.will?.value ?? 0 },
    immunities: list(at.immunities), weaknesses: list(at.weaknesses), resistances: list(at.resistances),
    strikes: (a.items ?? []).filter((i) => i.type === "melee").map((i) => ({ name: i.name, bonus: i.system?.bonus?.value ?? 0, ranged: i.system?.weaponType?.value === "ranged",
      traits: i.system?.traits?.value ?? [], damage: Object.values(i.system?.damageRolls ?? {}).map((r) => ({ formula: r.damage, type: r.damageType })),
      effects: i.system?.attackEffects?.value ?? [] })),
    abilities: (a.items ?? []).filter((i) => i.type === "action").map((i) => ({ name: i.name, action: i.system?.actionType?.value ?? "passive",
      actions: i.system?.actions?.value ?? null, traits: i.system?.traits?.value ?? [], rules: rules(i.system), text: desc(i.system, 360) })),
    spells: (a.items ?? []).filter((i) => i.type === "spell").map((i) => i.name),
    text: plain(s.details?.publicNotes, 400),
  };
};
const seen = new Set();
lib.bestiary = [];
for (const [packName, label] of [["pathfinder-monster-core", "Monster Core"], ["pathfinder-bestiary", "Bestiary"]])
  for (const a of pack(packName)) if (a.type === "npc" && !seen.has(a.name)) { seen.add(a.name); lib.bestiary.push(creature(a, label)); }
lib.hazards = pack("hazards").filter((h) => h.type === "hazard").map((h) => {
  const s = h.system;
  return { name: h.name, level: s.details?.level?.value ?? 0, rarity: s.traits?.rarity ?? "common", traits: s.traits?.value ?? [],
    stealth: s.attributes?.stealth?.value ?? 0, ac: s.attributes?.ac?.value ?? 0, hp: s.attributes?.hp?.max ?? 0,
    disable: plain(s.details?.disable, 300), reset: plain(s.details?.reset, 200), text: plain(s.details?.description, 400),
    actions: (h.items ?? []).filter((i) => i.type === "action").map((i) => ({ name: i.name, action: i.system?.actionType?.value, text: desc(i.system, 300) })) };
});
lib.conditions = pack("conditions").map((d) => ({ name: d.name, valued: !!d.system.value?.isValued, rules: rules(d.system), overrides: d.system.overrides ?? [], text: desc(d.system, 400) }));
lib.actions = pack("actions").map((d) => ({ name: d.name, action: d.system.actionType?.value, actions: d.system.actions?.value ?? null, traits: traits(d.system), text: desc(d.system, 300) }));
lib.deities = pack("deities").map((d) => ({ name: d.name, sanctification: d.system.sanctification ?? null, font: d.system.font ?? [], weapons: d.system.weapons ?? [], text: desc(d.system, 240) }));

// the user's own campaign: journals, rollable tables, NPCs and scene underlays (walls + lights for line of sight)
lib.journals = world("journal").map((j) => ({ name: j.name, pages: (j.pages ?? []).map((p) => ({ name: p.name, text: plain(p.text?.content, 6000) })) }));
lib.tables = world("tables").map((t) => ({ name: t.name, formula: t.formula, results: (t.results ?? []).map((r) => ({ range: r.range, text: plain(r.text ?? r.name ?? r.description, 200) })) }));
lib.npcs = world("actors").filter((a) => a.type === "npc").map((a) => ({ name: a.name, level: a.system?.details?.level?.value ?? 0,
  hp: a.system?.attributes?.hp?.max ?? 0, ac: a.system?.attributes?.ac?.value ?? 10, perception: a.system?.perception?.mod ?? 0,
  saves: { fort: a.system?.saves?.fortitude?.value ?? 0, ref: a.system?.saves?.reflex?.value ?? 0, will: a.system?.saves?.will?.value ?? 0 },
  traits: a.system?.traits?.value ?? [], text: plain(a.system?.details?.publicNotes, 400),
  strikes: (a.items ?? []).filter((i) => i.type === "melee").map((i) => ({ name: i.name, bonus: i.system?.bonus?.value ?? 0, damage: Object.values(i.system?.damageRolls ?? {}).map((r) => `${r.damage} ${r.damageType}`).join(" + ") })) }));
lib.scenes = world("scenes").map((s) => ({ name: s.name, width: s.width, height: s.height, grid: s.grid?.size ?? 100, darkness: s.environment?.darknessLevel ?? s.darkness ?? 0,
  walls: (s.walls ?? []).map((w) => ({ c: w.c, move: w.move, sight: w.sight, light: w.light, door: w.door ?? 0 })),
  lights: (s.lights ?? []).map((l) => ({ x: l.x, y: l.y, bright: l.config?.bright ?? 0, dim: l.config?.dim ?? 0, color: l.config?.color ?? null })) }));

// Pathmuncher's rename tables: { pbName, foundryName } literals parsed as text, never evaluated
lib.renames = { equipment: [], feats: [], spells: [] };
if (fs.existsSync(pmJs)) {
  const js = fs.readFileSync(pmJs, "utf8");
  const table = (name) => {
    const at = js.indexOf(`const ${name} =`);
    if (at < 0) return [];
    const body = js.slice(at, js.indexOf("\n];", at));
    return [...body.matchAll(/pbName:\s*"((?:[^"\\]|\\.)*)",\s*foundryName:\s*"((?:[^"\\]|\\.)*)"/g)].map((m) => ({ pb: m[1], foundry: m[2] }));
  };
  lib.renames.equipment = table("EQUIPMENT_RENAME_STATIC_MAP");
  lib.renames.feats = table("FEAT_RENAME_STATIC_MAP");
  lib.renames.spells = table("REMASTER_NAMES");
}

fs.mkdirSync(out, { recursive: true });
fs.writeFileSync(path.join(out, "library.json"), JSON.stringify(lib));
const counts = Object.fromEntries(Object.entries(lib).filter(([, v]) => Array.isArray(v)).map(([k, v]) => [k, v.length]));
counts.renames = `${lib.renames.equipment.length} equipment, ${lib.renames.feats.length} feats, ${lib.renames.spells.length} spells`;
console.log(JSON.stringify(counts, null, 1));
console.log("wrote", path.join(out, "library.json"), (fs.statSync(path.join(out, "library.json")).size / 1e6).toFixed(1) + " MB");
