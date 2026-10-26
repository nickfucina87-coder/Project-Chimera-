// Rules engine v1: Foundry pf2e rule elements read from the rules library and applied to the hero.
// Sources are the hero's feats, the class features they have reached, and build choices (subclass picks).
// Supported so far: FlatModifier (pf2e stacking: per type, highest bonus and lowest penalty; untyped stack),
// BaseSpeed, and Note. Rule elements with a predicate are conditional and skipped until predicates are evaluated.
#include "HollowRules.h"

#include "HollowLibrary.h"

namespace
{
	struct FMod { FString Sel, Type; int32 Value = 0; };

	bool Number(const TSharedPtr<FJsonValue>& V, const FPC& P, int32& Out)
	{
		if (!V.IsValid()) return false;
		double D = 0;
		if (V->TryGetNumber(D)) { Out = (int32)D; return true; }
		FString S;
		if (!V->TryGetString(S)) return false;
		S = S.Replace(TEXT(" "), TEXT(""));
		const bool bNeg = S.StartsWith(TEXT("-"));
		if (bNeg) S = S.Mid(1);
		if (S == TEXT("@actor.level") || S == TEXT("@item.level")) { Out = bNeg ? -P.Level : P.Level; return true; }
		if (S.IsNumeric()) { Out = FCString::Atoi(*S) * (bNeg ? -1 : 1); return true; }
		return false;   // formulas (ternary, max, ...) wait for a real evaluator
	}

	void Collect(const FLibEntry& E, const FPC& P, TArray<FMod>& Mods, int32& Speed, TArray<FString>& Notes)
	{
		for (const TSharedPtr<FJsonObject>& R : E.Rules)
		{
			FString Key;
			if (!R->TryGetStringField(TEXT("key"), Key)) continue;
			const TArray<TSharedPtr<FJsonValue>>* Pred = nullptr;
			if (R->TryGetArrayField(TEXT("predicate"), Pred) && Pred->Num()) continue;
			if (Key == TEXT("FlatModifier"))
			{
				int32 V = 0;
				if (!Number(R->TryGetField(TEXT("value")), P, V)) continue;
				FString Type = TEXT("untyped");
				R->TryGetStringField(TEXT("type"), Type);
				TArray<FString> Sels;
				FString One;
				const TArray<TSharedPtr<FJsonValue>>* Many = nullptr;
				if (R->TryGetStringField(TEXT("selector"), One)) Sels.Add(One);
				else if (R->TryGetArrayField(TEXT("selector"), Many)) for (const auto& S : *Many) Sels.Add(S->AsString());
				for (const FString& S : Sels) Mods.Add({ S, Type, V });
			}
			else if (Key == TEXT("BaseSpeed"))
			{
				FString Sel;
				int32 V = 0;
				if (R->TryGetStringField(TEXT("selector"), Sel) && (Sel == TEXT("land") || Sel == TEXT("land-speed")) && Number(R->TryGetField(TEXT("value")), P, V)) Speed = FMath::Max(Speed, V);
			}
			else if (Key == TEXT("Note"))
			{
				FString T;
				if (R->TryGetStringField(TEXT("text"), T) && !T.StartsWith(TEXT("PF2E."))) Notes.Add(E.Name + TEXT(": ") + T);
			}
		}
	}
}

namespace HollowRules
{
void Recompute(FPC& P)
{
	if (!HollowLib::Load()) return;
	TArray<FMod> Mods;
	int32 Speed = 0;
	P.RuleNotes.Reset();
	for (const TPair<int32, FString>& F : P.Feats)
	{
		if (const FLibFeat* L = HollowLib::Feat(F.Value)) Collect(*L, P, Mods, Speed, P.RuleNotes);
		else if (const FLibEntry* Cf = HollowLib::ClassFeature(F.Value)) Collect(*Cf, P, Mods, Speed, P.RuleNotes);
	}
	if (const FLibClass* C = HollowLib::Class(P.ClassName))
		for (const TPair<int32, FString>& Ft : C->Features)
			if (Ft.Key <= P.Level) if (const FLibEntry* Cf = HollowLib::ClassFeature(Ft.Value)) Collect(*Cf, P, Mods, Speed, P.RuleNotes);

	// pf2e stacking: for each selector, the best bonus and worst penalty of each type; untyped all apply
	TMap<FString, TMap<FString, TPair<int32, int32>>> Best;
	P.RuleMods.Reset();
	for (const FMod& M : Mods)
	{
		if (M.Type == TEXT("untyped")) { P.RuleMods.FindOrAdd(M.Sel) += M.Value; continue; }
		TPair<int32, int32>& B = Best.FindOrAdd(M.Sel).FindOrAdd(M.Type);
		if (M.Value > 0) B.Key = FMath::Max(B.Key, M.Value); else B.Value = FMath::Min(B.Value, M.Value);
	}
	for (const auto& S : Best) for (const auto& T : S.Value) P.RuleMods.FindOrAdd(S.Key) += T.Value.Key + T.Value.Value;

	// hit points and speed are stored on the sheet, so apply only the change since last time
	const int32 Hp = P.RuleMods.FindRef(TEXT("hp")) + P.RuleMods.FindRef(TEXT("hp-per-level")) * P.Level;
	const int32 DHp = Hp - P.RuleHpApplied;
	P.HpMax += DHp; P.Hp = FMath::Clamp(P.Hp + DHp, 1, P.HpMax);
	P.RuleHpApplied = Hp;
	// BaseSpeed rules grant extra movement types (climb, swim, fly) we don't model yet; land speed bonuses are flat
	(void)Speed;
	const int32 Sp = P.RuleMods.FindRef(TEXT("land-speed")) + P.RuleMods.FindRef(TEXT("speed"));
	P.Speed += Sp - P.RuleSpeedApplied;
	P.RuleSpeedApplied = Sp;
}

int32 RuleMod(const FPC& P, std::initializer_list<const TCHAR*> Selectors)
{
	int32 T = 0;
	for (const TCHAR* S : Selectors) T += P.RuleMods.FindRef(S);
	return T;
}
}
