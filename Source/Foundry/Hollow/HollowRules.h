// Pathfinder-flavoured rules math: dice, degrees of success, proficiency and derived stats.
#pragma once

#include "HollowTypes.h"

struct FRollResult
{
	int32 Total = 0;
	TArray<int32> Rolls;
	int32 Mod = 0;
};

namespace HollowRules
{
	int32 D(int32 N);
	FRollResult Roll(const FString& Expr);
	int32 R(const FString& Expr);
	// 0 crit fail, 1 fail, 2 success, 3 crit success
	int32 Degree(int32 Total, int32 DC, int32 Nat);
	const TCHAR* DegreeName(int32 Deg);
	FString Sgn(int32 V);

	int32 Prof(int32 Rank, int32 Level);
	int32 SkillMod(const FPC& PC, const FString& Skill);
	int32 SaveMod(const FPC& PC, const FString& Save);
	int32 AC(const FPC& PC);
	int32 AtkMod(const FPC& PC);
	int32 DmgBonus(const FPC& PC);
	int32 SpellAtk(const FPC& PC);
	int32 SpellDC(const FPC& PC);
	int32 CastMod(const FPC& PC);
	FString BestSkill(const FPC& PC, const TArray<FString>& Skills);
	FString SkillName(const FString& Skill);
	const TArray<FString>& SkillList();
	int32 XpForLevel(int32 Level);
	// Rule elements from the library (feats, class features, subclass picks) folded into RuleMods.
	void Recompute(FPC& P);
	int32 RuleMod(const FPC& P, std::initializer_list<const TCHAR*> Selectors);

	struct FAncestry { FString Id, Name, Boost, Skill, Note; int32 Hp, Speed, PerBonus; };
	struct FKit { FString Id, Name, Note; int32 Hp; };
	const TArray<FAncestry>& Ancestries();
	const TArray<FKit>& Kits();
	FPC BuildPC(const FString& Name, const FString& AncId, const FString& KitId);
}
