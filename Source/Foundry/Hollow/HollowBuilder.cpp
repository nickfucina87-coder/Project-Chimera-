// The character builder: every ancestry, heritage, background, class and subclass in the rules library, built the
// pf2e way (remaster boosts, trained skills, level-1 feats, spells, starting gear) and turned into the hero's sheet.
// Pathbuilder import and the four quick-start kits still work; this is the full path.
#include "HollowGameMode.h"

#include "HollowLibrary.h"
#include "HollowRules.h"

using namespace HollowRules;

namespace
{
	const TArray<FString>& Abils() { static const TArray<FString> A = { TEXT("str"), TEXT("dex"), TEXT("con"), TEXT("int"), TEXT("wis"), TEXT("cha") }; return A; }
	const TMap<FString, FString>& SaveKey() { static const TMap<FString, FString> M = { { TEXT("fortitude"), TEXT("fort") }, { TEXT("reflex"), TEXT("ref") }, { TEXT("will"), TEXT("will") } }; return M; }

	// The nearest quick-start kit, for the starter card deck and the chibi's look until real art exists.
	FString KitFor(const FLibClass* C)
	{
		if (!C) return TEXT("fighter");
		const FString N = C->Name.ToLower();
		if (N == TEXT("fighter") || N == TEXT("rogue") || N == TEXT("wizard") || N == TEXT("cleric")) return N;
		static const TSet<FString> Divine = { TEXT("druid"), TEXT("oracle"), TEXT("champion"), TEXT("animist") };
		static const TSet<FString> Arcane = { TEXT("sorcerer"), TEXT("witch"), TEXT("bard"), TEXT("psychic"), TEXT("summoner"), TEXT("magus"), TEXT("kineticist") };
		static const TSet<FString> Sly = { TEXT("investigator"), TEXT("swashbuckler"), TEXT("ranger"), TEXT("gunslinger"), TEXT("thaumaturge") };
		if (Divine.Contains(N)) return TEXT("cleric");
		if (Arcane.Contains(N)) return TEXT("wizard");
		if (Sly.Contains(N)) return TEXT("rogue");
		return TEXT("fighter");
	}
}

void AHollowGameMode::OpenBuilder()
{
	if (Mode != EHollowMode::Create) return;
	if (!HollowLib::Load()) { Toast(TEXT("The rules library is missing. Run: node Tools/BuildLibrary.mjs")); return; }
	Draft = FBuildDraft();
	Draft.Name = CreateName;
	bBuilder = true;
	PlaySfx(TEXT("book"), 0.7f);
	MarkUI();
}

FString AHollowGameMode::DraftTradition() const
{
	static const TMap<FString, FString> ByClass = { { TEXT("Wizard"), TEXT("arcane") }, { TEXT("Magus"), TEXT("arcane") }, { TEXT("Cleric"), TEXT("divine") },
		{ TEXT("Druid"), TEXT("primal") }, { TEXT("Bard"), TEXT("occult") }, { TEXT("Psychic"), TEXT("occult") }, { TEXT("Oracle"), TEXT("divine") },
		{ TEXT("Animist"), TEXT("divine") }, { TEXT("Summoner"), TEXT("arcane") } };
	const FLibClass* C = HollowLib::Class(Draft.Class);
	if (!C || C->Spellcasting <= 0) return FString();
	// sorcerer bloodlines and witch patrons name their tradition in their text
	for (const auto& P : Draft.Picks)
		for (const FLibChoice& Ch : HollowLib::Choices())
			if (Ch.Feature == P.Key)
				for (const FLibEntry& O : Ch.Options)
					if (O.Name == P.Value)
						for (const TCHAR* T : { TEXT("arcane"), TEXT("divine"), TEXT("occult"), TEXT("primal") })
							if (O.Text.ToLower().Contains(T)) return T;
	const FString* T = ByClass.Find(C->Name);
	return T ? *T : FString(TEXT("arcane"));
}

int32 AHollowGameMode::DraftSkillPicks() const
{
	const FLibClass* C = HollowLib::Class(Draft.Class);
	if (!C) return 0;
	const FLibEntry* Bg = HollowLib::Backgrounds().FindByPredicate([&](const FLibEntry& B) { return B.Name == Draft.Background; });
	int32 Overlap = 0;
	if (Bg) for (const FString& S : Bg->Skills) Overlap += C->TrainedSkills.Contains(S) ? 1 : 0;
	return FMath::Max(0, C->AdditionalSkills + DraftAbil(TEXT("int")) + Overlap);
}

int32 AHollowGameMode::DraftAbil(const FString& A) const
{
	int32 V = 0;
	if (const FLibAncestry* An = HollowLib::Ancestry(Draft.Ancestry))
	{
		for (const TArray<FString>& Slot : An->Boosts) if (Slot.Num() == 1 && Slot[0] == A) V++;
		for (const TArray<FString>& Slot : An->Flaws) if (Slot.Num() == 1 && Slot[0] == A) V--;
	}
	for (const TArray<FString>* L : { &Draft.AncBoosts, &Draft.BgBoosts, &Draft.FreeBoosts }) V += L->FilterByPredicate([&](const FString& X) { return X == A; }).Num();
	if (Draft.KeyAbility == A) V++;
	return FMath::Min(V, 4);
}

TArray<FString> AHollowGameMode::BuilderPending() const
{
	TArray<FString> P;
	const FLibClass* C = HollowLib::Class(Draft.Class);
	const FLibAncestry* An = HollowLib::Ancestry(Draft.Ancestry);
	if (!An) P.Add(TEXT("Choose an ancestry"));
	else
	{
		if (Draft.Heritage.IsEmpty()) P.Add(TEXT("Choose a heritage"));
		const int32 FreeSlots = An->Boosts.FilterByPredicate([](const TArray<FString>& S) { return S.Num() > 1; }).Num();
		if (Draft.AncBoosts.Num() < FreeSlots) P.Add(FString::Printf(TEXT("Ancestry boosts: %d more"), FreeSlots - Draft.AncBoosts.Num()));
	}
	if (Draft.Background.IsEmpty()) P.Add(TEXT("Choose a background"));
	else if (Draft.BgBoosts.Num() < 2) P.Add(FString::Printf(TEXT("Background boosts: %d more"), 2 - Draft.BgBoosts.Num()));
	else if (const FLibEntry* Bg = HollowLib::Backgrounds().FindByPredicate([&](const FLibEntry& B) { return B.Name == Draft.Background; }))
		if (Bg->Boosts.Num() && Bg->Boosts[0].Num() < 6 && !Draft.BgBoosts.ContainsByPredicate([&](const FString& X) { return Bg->Boosts[0].Contains(X); }))
			P.Add(FString::Printf(TEXT("One background boost must be %s"), *FString::Join(Bg->Boosts[0], TEXT(" or ")).ToUpper()));
	if (!C) P.Add(TEXT("Choose a class"));
	else
	{
		if (Draft.KeyAbility.IsEmpty()) P.Add(TEXT("Choose your key attribute"));
		for (const FLibChoice* Ch : HollowLib::ChoicesFor(C->Name, 1)) if (!Draft.Picks.Contains(Ch->Feature)) P.Add(TEXT("Choose: ") + Ch->Feature);
		if (Draft.Skills.Num() < DraftSkillPicks()) P.Add(FString::Printf(TEXT("Skills: %d more"), DraftSkillPicks() - Draft.Skills.Num()));
		if (C->ClassFeat.Contains(1) && Draft.ClassFeat.IsEmpty()) P.Add(TEXT("Choose a class feat"));
		if (!DraftTradition().IsEmpty())
		{
			if (Draft.Cantrips.Num() < 5) P.Add(FString::Printf(TEXT("Cantrips: %d more"), 5 - Draft.Cantrips.Num()));
			if (Draft.Spells.Num() < 2) P.Add(FString::Printf(TEXT("Rank 1 spells: %d more"), 2 - Draft.Spells.Num()));
		}
	}
	if (Draft.FreeBoosts.Num() < 4) P.Add(FString::Printf(TEXT("Free boosts: %d more"), 4 - Draft.FreeBoosts.Num()));
	if (An && Draft.AncFeat.IsEmpty()) P.Add(TEXT("Choose an ancestry feat"));
	if (Draft.Name.TrimStartAndEnd().IsEmpty()) P.Add(TEXT("Name your hero"));
	return P;
}

void AHollowGameMode::BuilderPick(const FString& Field, const FString& Value)
{
	auto Toggle = [&](TArray<FString>& L, int32 Max)
	{
		if (L.Remove(Value) == 0 && L.Num() < Max) L.Add(Value);
	};
	if (Field == TEXT("ancestry")) { if (Draft.Ancestry != Value) { Draft.Ancestry = Value; Draft.Heritage.Reset(); Draft.AncBoosts.Reset(); Draft.AncFeat.Reset(); } }
	else if (Field == TEXT("heritage")) Draft.Heritage = Value;
	else if (Field == TEXT("background")) { if (Draft.Background != Value) { Draft.Background = Value; Draft.BgBoosts.Reset(); } }
	else if (Field == TEXT("class"))
	{
		if (Draft.Class != Value)
		{
			Draft.Class = Value; Draft.Picks.Reset(); Draft.Skills.Reset(); Draft.ClassFeat.Reset(); Draft.Cantrips.Reset(); Draft.Spells.Reset();
			Draft.Weapon.Reset(); Draft.Armor.Reset();
			const FLibClass* C = HollowLib::Class(Value);
			Draft.KeyAbility = C && C->KeyAbility.Num() == 1 ? C->KeyAbility[0] : FString();
		}
	}
	else if (Field == TEXT("key")) Draft.KeyAbility = Value;
	else if (Field.StartsWith(TEXT("pick:"))) { const FString F = Field.Mid(5); if (Draft.Picks.FindRef(F) == Value) Draft.Picks.Remove(F); else Draft.Picks.Add(F, Value); }
	else if (Field == TEXT("ancboost")) Toggle(Draft.AncBoosts, 2);
	else if (Field == TEXT("bgboost")) Toggle(Draft.BgBoosts, 2);
	else if (Field == TEXT("freeboost")) Toggle(Draft.FreeBoosts, 4);
	else if (Field == TEXT("skill")) Toggle(Draft.Skills, DraftSkillPicks());
	else if (Field == TEXT("ancfeat")) Draft.AncFeat = Draft.AncFeat == Value ? FString() : Value;
	else if (Field == TEXT("classfeat")) Draft.ClassFeat = Draft.ClassFeat == Value ? FString() : Value;
	else if (Field == TEXT("cantrip")) Toggle(Draft.Cantrips, 5);
	else if (Field == TEXT("spell")) Toggle(Draft.Spells, 2);
	else if (Field == TEXT("weapon")) Draft.Weapon = Draft.Weapon == Value ? FString() : Value;
	else if (Field == TEXT("armor")) Draft.Armor = Draft.Armor == Value ? FString() : Value;
	PlaySfx(TEXT("click"), 0.4f);
	MarkUI();
}

FPC AHollowGameMode::BuilderPC() const
{
	const FLibClass* C = HollowLib::Class(Draft.Class);
	const FLibAncestry* An = HollowLib::Ancestry(Draft.Ancestry);
	FString AncId = TEXT("human");
	for (const FAncestry& A : Ancestries()) if (Draft.Ancestry.ToLower().Contains(A.Id)) AncId = A.Id;
	FPC P = BuildPC(Draft.Name.TrimStartAndEnd(), AncId, KitFor(C));
	P.Ancestry = Draft.Ancestry.IsEmpty() ? P.Ancestry : Draft.Ancestry;
	P.ClassName = C ? C->Name : P.ClassName;
	P.Heritage = Draft.Heritage; P.Background = Draft.Background;
	for (const FString& A : Abils()) P.Abil.Add(A, DraftAbil(A));
	if (!C) return P;

	// proficiencies from the class
	P.Ranks.Reset();
	for (const FString& S : C->TrainedSkills) if (SkillList().Contains(S)) P.Ranks.Add(S, 1);
	if (const FLibEntry* Bg = HollowLib::Backgrounds().FindByPredicate([&](const FLibEntry& B) { return B.Name == Draft.Background; }))
		for (const FString& S : Bg->Skills) if (SkillList().Contains(S)) P.Ranks.Add(S, 1);
	for (const FString& S : Draft.Skills) P.Ranks.Add(S, 1);
	P.PerRank = FMath::Max(1, C->Perception);
	P.Saves.Reset();
	for (const auto& S : C->Saves) if (const FString* K = SaveKey().Find(S.Key)) P.Saves.Add(*K, S.Value);

	// hit points: ancestry + class + Con
	const int32 Con = P.Abil.FindRef(TEXT("con"));
	P.HpPer = C->Hp + Con;
	P.HpMax = (An ? An->Hp : 8) + P.HpPer;
	P.Hp = P.HpMax;
	P.Speed = An ? An->Speed : 25;

	// gear: the chosen weapon and armor from the library, the rest of 15 gp as silver
	int32 Silver = 150;
	P.Inv.Init();
	P.Inv.Bags[0].Slots.SetNum(16);
	if (const FLibItem* W = HollowLib::Item(Draft.Weapon))
	{
		P.WeaponName = W->Name;
		P.WeaponDie = W->Die.StartsWith(TEXT("d")) ? FCString::Atoi(*W->Die.Mid(1)) : 6;
		const bool bDex = W->Traits.Contains(TEXT("finesse")) || W->Range > 0;
		P.WeaponAbil = bDex && P.Abil.FindRef(TEXT("dex")) > P.Abil.FindRef(TEXT("str")) ? TEXT("dex") : TEXT("str");
		P.WeaponRank = FMath::Max(1, C->Attacks.FindRef(W->Category));
		Silver -= FMath::RoundToInt(W->Price);
		P.Inv.Add(HollowItems::FromLibrary(W->Name), 1);
	}
	else { P.WeaponName = TEXT("Fist"); P.WeaponDie = 4; P.WeaponAbil = TEXT("str"); P.WeaponRank = FMath::Max(1, C->Attacks.FindRef(TEXT("unarmed"))); }
	const FLibItem* Ar = HollowLib::Item(Draft.Armor);
	const int32 Dex = P.Abil.FindRef(TEXT("dex"));
	if (Ar)
	{
		P.ArmorName = Ar->Name;
		P.AcItem = 10 + FMath::Min(Dex, Ar->DexCap) + Ar->Ac;
		P.ArmorRank = FMath::Max(1, C->Defenses.FindRef(Ar->Category));
		Silver -= FMath::RoundToInt(Ar->Price);
		P.Inv.Add(HollowItems::FromLibrary(Ar->Name), 1);
	}
	else { P.ArmorName = TEXT("Clothes"); P.AcItem = 10 + Dex; P.ArmorRank = FMath::Max(1, C->Defenses.FindRef(TEXT("unarmored"))); }
	P.Gold = FMath::Max(0, Silver);
	P.Inv.Add(TEXT("ration"), 4); P.Inv.Add(TEXT("torch"), 2); P.Inv.Add(TEXT("potion"), 1);

	// spellcasting
	const FString Trad = DraftTradition();
	P.bCaster = !Trad.IsEmpty();
	if (P.bCaster) { P.Tradition = Trad; P.SpellAbil = Draft.KeyAbility.IsEmpty() ? TEXT("int") : Draft.KeyAbility; P.SpellRank = 1; }
	P.Spells = Draft.Cantrips;
	P.Spells.Append(Draft.Spells);

	// feats and build choices (subclass picks are level-1 entries; the rules engine reads them all)
	P.Feats.Reset();
	for (const auto& Pk : Draft.Picks) P.Feats.Add({ 1, Pk.Value });
	if (!Draft.AncFeat.IsEmpty()) P.Feats.Add({ 1, Draft.AncFeat });
	if (!Draft.ClassFeat.IsEmpty()) P.Feats.Add({ 1, Draft.ClassFeat });

	// the deck: the nearest kit's tricks plus a card per spell (cantrips twice)
	for (const FString& Ct : Draft.Cantrips) { const FString Id = HollowData::SpellCard(Ct, 1); if (!Id.IsEmpty()) { P.Deck.Add(Id); P.Deck.Add(Id); } }
	for (const FString& Sp : Draft.Spells) { const FString Id = HollowData::SpellCard(Sp, 1); if (!Id.IsEmpty()) P.Deck.Add(Id); }
	Recompute(P);
	return P;
}

void AHollowGameMode::BuilderFinish()
{
	if (BuilderPending().Num()) { Toast(TEXT("Some choices are still open.")); return; }
	PC = BuilderPC();
	CreateName = PC.Name;
	bImported = true;
	bBuilder = false;
	PbStatus = FString::Printf(TEXT("Built %s, a %s %s %s. %d feats and choices, %d spells."), *PC.Name, *PC.Heritage, *PC.Ancestry, *PC.ClassName, PC.Feats.Num(), PC.Spells.Num());
	PlaySfx(TEXT("confirm"));
	MarkUI();
}
