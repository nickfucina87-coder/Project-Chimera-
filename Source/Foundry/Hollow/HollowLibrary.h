// The rules library: every pf2e class, ancestry, heritage, background, feat, spell and item, plus the user's own
// campaign (journals, tables, NPCs, scene walls and lights), loaded at runtime from ExternalAssets/Library/library.json.
// That file is built locally by Tools/BuildLibrary.mjs from a Foundry VTT export and never committed (Paizo content).
// Lookups normalise names the way Pathmuncher does (rename tables, stripped postfixes) so Pathbuilder builds resolve.
#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

struct FLibEntry
{
	FString Name, Text, Rarity;
	int32 Level = 0;
	TArray<FString> Traits, Tags;
	FString Parent;                            // heritage: its ancestry
	TArray<TArray<FString>> Boosts, Flaws;     // ancestry / background: each slot lists the abilities it may boost
	TArray<FString> Skills;                    // background: trained skills
	TArray<TSharedPtr<FJsonObject>> Rules;     // Foundry rule elements (FlatModifier, GrantItem, ChoiceSet...)
};

struct FLibClass : FLibEntry
{
	int32 Hp = 8, Perception = 1, Spellcasting = 0, AdditionalSkills = 0;
	TArray<FString> KeyAbility, TrainedSkills;
	TMap<FString, int32> Saves, Attacks, Defenses;
	TArray<int32> SkillIncrease, ClassFeat, GeneralFeat, AncestryFeat, SkillFeat;
	TArray<TPair<int32, FString>> Features;
};

struct FLibAncestry : FLibEntry
{
	int32 Hp = 8, Speed = 25;
	FString Size, Vision;
};

struct FLibFeat : FLibEntry
{
	FString Category, Action;   // category: ancestry, class, general, skill, classfeature...; action: action, reaction, free, passive
	int32 Actions = 0;
	TArray<FString> Prereq;
};

struct FLibSpell : FLibEntry
{
	int32 Rank = 1, AreaSize = 0, Interval = 0;
	bool bCantrip = false, bFocus = false, bHeal = false, bBasic = false;
	FString Cast, Range, AreaType, Target, Save, Vs, Duration;
	TArray<FString> Traditions;
	TArray<TPair<FString, FString>> Damage;   // formula, type
	TArray<FString> HeightenDamage;           // added per Interval ranks above base
	int32 Actions() const;                    // 1-3 (reactions and free actions count as 1)
};

struct FLibItem : FLibEntry
{
	FString Type, Usage, Die, DmgType, Group, Category;
	int32 Dice = 1, Ac = 0, DexCap = 99, Str = 0, CheckPenalty = 0, Range = 0;
	float Price = 0.f, Bulk = 0.f;
};

struct FLibJournal { FString Name; TArray<TPair<FString, FString>> Pages; };
struct FLibWall { FVector4 C; int32 Move = 0, Sight = 0, Light = 0, Door = 0; };
struct FLibLight { FVector2D P; float Bright = 0.f, Dim = 0.f; FString Color; };
struct FLibScene { FString Name; int32 Width = 0, Height = 0, Grid = 100; float Darkness = 0.f; TArray<FLibWall> Walls; TArray<FLibLight> Lights; };
struct FLibNpc : FLibEntry { int32 Hp = 10, Ac = 10, Per = 0, Fort = 0, Ref = 0, Will = 0; TArray<TPair<FString, FString>> Strikes; };

// A build choice from a class feature's ChoiceSet: Magus hybrid study, Sorcerer bloodline, Witch patron...
struct FLibChoice
{
	FString Feature, Class, Flag;
	int32 Level = 1;
	TArray<FLibEntry> Options;
};

struct FLibStrike { FString Name; int32 Bonus = 0; bool bRanged = false; TArray<FString> Traits; TArray<TPair<FString, FString>> Damage; };
struct FLibCreature : FLibEntry
{
	FString Source, Size;
	int32 Hp = 10, Ac = 10, Per = 0, Fort = 0, Ref = 0, Will = 0, Speed = 25;
	TArray<FString> Senses, Immunities, Weaknesses, Resistances, Spells;
	TArray<FLibStrike> Strikes;
	TArray<FLibFeat> Abilities;
};
struct FLibHazard : FLibEntry { int32 Stealth = 0, Ac = 0, Hp = 0; FString Disable, Reset; };

namespace HollowLib
{
	bool Load();            // lazy; safe to call often
	bool IsLoaded();
	FString Path();

	const TArray<FLibClass>& Classes();
	const TArray<FLibAncestry>& Ancestries();
	const TArray<FLibEntry>& Heritages();
	const TArray<FLibEntry>& Backgrounds();
	const TArray<FLibFeat>& Feats();
	const TArray<FLibEntry>& ClassFeatures();
	const TArray<FLibSpell>& Spells();
	const TArray<FLibItem>& Items();
	const TArray<FLibEntry>& Conditions();
	const TArray<FLibEntry>& Actions();
	const TArray<FLibEntry>& Deities();
	const TArray<FLibJournal>& Journals();
	const TArray<FLibScene>& Scenes();
	const TArray<FLibNpc>& Npcs();
	const TArray<FLibChoice>& Choices();
	const TArray<FLibCreature>& Bestiary();
	const TArray<FLibHazard>& Hazards();
	const FLibEntry* ClassFeature(const FString& Name);
	const FLibCreature* Creature(const FString& Name);
	// Build choices a class makes at a level (level 1 = character creation).
	TArray<const FLibChoice*> ChoicesFor(const FString& ClassName, int32 Level);
	const TArray<const FLibEntry*> HeritagesOf(const FString& Ancestry);

	// Name lookups (case-insensitive, Pathbuilder spellings accepted)
	const FLibClass* Class(const FString& Name);
	const FLibAncestry* Ancestry(const FString& Name);
	const FLibFeat* Feat(const FString& Name);
	const FLibSpell* Spell(const FString& Name);
	const FLibItem* Item(const FString& Name);

	FString Slug(const FString& Name);
	FString SpellSummary(const FLibSpell& S, int32 AtRank);
	FString HeightenedFormula(const FLibSpell& S, int32 AtRank, int32 DamageIndex = 0);
}
