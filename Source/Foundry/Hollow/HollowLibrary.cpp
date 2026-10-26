#include "HollowLibrary.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	struct FLib
	{
		bool bTried = false, bOk = false;
		TArray<FLibClass> Classes;
		TArray<FLibAncestry> Ancestries;
		TArray<FLibEntry> Heritages, Backgrounds, ClassFeatures, Conditions, Actions, Deities;
		TArray<FLibFeat> Feats;
		TArray<FLibSpell> Spells;
		TArray<FLibItem> Items;
		TArray<FLibJournal> Journals;
		TArray<FLibScene> Scenes;
		TArray<FLibNpc> Npcs;
		TArray<FLibChoice> Choices;
		TArray<FLibCreature> Bestiary;
		TArray<FLibHazard> Hazards;
		TMap<FString, int32> CfIx, CreatureIx;
		TMap<FString, FString> RenEquip, RenFeat, RenSpell;          // normalised Pathbuilder name -> Foundry name
		TMap<FString, int32> ClassIx, AncIx, FeatIx, SpellIx, ItemIx; // normalised name -> index
	};
	FLib& L() { static FLib X; return X; }

	FString Key(const FString& S)
	{
		FString O;
		O.Reserve(S.Len());
		for (TCHAR Ch : S.ToLower())
			if (FChar::IsAlnum(Ch)) O.AppendChar(Ch);
		return O;
	}
	FString Str(const TSharedPtr<FJsonObject>& O, const TCHAR* K) { FString S; if (O) O->TryGetStringField(K, S); return S; }
	int32 Int(const TSharedPtr<FJsonObject>& O, const TCHAR* K) { double V = 0; if (O) O->TryGetNumberField(K, V); return (int32)V; }
	float Num(const TSharedPtr<FJsonObject>& O, const TCHAR* K) { double V = 0; if (O) O->TryGetNumberField(K, V); return (float)V; }
	bool Bool(const TSharedPtr<FJsonObject>& O, const TCHAR* K) { bool V = false; if (O) O->TryGetBoolField(K, V); return V; }
	TSharedPtr<FJsonObject> Obj(const TSharedPtr<FJsonObject>& O, const TCHAR* K) { const TSharedPtr<FJsonObject>* P = nullptr; return O && O->TryGetObjectField(K, P) ? *P : nullptr; }
	const TArray<TSharedPtr<FJsonValue>>& Arr(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		static const TArray<TSharedPtr<FJsonValue>> None;
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		return O && O->TryGetArrayField(K, A) ? *A : None;
	}
	TArray<FString> Strs(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		TArray<FString> Out;
		for (const TSharedPtr<FJsonValue>& V : Arr(O, K)) { FString S; if (V->TryGetString(S)) Out.Add(S); }
		return Out;
	}
	TArray<int32> Ints(const TSharedPtr<FJsonObject>& O, const TCHAR* K)
	{
		TArray<int32> Out;
		for (const TSharedPtr<FJsonValue>& V : Arr(O, K)) Out.Add((int32)V->AsNumber());
		return Out;
	}
	TMap<FString, int32> IntMap(const TSharedPtr<FJsonObject>& O)
	{
		TMap<FString, int32> M;
		if (O) for (const auto& P : O->Values) { double V = 0; if (P.Value->TryGetNumber(V)) M.Add(FString(P.Key), (int32)V); }
		return M;
	}
	void Base(FLibEntry& E, const TSharedPtr<FJsonObject>& O)
	{
		E.Name = Str(O, TEXT("name")); E.Text = Str(O, TEXT("text")); E.Rarity = Str(O, TEXT("rarity"));
		E.Level = Int(O, TEXT("level")); E.Traits = Strs(O, TEXT("traits")); E.Tags = Strs(O, TEXT("tags"));
		E.Parent = Str(O, TEXT("ancestry")); E.Skills = Strs(O, TEXT("skills"));
		auto Slots = [&](const TCHAR* K, TArray<TArray<FString>>& Into)
		{
			const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
			if (O && O->TryGetArrayField(K, A))
				for (const TSharedPtr<FJsonValue>& V : *A)
				{
					TArray<FString>& Slot = Into.AddDefaulted_GetRef();
					const TArray<TSharedPtr<FJsonValue>>* In = nullptr;
					if (V->TryGetArray(In)) for (const TSharedPtr<FJsonValue>& X : *In) Slot.Add(X->AsString());
				}
		};
		Slots(TEXT("boosts"), E.Boosts); Slots(TEXT("flaws"), E.Flaws);
		const TArray<TSharedPtr<FJsonValue>>* Rs = nullptr;
		if (O && O->TryGetArrayField(TEXT("rules"), Rs)) for (const TSharedPtr<FJsonValue>& V : *Rs) if (V->AsObject()) E.Rules.Add(V->AsObject());
	}
	template <typename T> void Index(const TArray<T>& A, TMap<FString, int32>& Ix)
	{
		for (int32 i = 0; i < A.Num(); ++i) Ix.FindOrAdd(Key(A[i].Name), i);
	}
	// Pathmuncher-style cleanup of a Pathbuilder name: drop "(+1 Striking)" style suffixes and " - Melee" tails.
	FString Clean(const FString& S)
	{
		FString O = S;
		int32 Cut;
		if (O.FindLastChar(TEXT('('), Cut) && Cut > 0) O = O.Left(Cut);
		if ((Cut = O.Find(TEXT(" - "))) != INDEX_NONE) O = O.Left(Cut);
		return O.TrimStartAndEnd();
	}
	template <typename T> const T* Find(const TArray<T>& A, const TMap<FString, int32>& Ix, const TMap<FString, FString>& Ren, const FString& Name)
	{
		if (!HollowLib::Load() || Name.IsEmpty()) return nullptr;
		for (const FString& Try : { Name, Clean(Name) })
		{
			FString K = Key(Try);
			if (const FString* R = Ren.Find(K)) K = Key(*R);
			if (const int32* I = Ix.Find(K)) return &A[*I];
		}
		return nullptr;
	}
}

namespace HollowLib
{
FString Path() { return FPaths::Combine(FPaths::ProjectDir(), TEXT("ExternalAssets/Library/library.json")); }
bool IsLoaded() { return L().bOk; }
FString Slug(const FString& Name) { return Key(Name); }

bool Load()
{
	FLib& X = L();
	if (X.bTried) return X.bOk;
	X.bTried = true;
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *Path())) { UE_LOG(LogTemp, Warning, TEXT("Hollow library not found at %s (run Tools/BuildLibrary.mjs)"), *Path()); return false; }
	TSharedPtr<FJsonObject> R;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), R) || !R) { UE_LOG(LogTemp, Warning, TEXT("Hollow library is not valid JSON")); return false; }

	for (const auto& V : Arr(R, TEXT("classes")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FLibClass& C = X.Classes.AddDefaulted_GetRef();
		Base(C, O);
		C.Hp = Int(O, TEXT("hp")); C.Perception = Int(O, TEXT("perception")); C.Spellcasting = Int(O, TEXT("spellcasting"));
		C.AdditionalSkills = Int(O, TEXT("additionalSkills"));
		C.KeyAbility = Strs(O, TEXT("keyAbility")); C.TrainedSkills = Strs(O, TEXT("trainedSkills"));
		C.Saves = IntMap(Obj(O, TEXT("saves"))); C.Attacks = IntMap(Obj(O, TEXT("attacks"))); C.Defenses = IntMap(Obj(O, TEXT("defenses")));
		C.SkillIncrease = Ints(O, TEXT("skillIncreaseLevels")); C.ClassFeat = Ints(O, TEXT("classFeatLevels"));
		C.GeneralFeat = Ints(O, TEXT("generalFeatLevels")); C.AncestryFeat = Ints(O, TEXT("ancestryFeatLevels")); C.SkillFeat = Ints(O, TEXT("skillFeatLevels"));
		for (const auto& F : Arr(O, TEXT("features"))) C.Features.Add({ Int(F->AsObject(), TEXT("level")), Str(F->AsObject(), TEXT("name")) });
	}
	for (const auto& V : Arr(R, TEXT("ancestries")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FLibAncestry& A = X.Ancestries.AddDefaulted_GetRef();
		Base(A, O);
		A.Hp = Int(O, TEXT("hp")); A.Speed = Int(O, TEXT("speed")); A.Size = Str(O, TEXT("size")); A.Vision = Str(O, TEXT("vision"));
	}
	auto Plain = [&](const TCHAR* K, TArray<FLibEntry>& Into) { for (const auto& V : Arr(R, K)) Base(Into.AddDefaulted_GetRef(), V->AsObject()); };
	Plain(TEXT("heritages"), X.Heritages); Plain(TEXT("backgrounds"), X.Backgrounds); Plain(TEXT("classFeatures"), X.ClassFeatures);
	Plain(TEXT("conditions"), X.Conditions); Plain(TEXT("actions"), X.Actions); Plain(TEXT("deities"), X.Deities);
	for (const auto& V : Arr(R, TEXT("feats")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FLibFeat& F = X.Feats.AddDefaulted_GetRef();
		Base(F, O);
		F.Category = Str(O, TEXT("category")); F.Action = Str(O, TEXT("action")); F.Actions = Int(O, TEXT("actions")); F.Prereq = Strs(O, TEXT("prereq"));
	}
	for (const auto& V : Arr(R, TEXT("spells")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FLibSpell& S = X.Spells.AddDefaulted_GetRef();
		Base(S, O);
		S.Rank = Int(O, TEXT("rank")); S.bCantrip = Bool(O, TEXT("cantrip")); S.bFocus = Bool(O, TEXT("focus"));
		S.Cast = Str(O, TEXT("cast")); S.Range = Str(O, TEXT("range")); S.Target = Str(O, TEXT("target")); S.Duration = Str(O, TEXT("duration"));
		S.Traditions = Strs(O, TEXT("traditions"));
		if (const TSharedPtr<FJsonObject> A = Obj(O, TEXT("area"))) { S.AreaType = Str(A, TEXT("type")); S.AreaSize = Int(A, TEXT("size")); }
		if (const TSharedPtr<FJsonObject> D = Obj(O, TEXT("defense"))) { S.Save = Str(D, TEXT("save")); S.Vs = Str(D, TEXT("vs")); S.bBasic = Bool(D, TEXT("basic")); }
		for (const auto& D : Arr(O, TEXT("damage")))
		{
			const TSharedPtr<FJsonObject> Dd = D->AsObject();
			S.Damage.Add({ Str(Dd, TEXT("formula")), Str(Dd, TEXT("type")) });
			S.bHeal |= Bool(Dd, TEXT("healing"));
		}
		if (const TSharedPtr<FJsonObject> H = Obj(O, TEXT("heighten"))) { S.Interval = Int(H, TEXT("interval")); S.HeightenDamage = Strs(H, TEXT("damage")); }
	}
	for (const auto& V : Arr(R, TEXT("items")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FLibItem& I = X.Items.AddDefaulted_GetRef();
		Base(I, O);
		I.Type = Str(O, TEXT("type")); I.Usage = Str(O, TEXT("usage")); I.Price = Num(O, TEXT("price")); I.Bulk = Num(O, TEXT("bulk"));
		I.Die = Str(O, TEXT("die")); I.Dice = FMath::Max(1, Int(O, TEXT("dice"))); I.DmgType = Str(O, TEXT("dmgType"));
		I.Group = Str(O, TEXT("group")); I.Category = Str(O, TEXT("category"));
		I.Ac = Int(O, TEXT("ac")); I.Str = Int(O, TEXT("str")); I.CheckPenalty = Int(O, TEXT("checkPenalty")); I.Range = Int(O, TEXT("range"));
		I.DexCap = O->HasField(TEXT("dexCap")) ? Int(O, TEXT("dexCap")) : 99;
	}
	for (const auto& V : Arr(R, TEXT("journals")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FLibJournal& J = X.Journals.AddDefaulted_GetRef();
		J.Name = Str(O, TEXT("name"));
		for (const auto& P : Arr(O, TEXT("pages"))) J.Pages.Add({ Str(P->AsObject(), TEXT("name")), Str(P->AsObject(), TEXT("text")) });
	}
	for (const auto& V : Arr(R, TEXT("npcs")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FLibNpc& N = X.Npcs.AddDefaulted_GetRef();
		Base(N, O);
		N.Hp = Int(O, TEXT("hp")); N.Ac = Int(O, TEXT("ac")); N.Per = Int(O, TEXT("perception"));
		const TSharedPtr<FJsonObject> Sv = Obj(O, TEXT("saves"));
		N.Fort = Int(Sv, TEXT("fort")); N.Ref = Int(Sv, TEXT("ref")); N.Will = Int(Sv, TEXT("will"));
		for (const auto& St : Arr(O, TEXT("strikes"))) N.Strikes.Add({ Str(St->AsObject(), TEXT("name")), Str(St->AsObject(), TEXT("damage")) });
	}
	for (const auto& V : Arr(R, TEXT("scenes")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FLibScene& S = X.Scenes.AddDefaulted_GetRef();
		S.Name = Str(O, TEXT("name")); S.Width = Int(O, TEXT("width")); S.Height = Int(O, TEXT("height"));
		S.Grid = FMath::Max(1, Int(O, TEXT("grid"))); S.Darkness = Num(O, TEXT("darkness"));
		for (const auto& W : Arr(O, TEXT("walls")))
		{
			const TSharedPtr<FJsonObject> Wo = W->AsObject();
			const TArray<int32> C = Ints(Wo, TEXT("c"));
			if (C.Num() < 4) continue;
			FLibWall& Wl = S.Walls.AddDefaulted_GetRef();
			Wl.C = FVector4(C[0], C[1], C[2], C[3]);
			Wl.Move = Int(Wo, TEXT("move")); Wl.Sight = Int(Wo, TEXT("sight")); Wl.Light = Int(Wo, TEXT("light")); Wl.Door = Int(Wo, TEXT("door"));
		}
		for (const auto& Lt : Arr(O, TEXT("lights")))
		{
			const TSharedPtr<FJsonObject> Lo = Lt->AsObject();
			S.Lights.Add({ FVector2D(Num(Lo, TEXT("x")), Num(Lo, TEXT("y"))), Num(Lo, TEXT("bright")), Num(Lo, TEXT("dim")), Str(Lo, TEXT("color")) });
		}
	}
	for (const auto& V : Arr(R, TEXT("choices")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FLibChoice& C = X.Choices.AddDefaulted_GetRef();
		C.Feature = Str(O, TEXT("feature")); C.Class = Str(O, TEXT("class")); C.Flag = Str(O, TEXT("flag")); C.Level = Int(O, TEXT("level"));
		for (const auto& Op : Arr(O, TEXT("options"))) Base(C.Options.AddDefaulted_GetRef(), Op->AsObject());
	}
	for (const auto& V : Arr(R, TEXT("bestiary")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FLibCreature& Cr = X.Bestiary.AddDefaulted_GetRef();
		Base(Cr, O);
		Cr.Source = Str(O, TEXT("source")); Cr.Size = Str(O, TEXT("size"));
		Cr.Hp = Int(O, TEXT("hp")); Cr.Ac = Int(O, TEXT("ac")); Cr.Per = Int(O, TEXT("perception")); Cr.Speed = Int(O, TEXT("speed"));
		const TSharedPtr<FJsonObject> Sv = Obj(O, TEXT("saves"));
		Cr.Fort = Int(Sv, TEXT("fort")); Cr.Ref = Int(Sv, TEXT("ref")); Cr.Will = Int(Sv, TEXT("will"));
		Cr.Senses = Strs(O, TEXT("senses")); Cr.Immunities = Strs(O, TEXT("immunities")); Cr.Weaknesses = Strs(O, TEXT("weaknesses"));
		Cr.Resistances = Strs(O, TEXT("resistances")); Cr.Spells = Strs(O, TEXT("spells"));
		for (const auto& St : Arr(O, TEXT("strikes")))
		{
			const TSharedPtr<FJsonObject> So = St->AsObject();
			FLibStrike& S = Cr.Strikes.AddDefaulted_GetRef();
			S.Name = Str(So, TEXT("name")); S.Bonus = Int(So, TEXT("bonus")); S.bRanged = Bool(So, TEXT("ranged")); S.Traits = Strs(So, TEXT("traits"));
			for (const auto& D : Arr(So, TEXT("damage"))) S.Damage.Add({ Str(D->AsObject(), TEXT("formula")), Str(D->AsObject(), TEXT("type")) });
		}
		for (const auto& Ab : Arr(O, TEXT("abilities")))
		{
			const TSharedPtr<FJsonObject> Ao = Ab->AsObject();
			FLibFeat& F = Cr.Abilities.AddDefaulted_GetRef();
			Base(F, Ao);
			F.Action = Str(Ao, TEXT("action")); F.Actions = Int(Ao, TEXT("actions"));
		}
	}
	for (const auto& V : Arr(R, TEXT("hazards")))
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		FLibHazard& Hz = X.Hazards.AddDefaulted_GetRef();
		Base(Hz, O);
		Hz.Stealth = Int(O, TEXT("stealth")); Hz.Ac = Int(O, TEXT("ac")); Hz.Hp = Int(O, TEXT("hp"));
		Hz.Disable = Str(O, TEXT("disable")); Hz.Reset = Str(O, TEXT("reset"));
	}
	if (const TSharedPtr<FJsonObject> Rn = Obj(R, TEXT("renames")))
	{
		auto Ren = [&](const TCHAR* K, TMap<FString, FString>& M) { for (const auto& E : Arr(Rn, K)) M.Add(Key(Str(E->AsObject(), TEXT("pb"))), Str(E->AsObject(), TEXT("foundry"))); };
		Ren(TEXT("equipment"), X.RenEquip); Ren(TEXT("feats"), X.RenFeat); Ren(TEXT("spells"), X.RenSpell);
	}
	Index(X.ClassFeatures, X.CfIx); Index(X.Bestiary, X.CreatureIx);
	Index(X.Classes, X.ClassIx); Index(X.Ancestries, X.AncIx); Index(X.Feats, X.FeatIx); Index(X.Spells, X.SpellIx); Index(X.Items, X.ItemIx);
	X.bOk = true;
	UE_LOG(LogTemp, Display, TEXT("Hollow library: %d classes, %d ancestries, %d feats, %d spells, %d items, %d journals, %d scenes"),
		X.Classes.Num(), X.Ancestries.Num(), X.Feats.Num(), X.Spells.Num(), X.Items.Num(), X.Journals.Num(), X.Scenes.Num());
	return true;
}

const TArray<FLibClass>& Classes() { Load(); return L().Classes; }
const TArray<FLibAncestry>& Ancestries() { Load(); return L().Ancestries; }
const TArray<FLibEntry>& Heritages() { Load(); return L().Heritages; }
const TArray<FLibEntry>& Backgrounds() { Load(); return L().Backgrounds; }
const TArray<FLibFeat>& Feats() { Load(); return L().Feats; }
const TArray<FLibEntry>& ClassFeatures() { Load(); return L().ClassFeatures; }
const TArray<FLibSpell>& Spells() { Load(); return L().Spells; }
const TArray<FLibItem>& Items() { Load(); return L().Items; }
const TArray<FLibEntry>& Conditions() { Load(); return L().Conditions; }
const TArray<FLibEntry>& Actions() { Load(); return L().Actions; }
const TArray<FLibEntry>& Deities() { Load(); return L().Deities; }
const TArray<FLibJournal>& Journals() { Load(); return L().Journals; }
const TArray<FLibScene>& Scenes() { Load(); return L().Scenes; }
const TArray<FLibNpc>& Npcs() { Load(); return L().Npcs; }
const TArray<FLibChoice>& Choices() { Load(); return L().Choices; }
const TArray<FLibCreature>& Bestiary() { Load(); return L().Bestiary; }
const TArray<FLibHazard>& Hazards() { Load(); return L().Hazards; }
const FLibEntry* ClassFeature(const FString& N) { static const TMap<FString, FString> None; return Find(L().ClassFeatures, L().CfIx, None, N); }
const FLibCreature* Creature(const FString& N) { static const TMap<FString, FString> None; return Find(L().Bestiary, L().CreatureIx, None, N); }

TArray<const FLibChoice*> ChoicesFor(const FString& ClassName, int32 Level)
{
	TArray<const FLibChoice*> Out;
	const FString C = ClassName.ToLower();
	for (const FLibChoice& Ch : Choices()) if (Ch.Class == C && Ch.Level == Level) Out.Add(&Ch);
	return Out;
}

const TArray<const FLibEntry*> HeritagesOf(const FString& Ancestry)
{
	TArray<const FLibEntry*> Out;
	for (const FLibEntry& H : Heritages()) if (H.Parent.IsEmpty() || H.Parent.Equals(Ancestry, ESearchCase::IgnoreCase)) Out.Add(&H);
	return Out;
}

const FLibClass* Class(const FString& N) { static const TMap<FString, FString> None; return Find(L().Classes, L().ClassIx, None, N); }
const FLibAncestry* Ancestry(const FString& N) { static const TMap<FString, FString> None; return Find(L().Ancestries, L().AncIx, None, N); }
const FLibFeat* Feat(const FString& N) { return Find(L().Feats, L().FeatIx, L().RenFeat, N); }
const FLibSpell* Spell(const FString& N) { return Find(L().Spells, L().SpellIx, L().RenSpell, N); }
const FLibItem* Item(const FString& N) { return Find(L().Items, L().ItemIx, L().RenEquip, N); }

FString HeightenedFormula(const FLibSpell& S, int32 AtRank, int32 Ix)
{
	if (!S.Damage.IsValidIndex(Ix)) return FString();
	FString F = S.Damage[Ix].Key;
	const int32 Steps = S.Interval > 0 && S.HeightenDamage.IsValidIndex(Ix) ? FMath::Max(0, (AtRank - FMath::Max(1, S.Rank)) / S.Interval) : 0;
	if (!Steps) return F;
	// "6d6" + 3 x "2d6" = "12d6"; mismatched dice are appended as "+NdX"
	auto Split = [](const FString& E, int32& N, FString& Die, FString& Rest)
	{
		int32 D;
		if (!E.FindChar(TEXT('d'), D)) { N = 0; Die.Reset(); Rest = E; return; }
		N = FCString::Atoi(*E.Left(D));
		int32 End = D + 1;
		while (End < E.Len() && FChar::IsDigit(E[End])) End++;
		Die = E.Mid(D, End - D); Rest = E.Mid(End);
	};
	int32 Nb, Ni; FString Db, Di, Rb, Ri;
	Split(F, Nb, Db, Rb); Split(S.HeightenDamage[Ix], Ni, Di, Ri);
	if (Db == Di && !Db.IsEmpty()) return FString::Printf(TEXT("%d%s%s"), Nb + Ni * Steps, *Db, *Rb);
	return F + FString::Printf(TEXT("+%d%s"), Ni * Steps, *Di);
}

FString SpellSummary(const FLibSpell& S, int32 AtRank)
{
	TArray<FString> P;
	for (int32 i = 0; i < S.Damage.Num(); ++i) P.Add(FString::Printf(TEXT("%s %s"), *HeightenedFormula(S, AtRank, i), S.bHeal ? TEXT("healing") : *S.Damage[i].Value));
	if (!S.Save.IsEmpty()) P.Add(FString::Printf(TEXT("%s%s"), *S.Save.Left(1).ToUpper(), *(S.Save.Mid(1) + (S.bBasic ? TEXT(" (basic)") : TEXT("")))));
	else if (S.Vs == TEXT("ac")) P.Add(TEXT("spell attack"));
	if (!S.AreaType.IsEmpty()) P.Add(FString::Printf(TEXT("%d-ft %s"), S.AreaSize, *S.AreaType));
	if (!S.Duration.IsEmpty()) P.Add(S.Duration);
	return FString::Join(P, TEXT("  -  "));
}
}

int32 FLibSpell::Actions() const
{
	const int32 N = FCString::Atoi(*Cast);
	if (N >= 1) return FMath::Clamp(Cast.Contains(TEXT("to 3")) ? 2 : N, 1, 3);
	return Cast.Contains(TEXT("minute")) || Cast.Contains(TEXT("hour")) ? 3 : 1;
}
