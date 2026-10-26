#include "HollowRules.h"

namespace HollowRules
{

int32 D(int32 N) { return FMath::RandRange(1, FMath::Max(1, N)); }

FRollResult Roll(const FString& Expr)
{
	FRollResult Res;
	FString S = Expr.TrimStartAndEnd();
	int32 DPos;
	if (!S.FindChar(TEXT('d'), DPos))
	{
		Res.Total = FCString::Atoi(*S);
		return Res;
	}
	const int32 Count = FCString::Atoi(*S.Left(DPos));
	FString Rest = S.Mid(DPos + 1);
	int32 Sides, Mod = 0;
	int32 SignPos = INDEX_NONE;
	for (int32 i = 0; i < Rest.Len(); ++i)
		if (Rest[i] == TEXT('+') || Rest[i] == TEXT('-')) { SignPos = i; break; }
	if (SignPos == INDEX_NONE) Sides = FCString::Atoi(*Rest);
	else { Sides = FCString::Atoi(*Rest.Left(SignPos)); Mod = FCString::Atoi(*Rest.Mid(SignPos)); }
	int32 T = 0;
	for (int32 i = 0; i < FMath::Max(1, Count); ++i) { const int32 V = D(Sides); Res.Rolls.Add(V); T += V; }
	Res.Mod = Mod;
	Res.Total = FMath::Max(0, T + Mod);
	return Res;
}

int32 R(const FString& Expr) { return Roll(Expr).Total; }

int32 Degree(int32 Total, int32 DC, int32 Nat)
{
	int32 G = Total >= DC + 10 ? 3 : Total >= DC ? 2 : Total <= DC - 10 ? 0 : 1;
	if (Nat == 20) G = FMath::Min(3, G + 1);
	if (Nat == 1) G = FMath::Max(0, G - 1);
	return G;
}

const TCHAR* DegreeName(int32 Deg)
{
	static const TCHAR* N[] = { TEXT("Critical Failure"), TEXT("Failure"), TEXT("Success"), TEXT("Critical Success") };
	return N[FMath::Clamp(Deg, 0, 3)];
}

FString Sgn(int32 V) { return V >= 0 ? FString::Printf(TEXT("+%d"), V) : FString::Printf(TEXT("-%d"), -V); }

static const TMap<FString, FString>& SkillAbil()
{
	static const TMap<FString, FString> M = {
		{ TEXT("acrobatics"), TEXT("dex") }, { TEXT("arcana"), TEXT("int") }, { TEXT("athletics"), TEXT("str") },
		{ TEXT("deception"), TEXT("cha") }, { TEXT("diplomacy"), TEXT("cha") }, { TEXT("intimidation"), TEXT("cha") },
		{ TEXT("medicine"), TEXT("wis") }, { TEXT("religion"), TEXT("wis") }, { TEXT("stealth"), TEXT("dex") },
		{ TEXT("survival"), TEXT("wis") }, { TEXT("thievery"), TEXT("dex") } };
	return M;
}

const TArray<FString>& SkillList()
{
	static const TArray<FString> L = [] { TArray<FString> K; SkillAbil().GetKeys(K); K.Sort(); return K; }();
	return L;
}

FString SkillName(const FString& Skill)
{
	if (Skill.IsEmpty()) return Skill;
	return Skill.Left(1).ToUpper() + Skill.Mid(1);
}

int32 Prof(int32 Rank, int32 Level) { return Rank > 0 ? Rank * 2 + Level : 0; }

int32 SkillMod(const FPC& PC, const FString& Skill)
{
	if (Skill == TEXT("perception")) return PC.Abil.FindRef(TEXT("wis")) + Prof(PC.PerRank, PC.Level) + RuleMod(PC, { TEXT("perception"), TEXT("all") });
	const FString* A = SkillAbil().Find(Skill);
	return (A ? PC.Abil.FindRef(*A) : 0) + Prof(PC.Ranks.FindRef(Skill), PC.Level) + PC.RuleMods.FindRef(Skill) + RuleMod(PC, { TEXT("skill-check"), TEXT("all") });
}

int32 SaveMod(const FPC& PC, const FString& Save)
{
	const TCHAR* A = Save == TEXT("fort") ? TEXT("con") : Save == TEXT("ref") ? TEXT("dex") : TEXT("wis");
	const TCHAR* Full = Save == TEXT("fort") ? TEXT("fortitude") : Save == TEXT("ref") ? TEXT("reflex") : TEXT("will");
	return PC.Abil.FindRef(A) + Prof(PC.Saves.FindRef(Save), PC.Level) + RuleMod(PC, { Full, TEXT("saving-throw"), TEXT("all") });
}

int32 AC(const FPC& PC) { return PC.AcItem + Prof(PC.ArmorRank, PC.Level) + RuleMod(PC, { TEXT("ac") }); }
int32 AtkMod(const FPC& PC) { return PC.Abil.FindRef(PC.WeaponAbil) + Prof(PC.WeaponRank, PC.Level) + RuleMod(PC, { TEXT("attack-roll"), TEXT("strike-attack-roll"), TEXT("attack") }); }
int32 DmgBonus(const FPC& PC) { return PC.Abil.FindRef(PC.WeaponAbil) + RuleMod(PC, { TEXT("damage"), TEXT("strike-damage") }); }
int32 CastMod(const FPC& PC) { return PC.bCaster ? PC.Abil.FindRef(PC.SpellAbil) : FMath::Max(PC.Abil.FindRef(TEXT("int")), PC.Abil.FindRef(TEXT("wis"))); }
int32 SpellAtk(const FPC& PC) { return CastMod(PC) + Prof(PC.bCaster ? PC.SpellRank : 1, PC.Level); }
int32 SpellDC(const FPC& PC) { return 10 + SpellAtk(PC); }

FString BestSkill(const FPC& PC, const TArray<FString>& Skills)
{
	FString Best = Skills.Num() ? Skills[0] : FString();
	for (const FString& S : Skills) if (SkillMod(PC, S) > SkillMod(PC, Best)) Best = S;
	return Best;
}

int32 XpForLevel(int32 Level)
{
	static const int32 T[] = { 0, 60, 150, 280, 450, 650 };
	if (Level <= 6) return T[FMath::Clamp(Level - 1, 0, 5)];
	return 650 + (Level - 6) * 250;   // past level 6 every level costs a steady 250
}

const TArray<FAncestry>& Ancestries()
{
	static const TArray<FAncestry> L = {
		{ TEXT("human"),  TEXT("Human"),  TEXT("cha"), TEXT("diplomacy"),    TEXT("8 HP, +1 Cha, trained Diplomacy"), 8, 25, 0 },
		{ TEXT("elf"),    TEXT("Elf"),    TEXT("int"), TEXT(""),             TEXT("6 HP, +1 Int, keen Perception, fast"), 6, 30, 1 },
		{ TEXT("dwarf"),  TEXT("Dwarf"),  TEXT("con"), TEXT(""),             TEXT("10 HP, +1 Con, slow and sturdy"), 10, 20, 0 },
		{ TEXT("orc"),    TEXT("Orc"),    TEXT("str"), TEXT("intimidation"), TEXT("10 HP, +1 Str, trained Intimidation"), 10, 25, 0 },
		{ TEXT("goblin"), TEXT("Goblin"), TEXT("cha"), TEXT("stealth"),      TEXT("6 HP, +1 Cha, trained Stealth"), 6, 25, 0 },
	};
	return L;
}

const TArray<FKit>& Kits()
{
	static const TArray<FKit> L = {
		{ TEXT("fighter"), TEXT("Fighter"), TEXT("Expert with weapons. Power Attack and Double Slice."), 10 },
		{ TEXT("rogue"),   TEXT("Rogue"),   TEXT("Feint to leave foes off-guard, then sneak attack."), 8 },
		{ TEXT("wizard"),  TEXT("Wizard"),  TEXT("Force Barrage never misses. Electric Arc hits everyone."), 6 },
		{ TEXT("cleric"),  TEXT("Cleric"),  TEXT("Heals mid-fight and burns the dead."), 8 },
	};
	return L;
}

FPC BuildPC(const FString& Name, const FString& AncId, const FString& KitId)
{
	const FAncestry* A = Ancestries().FindByPredicate([&](const FAncestry& X) { return X.Id == AncId; });
	if (!A) A = &Ancestries()[0];
	const FKit* K = Kits().FindByPredicate([&](const FKit& X) { return X.Id == KitId; });
	if (!K) K = &Kits()[0];

	FPC P;
	P.Name = Name.IsEmpty() ? TEXT("Nameless") : Name;
	P.Ancestry = A->Name; P.Kit = K->Id; P.ClassName = K->Name;
	P.Speed = A->Speed;

	auto SetAbil = [&](int32 S, int32 Dx, int32 C, int32 I, int32 W, int32 Ch)
	{
		P.Abil = { { TEXT("str"), S }, { TEXT("dex"), Dx }, { TEXT("con"), C }, { TEXT("int"), I }, { TEXT("wis"), W }, { TEXT("cha"), Ch } };
	};
	int32 ArmorBonus = 0, DexCap = 5;
	if (K->Id == TEXT("rogue"))
	{
		SetAbil(0, 4, 1, 1, 1, 2);
		P.Ranks = { { TEXT("stealth"), 1 }, { TEXT("thievery"), 1 }, { TEXT("acrobatics"), 1 }, { TEXT("deception"), 1 }, { TEXT("diplomacy"), 1 }, { TEXT("athletics"), 1 } };
		P.PerRank = 2; P.Saves = { { TEXT("fort"), 1 }, { TEXT("ref"), 2 }, { TEXT("will"), 2 } };
		P.ArmorName = TEXT("Leather Armor"); ArmorBonus = 1; DexCap = 4;
		P.WeaponName = TEXT("Rapier"); P.WeaponDie = 6; P.WeaponAbil = TEXT("dex"); P.WeaponRank = 1;
		P.Body = FLinearColor(0.18f, 0.2f, 0.16f); P.Hat = FLinearColor(0.12f, 0.14f, 0.11f); P.HatStyle = 4;
	}
	else if (K->Id == TEXT("wizard"))
	{
		SetAbil(0, 2, 1, 4, 1, 0);
		P.Ranks = { { TEXT("arcana"), 1 }, { TEXT("medicine"), 1 }, { TEXT("religion"), 1 } };
		P.PerRank = 1; P.Saves = { { TEXT("fort"), 1 }, { TEXT("ref"), 1 }, { TEXT("will"), 2 } };
		P.ArmorName = TEXT("Robes"); ArmorBonus = 0;
		P.WeaponName = TEXT("Staff"); P.WeaponDie = 4; P.WeaponAbil = TEXT("str"); P.WeaponRank = 1;
		P.bCaster = true; P.SpellAbil = TEXT("int");
		P.Body = FLinearColor(0.25f, 0.2f, 0.6f); P.Hat = FLinearColor(0.3f, 0.22f, 0.7f); P.HatStyle = 2;
	}
	else if (K->Id == TEXT("cleric"))
	{
		SetAbil(2, 0, 1, 0, 4, 1);
		P.Ranks = { { TEXT("religion"), 1 }, { TEXT("medicine"), 1 }, { TEXT("diplomacy"), 1 } };
		P.PerRank = 1; P.Saves = { { TEXT("fort"), 1 }, { TEXT("ref"), 1 }, { TEXT("will"), 2 } };
		P.ArmorName = TEXT("Hide Armor"); ArmorBonus = 3; DexCap = 2;
		P.WeaponName = TEXT("Mace"); P.WeaponDie = 6; P.WeaponAbil = TEXT("str"); P.WeaponRank = 1;
		P.bCaster = true; P.SpellAbil = TEXT("wis");
		P.Body = FLinearColor(0.92f, 0.86f, 0.7f); P.Hat = FLinearColor(0.95f, 0.75f, 0.2f); P.HatStyle = 3;
	}
	else
	{
		SetAbil(4, 2, 2, 0, 1, 0);
		P.Ranks = { { TEXT("athletics"), 1 }, { TEXT("intimidation"), 1 }, { TEXT("survival"), 1 }, { TEXT("acrobatics"), 1 } };
		P.PerRank = 2; P.Saves = { { TEXT("fort"), 2 }, { TEXT("ref"), 2 }, { TEXT("will"), 1 } };
		P.ArmorName = TEXT("Chain Mail"); ArmorBonus = 4; DexCap = 1;
		P.WeaponName = TEXT("Longsword"); P.WeaponDie = 8; P.WeaponAbil = TEXT("str"); P.WeaponRank = 2;
		P.Body = FLinearColor(0.62f, 0.15f, 0.12f); P.Hat = FLinearColor(0.6f, 0.62f, 0.66f); P.HatStyle = 1;
	}
	P.Abil[A->Boost] = FMath::Min(4, P.Abil[A->Boost] + 1);
	if (!A->Skill.IsEmpty()) P.Ranks.FindOrAdd(A->Skill) = FMath::Max(P.Ranks.FindRef(A->Skill), 1);
	P.PerRank = FMath::Min(2, P.PerRank + A->PerBonus);
	P.AcItem = 10 + FMath::Min(P.Abil[TEXT("dex")], DexCap) + ArmorBonus;
	P.HpPer = K->Hp + P.Abil[TEXT("con")];
	P.HpMax = A->Hp + P.HpPer;
	P.Hp = P.HpMax;
	P.Gold = 20;
	P.Inv.Init();
	P.Inv.Add(TEXT("ration"), 6);
	P.Inv.Add(TEXT("potion"), 1);
	P.Inv.Add(TEXT("torch"), 2);
	// one starting tool per kit, so each class explores a little differently
	P.Inv.Add(K->Id == TEXT("rogue") ? TEXT("picks") : K->Id == TEXT("fighter") ? TEXT("rope") : K->Id == TEXT("cleric") ? TEXT("holy") : TEXT("tinderbox"), 1);
	P.Deck = HollowData::StarterDeck(K->Id);

	static const TMap<FString, FLinearColor> Skins = {
		{ TEXT("human"), FLinearColor(0.93f, 0.72f, 0.55f) }, { TEXT("elf"), FLinearColor(0.96f, 0.84f, 0.7f) },
		{ TEXT("dwarf"), FLinearColor(0.85f, 0.6f, 0.45f) }, { TEXT("orc"), FLinearColor(0.45f, 0.65f, 0.35f) },
		{ TEXT("goblin"), FLinearColor(0.55f, 0.78f, 0.3f) } };
	P.Skin = Skins.FindRef(A->Id);
	return P;
}

}
