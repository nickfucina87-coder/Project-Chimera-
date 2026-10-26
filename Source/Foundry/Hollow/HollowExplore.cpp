// Exploration systems layered on the director: the pause screen, inventory verbs, trading, tool-gated field spots,
// level-up choices, exploration buffs (and the aura that shows them), structured dialogue choices, and Pathbuilder import.
#include "HollowGameMode.h"

#include "Components/MaterialBillboardComponent.h"
#include "Components/PointLightComponent.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HollowArt.h"
#include "HollowHero.h"
#include "HollowRules.h"
#include "HollowLibrary.h"

using namespace HollowRules;
using HollowArt::EShape;

/* ---------- Pause screen ---------- */

void AHollowGameMode::SetPaused(bool bOn)
{
	if (bOn && Mode != EHollowMode::Explore && Mode != EHollowMode::Map && Mode != EHollowMode::Combat) return;
	if (bPaused == bOn) return;
	bPaused = bOn;
	InvSel = FIntPoint(-1, -1);
	if (Hero) Hero->StopMove();
	UGameplayStatics::SetGamePaused(this, bOn);
	PlaySfx(bOn ? TEXT("book") : TEXT("click"), 0.5f);
	MarkUI();
}

void AHollowGameMode::TogglePause(EPauseTab Tab)
{
	if (bPaused && PauseTab == Tab) { SetPaused(false); return; }
	PauseTab = Tab;
	InvSel = FIntPoint(-1, -1);
	SetPaused(true);
	MarkUI();
}

/* ---------- Inventory ---------- */

bool AHollowGameMode::Give(const FString& ItemId, int32 N, bool bToast)
{
	const FItemDef* D = HollowItems::Find(ItemId);
	if (!D || N <= 0) return false;
	int32 Left = 0;
	PC.Inv.Add(ItemId, N, &Left);
	if (Left > 0)
	{
		Toast(FString::Printf(TEXT("Your pack is full: %d %s left behind."), Left, *D->Name));
		Log(FString::Printf(TEXT("No room for %d %s."), Left, *D->Name));
	}
	else if (bToast) Toast(FString::Printf(TEXT("+%d %s"), N, *D->Name));
	RefreshFeatureGlow();
	MarkUI();
	return Left == 0;
}

void AHollowGameMode::InvClick(int32 Bag, int32 Slot)
{
	const FItemStack* Clicked = PC.Inv.At(Bag, Slot);
	if (!Clicked) return;
	const FItemStack* Sel = PC.Inv.At(InvSel.X, InvSel.Y);
	if (Sel && !Sel->IsEmpty() && InvSel != FIntPoint(Bag, Slot) && Clicked->IsEmpty())
	{
		// selected item + empty slot = move it there
		if (PC.Inv.Move(InvSel.X, InvSel.Y, Bag, Slot)) { PlaySfx(TEXT("click"), 0.5f); InvSel = FIntPoint(Bag, Slot); }
		else Toast(TEXT("That can't go there."));
	}
	else InvSel = InvSel == FIntPoint(Bag, Slot) ? FIntPoint(-1, -1) : FIntPoint(Bag, Slot);
	MarkUI();
}

void AHollowGameMode::InvUse()
{
	const FItemStack* S = PC.Inv.At(InvSel.X, InvSel.Y);
	if (!S || S->IsEmpty()) return;
	const FItemDef* D = HollowItems::Find(S->Id);
	if (!D) return;
	const FString Id = S->Id;
	if (D->Use == TEXT("heal"))
	{
		if (PC.Hp >= PC.HpMax) { Toast(TEXT("You're already at full health.")); return; }
		PC.Inv.Take(Id);
		const int32 H = FMath::Min(PC.HpMax - PC.Hp, R(TEXT("2d8+4")));
		PC.Hp += H;
		Toast(FString::Printf(TEXT("You drink the potion: +%d HP."), H));
		PlaySfx(TEXT("potion"), 0.7f);
	}
	else if (D->Use == TEXT("eat"))
	{
		if (PC.Hp >= PC.HpMax) { Toast(TEXT("You're not hungry. Save it for the road.")); return; }
		PC.Inv.Take(Id);
		const int32 H = FMath::Min(PC.HpMax - PC.Hp, HollowRules::D(6));
		PC.Hp += H;
		Toast(FString::Printf(TEXT("You eat by the roadside: +%d HP."), H));
	}
	else if (D->Grants.Num() && Hero && Mode == EHollowMode::Explore)
	{
		// use a tool on the nearest spot that needs it
		for (int32 i = 0; i < Interactables.Num(); ++i)
		{
			const FInteractable I = Interactables[i];
			if (I.Kind == TEXT("feature") && I.Actor.IsValid() && FVector::Dist2D(Hero->GetActorLocation(), I.Loc) <= I.Radius)
			{
				SetPaused(false);
				UseFeature(I);
				return;
			}
		}
		Toast(FString::Printf(TEXT("Nothing here to %s. Look for spots that glow when you carry the right tool."), *HollowItems::AbilityName(D->Grants[0]).ToLower()));
	}
	else Toast(TEXT("You can't use that right now."));
	MarkUI();
}

void AHollowGameMode::InvDrop()
{
	const FItemStack* S = PC.Inv.At(InvSel.X, InvSel.Y);
	if (!S || S->IsEmpty()) return;
	const FItemDef* D = HollowItems::Find(S->Id);
	const FString Name = D ? D->Name : S->Id;
	if (!PC.Inv.Drop(InvSel.X, InvSel.Y, 1))
	{
		Toast(D && D->Kind == EItemKind::Quest ? TEXT("You can't bring yourself to leave that behind.") : TEXT("Empty the bag before you drop it."));
		return;
	}
	if (!PC.Inv.At(InvSel.X, InvSel.Y)) InvSel = FIntPoint(-1, -1);
	Toast(FString::Printf(TEXT("Dropped %s."), *Name));
	RefreshFeatureGlow();
	MarkUI();
}

void AHollowGameMode::InvStow()
{
	const FItemStack* S = PC.Inv.At(InvSel.X, InvSel.Y);
	if (!S || S->IsEmpty() || PC.Inv.Bags.Num() < 2) return;
	const int32 To = InvSel.X == 0 ? 1 : 0;
	FItemContainer& Dest = PC.Inv.Bags[To];
	int32 Free = Dest.Slots.IndexOfByPredicate([&](const FItemStack& X) { return X.Id == S->Id; });
	if (Free == INDEX_NONE) Free = Dest.Slots.IndexOfByPredicate([](const FItemStack& X) { return X.IsEmpty(); });
	if (Free == INDEX_NONE || !PC.Inv.Move(InvSel.X, InvSel.Y, To, Free)) { Toast(FString::Printf(TEXT("No room in the %s for that."), *Dest.Name)); return; }
	InvSel = FIntPoint(-1, -1);
	PlaySfx(TEXT("click"), 0.5f);
	MarkUI();
}

void AHollowGameMode::InvSortBy(int32 By)
{
	InvSort = By;
	for (int32 b = 0; b < PC.Inv.Bags.Num(); ++b) PC.Inv.Sort(b, By);
	InvSel = FIntPoint(-1, -1);
	MarkUI();
}

/* ---------- Trade ---------- */

void AHollowGameMode::OpenTrade()
{
	if (!Talk.IsSet()) return;
	const FNpcDef* N = HollowData::FindNpc(Talk->NpcId);
	if (!N || !N->Stock.Num()) return;
	FTradeView T;
	T.NpcId = N->Id;
	T.Line = N->Id == TEXT("pip") ? TEXT("Pip has everything! Almost everything. Look, look.") : TEXT("Here's what I can spare.");
	Trade = T;
	PlaySfx(TEXT("coins"), 0.5f);
	MarkUI();
}

int32 AHollowGameMode::SellPrice(const FString& ItemId) const
{
	const FItemDef* D = HollowItems::Find(ItemId);
	if (!D || D->Kind == EItemKind::Quest || D->Price <= 0) return 0;
	return FMath::Max(1, D->Kind == EItemKind::Valuable ? D->Price * 3 / 4 : D->Price / 2);
}

void AHollowGameMode::Buy(const FString& ItemId)
{
	const FItemDef* D = HollowItems::Find(ItemId);
	if (!D || !Trade.IsSet()) return;
	if (PC.Gold < D->Price) { Trade->Line = TEXT("Silver first, then goods."); MarkUI(); return; }
	if (D->Rarity == ERarity::Rare && D->BagSlots && PC.Inv.Has(ItemId)) { Trade->Line = TEXT("You've already got one. Nobody needs two."); MarkUI(); return; }
	int32 Room = 0;
	for (const FItemContainer& B : PC.Inv.Bags) Room += B.Capacity(ItemId);
	if (Room <= 0) { Trade->Line = TEXT("Your pack's full. Sell me something first?"); MarkUI(); return; }
	PC.Gold -= D->Price;
	Give(ItemId, 1);
	Trade->Line = FString::Printf(TEXT("One %s. Pleasure doing business."), *D->Name);
	PlaySfx(TEXT("coins"), 0.6f);
	MarkUI();
}

void AHollowGameMode::Sell(const FString& ItemId)
{
	const int32 P = SellPrice(ItemId);
	if (!Trade.IsSet() || P <= 0 || !PC.Inv.Take(ItemId)) return;
	PC.Gold += P;
	if (const FItemDef* D = HollowItems::Find(ItemId)) Trade->Line = FString::Printf(TEXT("%d silver for the %s. Fair. Fair-ish."), P, *D->Name);
	PlaySfx(TEXT("coins"), 0.6f);
	RefreshFeatureGlow();
	MarkUI();
}

/* ---------- Level up and buffs ---------- */

bool FLevelUpView::IsComplete() const
{
	return Boosts.Num() == BoostsNeeded && (!bSkill || !Skill.IsEmpty()) && (!bClassFeat || !ClassFeat.IsEmpty()) && (!bSkillFeat || !SkillFeat.IsEmpty())
		&& (!bGeneralFeat || !GeneralFeat.IsEmpty()) && (!bAncestryFeat || !AncestryFeat.IsEmpty()) && NewSpells.Num() >= SpellsNeeded;
}

namespace
{
	FLevelUpView PlanLevel(const FPC& P, int32 Level, const FString& Tradition)
	{
		FLevelUpView V;
		V.Level = Level;
		const FLibClass* C = HollowLib::Class(P.ClassName);
		auto Has = [&](const TArray<int32>& L, const TArray<int32>& Def) { return (C && L.Num() ? L : Def).Contains(Level); };
		V.BoostsNeeded = Level % 5 == 0 ? 4 : 0;
		V.bSkill = Has(C ? C->SkillIncrease : TArray<int32>(), { 3, 5, 7, 9, 11, 13, 15, 17, 19 });
		V.bClassFeat = Has(C ? C->ClassFeat : TArray<int32>(), { 2, 4, 6, 8, 10, 12, 14, 16, 18, 20 });
		V.bSkillFeat = Has(C ? C->SkillFeat : TArray<int32>(), { 2, 4, 6, 8, 10, 12, 14, 16, 18, 20 });
		V.bGeneralFeat = Has(C ? C->GeneralFeat : TArray<int32>(), { 3, 7, 11, 15, 19 });
		V.bAncestryFeat = Has(C ? C->AncestryFeat : TArray<int32>(), { 5, 9, 13, 17 });
		V.SpellsNeeded = Tradition.IsEmpty() ? 0 : P.ClassName == TEXT("Wizard") ? 2 : 1;
		// without the rules library there is nothing to pick feats or spells from
		if (!HollowLib::IsLoaded()) { V.bClassFeat = V.bSkillFeat = V.bGeneralFeat = V.bAncestryFeat = false; V.SpellsNeeded = 0; }
		return V;
	}
}

FString AHollowGameMode::CasterTradition() const
{
	if (!PC.Tradition.IsEmpty()) return PC.Tradition;
	if (!PC.bCaster) return FString();
	static const TMap<FString, FString> ByClass = { { TEXT("Wizard"), TEXT("arcane") }, { TEXT("Cleric"), TEXT("divine") }, { TEXT("Druid"), TEXT("primal") },
		{ TEXT("Bard"), TEXT("occult") }, { TEXT("Witch"), TEXT("occult") }, { TEXT("Sorcerer"), TEXT("arcane") }, { TEXT("Oracle"), TEXT("divine") }, { TEXT("Psychic"), TEXT("occult") } };
	const FString* T = ByClass.Find(PC.ClassName);
	return T ? *T : FString(TEXT("arcane"));
}

void AHollowGameMode::OnLevelUp()
{
	HollowLib::Load();
	if (!LevelUp.IsSet()) LevelUp = PlanLevel(PC, PC.Level, CasterTradition());   // a later level waits for this one
	AddBuff(TEXT("resolve"), TEXT("Fresh Resolve"), TEXT("You've grown. Heal 5 HP when your next fight begins."), FLinearColor(0.55f, 0.85f, 1.f), 1);
	if (Hero)
	{
		const FVector At = Hero->GetActorLocation();
		NiagaraAt(TEXT("/Game/NiagaraExamples/FX_Player/NS_Player_Teleport_In"), At, 1.2f);
		Burst(At + FVector(0, 0, 60), TEXT("star_07"), FLinearColor(1.f, 0.85f, 0.4f), 40, 380.f, 1.4f, 50.f, 420.f, -60.f, 5.f);
	}
}

void AHollowGameMode::PickBoost(const FString& Abil)
{
	if (!LevelUp.IsSet()) return;
	// four different abilities, toggled on and off
	if (LevelUp->Boosts.Remove(Abil) == 0 && LevelUp->Boosts.Num() < LevelUp->BoostsNeeded) LevelUp->Boosts.Add(Abil);
	MarkUI();
}

void AHollowGameMode::PickFeat(const FString& Slot, const FString& Feat)
{
	if (!LevelUp.IsSet()) return;
	FString& Dst = Slot == TEXT("class") ? LevelUp->ClassFeat : Slot == TEXT("skill") ? LevelUp->SkillFeat : Slot == TEXT("general") ? LevelUp->GeneralFeat : LevelUp->AncestryFeat;
	Dst = Dst == Feat ? FString() : Feat;
	MarkUI();
}

void AHollowGameMode::PickSpell(const FString& Spell)
{
	if (!LevelUp.IsSet()) return;
	if (LevelUp->NewSpells.Remove(Spell) == 0 && LevelUp->NewSpells.Num() < LevelUp->SpellsNeeded) LevelUp->NewSpells.Add(Spell);
	MarkUI();
}

TArray<const FLibFeat*> AHollowGameMode::FeatOptions(const FString& Slot) const
{
	TArray<const FLibFeat*> Out;
	if (!LevelUp.IsSet()) return Out;
	const int32 Lv = LevelUp->Level;
	const FString Cls = PC.ClassName.ToLower(), Anc = PC.Ancestry.ToLower();
	for (const FLibFeat& F : HollowLib::Feats())
	{
		if (F.Level > Lv || F.Rarity != TEXT("common")) continue;
		if (PC.Feats.ContainsByPredicate([&](const TPair<int32, FString>& T) { return T.Value == F.Name; })) continue;
		const bool bOk = Slot == TEXT("class") ? F.Category == TEXT("class") && F.Traits.Contains(Cls)
			: Slot == TEXT("skill") ? F.Category == TEXT("skill")
			: Slot == TEXT("general") ? F.Category == TEXT("general") || F.Category == TEXT("skill")
			: F.Category == TEXT("ancestry") && F.Traits.Contains(Anc);
		if (bOk) Out.Add(&F);
	}
	Out.Sort([](const FLibFeat& A, const FLibFeat& B) { return A.Level != B.Level ? A.Level > B.Level : A.Name < B.Name; });
	if (Out.Num() > 80) Out.SetNum(80);
	return Out;
}

TArray<const FLibSpell*> AHollowGameMode::SpellOptions() const
{
	TArray<const FLibSpell*> Out;
	if (!LevelUp.IsSet()) return Out;
	const FString Trad = CasterTradition();
	const int32 MaxRank = FMath::Max(1, (LevelUp->Level + 1) / 2);
	for (const FLibSpell& S : HollowLib::Spells())
		if (!S.bCantrip && !S.bFocus && S.Rarity == TEXT("common") && S.Rank <= MaxRank && S.Rank >= MaxRank - 1 && S.Traditions.Contains(Trad) && !PC.Spells.Contains(S.Name))
			Out.Add(&S);
	Out.Sort([](const FLibSpell& A, const FLibSpell& B) { return A.Rank != B.Rank ? A.Rank > B.Rank : A.Name < B.Name; });
	if (Out.Num() > 80) Out.SetNum(80);
	return Out;
}

void AHollowGameMode::ConfirmLevelUp()
{
	if (!LevelUp.IsSet() || !LevelUp->IsComplete()) return;
	const FLevelUpView V = LevelUp.GetValue();
	// PF2e boosts: +1 to a modifier below +4; at +4 or more a boost is "partial" and two of them make +1
	for (const FString& Ab : V.Boosts)
	{
		int32& A = PC.Abil.FindOrAdd(Ab);
		const int32 Partials = PC.Boosts.FilterByPredicate([&](const FString& B) { return B == Ab + TEXT("*"); }).Num();
		if (A < 4) { A++; PC.Boosts.Add(Ab); }
		else { PC.Boosts.Add(Ab + TEXT("*")); if (Partials % 2 == 1) A++; }
		if (Ab == TEXT("con")) { PC.HpPer++; PC.HpMax += PC.Level; PC.Hp += PC.Level; }
	}
	if (V.bSkill) { int32& Rk = PC.Ranks.FindOrAdd(V.Skill); Rk = FMath::Min(MaxRankAt(V.Level), Rk + 1); }
	TArray<FString> Took;
	for (const FString& F : { V.ClassFeat, V.SkillFeat, V.GeneralFeat, V.AncestryFeat }) if (!F.IsEmpty()) { PC.Feats.Add({ V.Level, F }); Took.Add(F); }
	// new spells go straight into the deck as cards at the rank you learned them
	for (const FString& Sp : V.NewSpells)
	{
		PC.Spells.AddUnique(Sp);
		if (const FLibSpell* S = HollowLib::Spell(Sp)) { const FString Card = HollowData::SpellCard(S->Name, S->Rank); if (!Card.IsEmpty()) PC.Deck.Add(Card); }
		Took.Add(Sp);
	}
	FString Line = FString::Printf(TEXT("Level %d"), V.Level);
	if (V.Boosts.Num()) Line += TEXT(": boosted ") + FString::Join(V.Boosts, TEXT(", ")).ToUpper();
	if (V.bSkill) Line += TEXT(", improved ") + SkillName(V.Skill);
	if (Took.Num()) Line += TEXT(", took ") + FString::Join(Took, TEXT(", "));
	Log(Line + TEXT("."));
	PlaySfx(TEXT("confirm"));
	LevelUp.Reset();
	// several levels at once (a big XP award, or an import below its XP): plan the next one
	int32 Planned = V.Level;
	if (Planned < PC.Level) LevelUp = PlanLevel(PC, Planned + 1, CasterTradition());
	Recompute(PC);
	RefreshAura();
	MarkUI();
}

void AHollowGameMode::AddBuff(const FString& Id, const FString& Name, const FString& Desc, const FLinearColor& Col, int32 Fights)
{
	Buffs.RemoveAll([&](const FBuff& B) { return B.Id == Id; });
	FBuff B; B.Id = Id; B.Name = Name; B.Desc = Desc; B.Col = Col; B.Fights = Fights;
	Buffs.Add(B);
	RefreshAura();
	MarkUI();
}

// The hero's aura: a glowing ring at the feet, three orbiting motes, a tinted light and the Fab buff Niagara when present.
void AHollowGameMode::RefreshAura()
{
	if (!Hero) return;
	const bool bOn = Buffs.Num() > 0;
	const FLinearColor Col = bOn ? Buffs.Last().Col : FLinearColor::White;
	if (!AuraRing && bOn)
	{
		AuraRing = HollowArt::GlowPart(Hero, EShape::Cylinder, FVector(0, 0, -86.f), FVector(150, 150, 2), Col * 0.18f);
		for (int32 i = 0; i < 3; ++i)
			AuraOrbs.Add(HollowArt::GlowPart(Hero, EShape::Sphere, FVector::ZeroVector, FVector(12, 12, 12), Col));
		AuraLight = HollowArt::Light(Hero, FVector(0, 0, 40), Col, 2.5f, 320.f);
		HeroAura = HollowArt::NiagaraPart(Hero, TEXT("/Game/NiagaraExamples/FX_Player/NS_Player_Buff_Looping"), FVector(0, 0, -86.f), 0.8f);
	}
	if (AuraRing)
	{
		AuraRing->SetVisibility(bOn && !HeroAura);   // the Fab buff Niagara draws its own ground ring
		for (UStaticMeshComponent* O : AuraOrbs) if (O) O->SetVisibility(bOn);
		if (bOn)
		{
			AuraRing->SetMaterial(0, HollowArt::GlowMat(Col * 0.18f));
			for (UStaticMeshComponent* O : AuraOrbs) if (O) O->SetMaterial(0, HollowArt::GlowMat(Col));
		}
	}
	if (AuraLight) { AuraLight->SetVisibility(bOn); AuraLight->SetLightColor(Col); }
	if (HeroAura) { if (bOn) HeroAura->Activate(); else HeroAura->Deactivate(); }
}

/* ---------- Tool-gated field spots ---------- */

namespace
{
	struct FFeatureDef { const TCHAR* Kind; const TCHAR* Name; const TCHAR* Need; const TCHAR* Alt; const TCHAR* AltSkill; int32 AltDC; const TCHAR* NoTool; };
	const FFeatureDef* FeatureDef(const FString& Kind)
	{
		static const FFeatureDef L[] = {
			{ TEXT("dig"), TEXT("Loose earth"), TEXT("dig"), nullptr, nullptr, 0, TEXT("The earth here has been turned recently. Your fingers won't get far. A shovel would.") },
			{ TEXT("cut"), TEXT("Bramble thicket"), TEXT("cut"), TEXT("fire"), nullptr, 0, TEXT("Something glints behind a wall of black thorns. A hatchet could cut through, or a flame could clear it.") },
			{ TEXT("climb"), TEXT("High ledge"), TEXT("climb"), nullptr, TEXT("athletics"), 18, TEXT("A pack is wedged on a ledge far above. With a rope and grapnel it'd be easy.") },
			{ TEXT("pick"), TEXT("Locked strongbox"), TEXT("pick"), nullptr, TEXT("athletics"), 18, TEXT("An iron-banded box with a stubborn lock. Thieves' tools would open it quietly.") },
			{ TEXT("dark"), TEXT("Black hollow"), TEXT("light"), nullptr, nullptr, 0, TEXT("A cave mouth so dark it swallows the moonlight. You won't go in there without a light.") },
			{ TEXT("fire"), TEXT("Cold beacon"), TEXT("fire"), nullptr, nullptr, 0, TEXT("An old signal brazier, packed with dry wood. You'd need fire to light it.") },
		};
		for (const FFeatureDef& D : L) if (Kind == D.Kind) return &D;
		return nullptr;
	}
	// Paper cutouts: a painted PNG on a camera-facing card, the Paper Mario way. Textures come straight from
	// Content/Hollow/EmberfallArt so new art drops in without an import step.
	UTexture2D* ArtTex(const FString& Rel)
	{
		static TMap<FString, UTexture2D*> Cache;
		if (UTexture2D** Found = Cache.Find(Rel)) return *Found;
		UTexture2D* T = nullptr;
		const FString File = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Hollow/EmberfallArt"), Rel + TEXT(".png"));
		if (FPaths::FileExists(File)) { T = FImageUtils::ImportFileAsTexture2D(File); if (T) T->AddToRoot(); }
		Cache.Add(Rel, T);
		return T;
	}
	UMaterialBillboardComponent* Paper(AActor* A, UMaterialInterface* Base, const FString& Rel, float Height)
	{
		static UMaterialInterface* Lit = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Hollow/Materials/M_HollowPaper.M_HollowPaper"));
		if (Lit) Base = Lit;
		UTexture2D* T = ArtTex(Rel);
		if (!A || !Base || !T) return nullptr;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, A);
		M->SetTextureParameterValue(TEXT("Tex"), T);
		M->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.95f, 0.93f, 1.f, 1.f));   // lit by the scene, so full colour
		UMaterialBillboardComponent* B = NewObject<UMaterialBillboardComponent>(A);
		B->SetupAttachment(A->GetRootComponent());
		const float W = Height * T->GetSizeX() / FMath::Max(1, T->GetSizeY());
		B->AddElement(M, nullptr, false, W * 0.5f, Height * 0.5f, nullptr);
		B->SetRelativeLocation(FVector(0, 0, Height * 0.5f));
		B->RegisterComponent();
		A->AddInstanceComponent(B);
		return B;
	}
	// Before and after art for each tool spot (Seph's Six_Tool_Spots_Paired_States sheet).
	FString SpotArt(const FString& Kind, bool bDone)
	{
		static const TMap<FString, TPair<const TCHAR*, const TCHAR*>> M = {
			{ TEXT("dig"), { TEXT("001_dig_mound"), TEXT("002_dig_hole") } }, { TEXT("cut"), { TEXT("003_brambles"), TEXT("004_brambles_cleared") } },
			{ TEXT("climb"), { TEXT("005_ledge"), TEXT("006_ledge_rope") } }, { TEXT("pick"), { TEXT("007_box_closed"), TEXT("008_box_open") } },
			{ TEXT("dark"), { TEXT("009_cave"), TEXT("009_cave") } }, { TEXT("fire"), { TEXT("011_beacon_unlit"), TEXT("012_beacon_lit") } } };
		const auto* P = M.Find(Kind);
		return P ? FString::Printf(TEXT("Environment/Six_Tool_Spots_Paired_States/Six_Tool_Spots_Paired_States_%s"), bDone ? P->Value : P->Key) : FString();
	}
	float SpotHeight(const FString& Kind) { return Kind == TEXT("climb") || Kind == TEXT("dark") ? 420.f : Kind == TEXT("fire") ? 300.f : Kind == TEXT("cut") ? 260.f : 170.f; }

	FLinearColor AbilityCol(const FString& A)
	{
		if (A == TEXT("dig")) return FLinearColor(0.85f, 0.65f, 0.35f);
		if (A == TEXT("cut")) return FLinearColor(0.5f, 0.9f, 0.4f);
		if (A == TEXT("climb")) return FLinearColor(0.75f, 0.75f, 1.f);
		if (A == TEXT("pick")) return FLinearColor(0.8f, 0.85f, 0.95f);
		if (A == TEXT("light")) return FLinearColor(1.f, 0.85f, 0.45f);
		return FLinearColor(1.f, 0.5f, 0.2f);
	}
}

void AHollowGameMode::BuildFeatures(FHollowHex& Hex, TFunctionRef<bool(float, float, float, FVector&)> Pick)
{
	UWorld* W = GetWorld();
	FRandomStream Rng(Hex.Q * 4241 + Hex.R * 7703 + 5);
	static const TCHAR* Kinds[] = { TEXT("dig"), TEXT("cut"), TEXT("climb"), TEXT("pick"), TEXT("dark"), TEXT("fire") };
	const int32 Count = 2 + (Rng.FRand() < 0.5f ? 1 : 0);
	if (Hex.FeatureDone.Num() != Count) Hex.FeatureDone.Init(false, Count);
	TArray<int32> Used;
	for (int32 i = 0; i < Count; ++i)
	{
		int32 K = Rng.RandRange(0, 5);
		while (Used.Contains(K)) K = (K + 1) % 6;
		Used.Add(K);
		FVector P;
		if (!Pick(700.f, 1900.f, 200.f, P)) continue;
		const FString Kind = Kinds[K];
		AActor* A = Local(HollowArt::NewActor(W, P, Rng.FRandRange(0.f, 360.f)));
		// spent spots stay in the world in their "after" state: a dug hole, an open box, a lit beacon
		if (Hex.FeatureDone[i]) { Paper(A, SpriteMat, SpotArt(Kind, true), SpotHeight(Kind)); continue; }
		const FLinearColor Iron(0.18f, 0.17f, 0.16f), Wood(0.32f, 0.2f, 0.11f);
		if (Paper(A, SpriteMat, SpotArt(Kind, false), SpotHeight(Kind))) {}
		else if (Kind == TEXT("dig"))
		{
			HollowArt::Part(A, EShape::Sphere, FVector(0, 0, -10), FVector(150, 120, 40), FLinearColor(0.22f, 0.15f, 0.09f));
			HollowArt::Part(A, EShape::Cube, FVector(30, -20, 30), FVector(8, 30, 60), Wood, FRotator(0, 0, 20));
		}
		else if (Kind == TEXT("cut"))
		{
			for (int32 j = 0; j < 6; ++j)
				HollowArt::Part(A, j % 2 ? EShape::Cone : EShape::Sphere, FVector(FMath::Cos((float)j) * 70.f, FMath::Sin(j * 1.7f) * 90.f, 40 + (j % 3) * 20.f), FVector(90, 90, 110), FLinearColor(0.07f, 0.08f, 0.05f));
			HollowArt::Part(A, EShape::Cube, FVector(0, 0, 20), FVector(50, 40, 40), Wood);
		}
		else if (Kind == TEXT("climb"))
		{
			HollowArt::Part(A, EShape::Cube, FVector(0, 0, 160), FVector(170, 150, 320), FLinearColor(0.2f, 0.19f, 0.2f), FRotator(0, 0, 4), true);
			HollowArt::Part(A, EShape::Cube, FVector(0, 0, 340), FVector(50, 40, 36), Wood);
		}
		else if (Kind == TEXT("pick"))
		{
			HollowArt::Part(A, EShape::Cube, FVector(0, 0, 30), FVector(80, 55, 55), Wood, FRotator::ZeroRotator, true);
			HollowArt::Part(A, EShape::Cube, FVector(0, 0, 30), FVector(84, 59, 10), Iron);
			HollowArt::Part(A, EShape::Cube, FVector(42, 0, 30), FVector(6, 16, 20), FLinearColor(0.6f, 0.5f, 0.25f));
		}
		else if (Kind == TEXT("dark"))
		{
			HollowArt::Part(A, EShape::Sphere, FVector(0, 0, 40), FVector(320, 280, 220), FLinearColor(0.13f, 0.12f, 0.12f), FRotator::ZeroRotator, true);
			HollowArt::Part(A, EShape::Sphere, FVector(120, 0, 40), FVector(110, 150, 150), FLinearColor(0.005f, 0.005f, 0.008f));
		}
		else
		{
			HollowArt::Part(A, EShape::Cylinder, FVector(0, 0, 60), FVector(30, 30, 120), Iron);
			HollowArt::Part(A, EShape::Cylinder, FVector(0, 0, 125), FVector(110, 110, 30), Iron);
			for (int32 j = 0; j < 4; ++j) HollowArt::Part(A, EShape::Cylinder, FVector(0, 0, 150), FVector(12, 12, 90), Wood, FRotator(60, j * 90.f, 0));
		}
		// the overlay: a ring that glows only while you carry something that can deal with this spot
		const FFeatureDef* D = FeatureDef(Kind);
		UStaticMeshComponent* Ring = HollowArt::GlowPart(A, EShape::Cylinder, FVector(0, 0, 2), FVector(260, 260, 2), AbilityCol(D->Need) * 0.15f);
		FeatureGlows.Add({ Ring, D->Alt ? FString(D->Need) + TEXT("|") + D->Alt : FString(D->Need) });
		AddInteract(A, TEXT("feature"), Kind, D->Name, i, Kind == TEXT("climb") || Kind == TEXT("dark") ? 320.f : 240.f);
	}
	RefreshFeatureGlow();
}

void AHollowGameMode::RefreshFeatureGlow()
{
	for (int32 i = FeatureGlows.Num() - 1; i >= 0; --i)
	{
		UStaticMeshComponent* R = FeatureGlows[i].Key.Get();
		if (!R) { FeatureGlows.RemoveAt(i); continue; }
		TArray<FString> Needs;
		FeatureGlows[i].Value.ParseIntoArray(Needs, TEXT("|"));
		bool bCan = false;
		for (const FString& N : Needs) bCan |= HasAbility(N);
		R->SetVisibility(bCan);
	}
}

FString AHollowGameMode::FeatureLabel(const FInteractable& I) const
{
	const FFeatureDef* D = FeatureDef(I.Id);
	if (!D) return I.Label;
	const FItemDef* Tool = HollowItems::Find(PC.Inv.GrantingItem(D->Need));
	if (!Tool && D->Alt) Tool = HollowItems::Find(PC.Inv.GrantingItem(D->Alt));
	if (Tool) return FString::Printf(TEXT("%s: %s (%s)"), D->Name, *HollowItems::AbilityName(PC.Inv.CanGrant(D->Need) ? D->Need : D->Alt), *Tool->Name);
	if (D->AltSkill) return FString::Printf(TEXT("%s: try by hand (%s DC %d)"), D->Name, *SkillName(D->AltSkill), D->AltDC);
	return FString::Printf(TEXT("%s: needs %s"), D->Name, *HollowItems::AbilityName(D->Need).ToLower());
}

void AHollowGameMode::FeatureLoot(const FString& Kind, const FVector& At, FString& Msg)
{
	int32 Silver = 0;
	TArray<TPair<FString, int32>> Items;
	if (Kind == TEXT("dig")) { Silver = R(TEXT("2d6+4")); if (FMath::FRand() < 0.5f) Items.Add({ TEXT("trinket"), 1 }); if (FMath::FRand() < 0.15f) Items.Add({ TEXT("shard"), 1 }); }
	else if (Kind == TEXT("cut")) { Silver = R(TEXT("1d6")); Items.Add({ FMath::FRand() < 0.5f ? TEXT("potion") : TEXT("ration"), FMath::FRand() < 0.5f ? 1 : 2 }); }
	else if (Kind == TEXT("climb")) { Silver = R(TEXT("2d8")); Items.Add({ FMath::FRand() < 0.4f ? TEXT("shard") : TEXT("trinket"), 1 }); }
	else if (Kind == TEXT("pick")) { Silver = R(TEXT("3d6")); if (FMath::FRand() < 0.6f) Items.Add({ TEXT("potion"), 1 }); }
	else if (Kind == TEXT("dark"))
	{
		Silver = R(TEXT("2d6"));
		Items.Add({ TEXT("shard"), 1 });
		if (!PC.Inv.Has(TEXT("charm")) && !GrantedOnce.Contains(TEXT("opt:maren:charm"))) Items.Add({ TEXT("charm"), 1 });
	}
	PC.Gold += Silver;
	TArray<FString> Parts;
	if (Silver) Parts.Add(FString::Printf(TEXT("%d silver"), Silver));
	for (const auto& It : Items)
		if (const FItemDef* D = HollowItems::Find(It.Key))
		{
			Give(It.Key, It.Value);
			Parts.Add(It.Value > 1 ? FString::Printf(TEXT("%d %s"), It.Value, *D->Name) : D->Name);
		}
	// now and then something from the wider world: a library item at this hex's level, rarity-weighted
	if (Kind != TEXT("fire") && FMath::FRand() < 0.35f)
	{
		const FString Lib = RollLoot(HexLevel(Pos), FMath::Rand());
		if (const FItemDef* D = HollowItems::Find(Lib)) { Give(Lib, 1); Parts.Add(D->Name + (D->Rarity >= ERarity::Uncommon ? FString::Printf(TEXT(" (%s)"), *HollowItems::RarityName(D->Rarity).ToLower()) : FString())); }
	}
	if (Parts.Num()) Msg += TEXT(" You find ") + FString::Join(Parts, TEXT(", ")) + TEXT(".");
	PlaySfx(TEXT("coins"));
	Burst(At + FVector(0, 0, 60), TEXT("star_04"), FLinearColor(1.f, 0.75f, 0.3f), 16, 260.f, 1.f, 40.f, 200.f, 400.f);
}

void AHollowGameMode::UseFeature(const FInteractable& I)
{
	const FFeatureDef* D = FeatureDef(I.Id);
	FHollowHex* H = Hexes.Find(Pos);
	if (!D || !H) return;
	const bool bTool = HasAbility(D->Need);
	const bool bAlt = !bTool && D->Alt && HasAbility(D->Alt);
	FString Msg;
	bool bDone = false;
	if (bTool || bAlt)
	{
		const FString Used = bTool ? D->Need : D->Alt;
		const FItemDef* Tool = HollowItems::Find(PC.Inv.GrantingItem(Used));
		const FString ToolName = Tool ? Tool->Name : FString(TEXT("your tools"));
		if (I.Id == TEXT("pick"))
		{
			// tools make it a skill check you can actually pass, and a failure only costs time
			const int32 Nat = HollowRules::D(20); const int32 Mod = SkillMod(PC, TEXT("thievery")) + 2;
			const int32 Deg = Degree(Nat + Mod, 15, Nat);
			Msg = FString::Printf(TEXT("Thievery with %s: %d %s = %d vs DC 15, %s."), *ToolName, Nat, *Sgn(Mod), Nat + Mod, DegreeName(Deg));
			if (Deg >= 2) { bDone = true; Msg += TEXT(" The lock gives with a click."); }
			else { Msg += TEXT(" The pins won't set. An hour lost."); }
		}
		else
		{
			static const TMap<FString, FString> Lines = {
				{ TEXT("dig"), TEXT("You dig until the blade knocks on wood.") },
				{ TEXT("cut"), TEXT("You hack a way through the thorns.") },
				{ TEXT("climb"), TEXT("The grapnel bites on the third throw and you haul yourself up.") },
				{ TEXT("dark"), TEXT("Your light pushes the dark back into the cave. Bones, a bedroll, and something that glows.") },
				{ TEXT("fire"), TEXT("The beacon roars alight. From up here the fire shows you the land around.") } };
			Msg = Lines.FindRef(I.Id);
			if (bAlt) Msg = TEXT("You set the thorns alight. They burn fast and leave a smoking path.");
			bDone = true;
			if (Used == TEXT("light") || Used == TEXT("fire")) PC.Inv.UseCharge(Used);
		}
		if (I.Id == TEXT("dark") && bDone && FMath::FRand() < 0.3f)
		{
			Note = FNoteView{ D->Name, Msg + TEXT(" Then something in the back of the cave stands up.") };
			H->FeatureDone[I.Index] = true;
			if (AActor* A = I.Actor.Get()) A->Destroy();
			Interactables.RemoveAll([&](const FInteractable& X) { return X.Actor == I.Actor; });
			FeatureLoot(I.Id, I.Loc, Msg);
			StartCombat(TEXT("ghoul"), false, false);
			MarkUI();
			return;
		}
	}
	else if (D->AltSkill)
	{
		const int32 Nat = HollowRules::D(20); const int32 Mod = SkillMod(PC, D->AltSkill);
		const int32 Deg = Degree(Nat + Mod, D->AltDC, Nat);
		Msg = FString::Printf(TEXT("By hand, %s: %d %s = %d vs DC %d, %s."), *SkillName(D->AltSkill), Nat, *Sgn(Mod), Nat + Mod, D->AltDC, DegreeName(Deg));
		if (Deg >= 2) { bDone = true; Msg += I.Id == TEXT("pick") ? TEXT(" You smash the lock off.") : TEXT(" You scramble up, fingers bleeding."); }
		else
		{
			const int32 Dm = HollowRules::D(6);
			PC.Hp = FMath::Max(1, PC.Hp - Dm);
			Msg += FString::Printf(TEXT(" You slip and hurt yourself: -%d HP. %s"), Dm, D->NoTool);
		}
	}
	else
	{
		Note = FNoteView{ D->Name, D->NoTool };
		PlaySfx(TEXT("book"), 0.6f);
		MarkUI();
		return;
	}

	if (bDone)
	{
		H->FeatureDone[I.Index] = true;
		if (I.Id == TEXT("fire"))
		{
			int32 Shown = 0;
			for (TPair<FIntPoint, FHollowHex>& Hx : Hexes)
			{
				const FIntPoint Dd = Hx.Key - Pos;
				const int32 Dist = (FMath::Abs(Dd.X) + FMath::Abs(Dd.Y) + FMath::Abs(Dd.X + Dd.Y)) / 2;
				if (Dist <= 2 && Hx.Value.State == EHexState::Hidden) { Hx.Value.State = EHexState::Rumored; Shown++; }
			}
			Msg += Shown ? FString::Printf(TEXT(" %d nearby hexes appear on your map."), Shown) : FString(TEXT(" You already know this country."));
			NiagaraAt(TEXT("/Game/NiagaraExamples/FX_Misc/NS_Fire"), I.Loc + FVector(0, 0, 140), 1.f);
			GainXp(5);
		}
		else FeatureLoot(I.Id, I.Loc, Msg);
		if (AActor* A = I.Actor.Get())
		{
			// swap the cutout to its "after" art, and put the hint ring out
			TInlineComponentArray<UMaterialBillboardComponent*> Cards(A);
			if (Cards.Num()) { for (UMaterialBillboardComponent* B : Cards) B->DestroyComponent(); Paper(A, SpriteMat, SpotArt(I.Id, true), SpotHeight(I.Id)); }
			else if (I.Id != TEXT("fire")) A->Destroy();
			if (IsValid(A)) { TInlineComponentArray<UStaticMeshComponent*> Parts(A); for (UStaticMeshComponent* Pm : Parts) if (Pm->GetRelativeScale3D().Z < 0.05f) Pm->SetVisibility(false); }
			FeatureGlows.RemoveAll([A](const TPair<TWeakObjectPtr<UStaticMeshComponent>, FString>& Gl) { return !Gl.Key.IsValid() || Gl.Key->GetOwner() == A; });
		}
		Interactables.RemoveAll([&](const FInteractable& X) { return X.Actor == I.Actor; });
		NearIdx = INDEX_NONE;
		GainXp(3);
	}
	Toast(Msg);
	Log(Msg);
	RefreshFeatureGlow();
	MarkUI();
}

/* ---------- Structured dialogue ---------- */

void AHollowGameMode::SetTalkLine(const FString& Line)
{
	if (!Talk.IsSet()) return;
	Talk->Line = Line;
	Talk->LineAt = FPlatformTime::Seconds();
	Talk->Serial++;
	MarkUI();
}

bool AHollowGameMode::TalkRevealing() const
{
	if (!Talk.IsSet() || Talk->bThinking) return false;
	const int32 Len = StripBrackets(Talk->Line).Len();
	return (FPlatformTime::Seconds() - Talk->LineAt) * 55.0 < Len;
}

int32 AHollowGameMode::CheckChance(const FString& Skill, int32 DC) const
{
	// chance of success on d20 + mod >= DC; a natural 20 always bumps a degree, a natural 1 always drops one
	const int32 Need = DC - SkillMod(PC, Skill);
	return FMath::Clamp((21 - Need) * 5, 5, 95);
}

TArray<FTalkChoice> AHollowGameMode::TalkChoices() const
{
	TArray<FTalkChoice> L;
	if (!Talk.IsSet()) return L;
	const FNpcDef* N = HollowData::FindNpc(Talk->NpcId);
	if (!N) return L;

	for (const FTalkOption& O : N->Options)
	{
		const FString Key = TEXT("opt:") + N->Id + TEXT(":") + O.Id;
		if (O.bOnce && GrantedOnce.Contains(Key)) continue;
		if (!O.After.IsEmpty() && !GrantedOnce.Contains(TEXT("opt:") + N->Id + TEXT(":") + O.After)) continue;
		FTalkChoice C;
		C.Kind = FTalkChoice::Option; C.Id = O.Id; C.Text = O.Label;
		const FItemDef* Need = HollowItems::Find(O.NeedItem);
		if (Need && !PC.Inv.Has(O.NeedItem, O.NeedCount))
		{
			C.bLocked = true; C.TagStyle = 4;
			C.Tag = FString::Printf(TEXT("Requires %s"), *Need->Name);
		}
		else if (O.Cost > PC.Gold) { C.bLocked = true; C.TagStyle = 4; C.Tag = FString::Printf(TEXT("%d silver"), O.Cost); }
		else if (!O.Skill.IsEmpty()) { C.TagStyle = 1; C.Tag = FString::Printf(TEXT("%s %d  %d%%"), *SkillName(O.Skill), O.DC, CheckChance(O.Skill, O.DC)); }
		else if (Need && O.bGive) { C.TagStyle = 2; C.Tag = O.bGiveAll ? FString::Printf(TEXT("Give: %s x%d"), *Need->Name, PC.Count(O.NeedItem)) : FString::Printf(TEXT("Give: %s"), *Need->Name); }
		else if (Need) { C.TagStyle = 2; C.Tag = FString::Printf(TEXT("Show: %s"), *Need->Name); }
		else if (O.Cost) { C.TagStyle = 3; C.Tag = FString::Printf(TEXT("%d silver"), O.Cost); }
		L.Add(C);
	}

	// topics this person can speak to: new ones first, already-heard ones dimmed after
	TArray<FTalkChoice> Seen;
	for (const FTopicDef& Tp : HollowData::Topics())
	{
		if (!KnownTopics.Contains(Tp.Id) || !N->Replies.Contains(Tp.Id)) continue;
		FTalkChoice C;
		C.Kind = FTalkChoice::Topic; C.Id = Tp.Id;
		C.Text = FString::Printf(TEXT("Ask about %s."), *Tp.Label);
		C.bSeen = GrantedOnce.Contains(N->Id + TEXT(":") + Tp.Id);
		if (!N->Replies[Tp.Id].Lead.IsEmpty() && !LeadsGiven.Contains(N->Replies[Tp.Id].Lead)) { C.Tag = TEXT("New route"); C.TagStyle = 2; }
		(C.bSeen ? Seen : L).Add(C);
	}
	L.Append(Seen);

	auto Svc = [&](const FString& Id, const FString& Text, int32 Cost)
	{
		FTalkChoice C; C.Kind = FTalkChoice::Service; C.Id = Id; C.Text = Text;
		C.Tag = FString::Printf(TEXT("%d silver"), Cost); C.TagStyle = PC.Gold >= Cost ? 3 : 4; C.bLocked = PC.Gold < Cost;
		L.Add(C);
	};
	if (N->Service == TEXT("inn")) Svc(TEXT("rest"), TEXT("I'll take a bed for the night."), 5);
	if (N->Service == TEXT("temple")) Svc(TEXT("prayer"), TEXT("Pray over my wounds, Sister."), 6);
	if (N->Service == TEXT("shop")) Svc(TEXT("pack"), TEXT("Sell me a pack of fight-cards."), 12);
	if (N->Stock.Num()) { FTalkChoice C; C.Kind = FTalkChoice::Trade; C.Text = TEXT("Let's trade."); C.Tag = TEXT("Trade"); C.TagStyle = 3; L.Add(C); }
	{ FTalkChoice C; C.Kind = FTalkChoice::Say; C.Text = TEXT("Say something of your own..."); C.Tag = bBrainOnline ? TEXT("free voice") : FString(); C.TagStyle = 1; L.Add(C); }
	{ FTalkChoice C; C.Kind = FTalkChoice::Leave; C.Text = TEXT("Farewell."); L.Add(C); }
	return L;
}

void AHollowGameMode::TalkNumber(int32 Number)
{
	if (!Talk.IsSet() || Talk->bTyping || Trade.IsSet() || bPaused) return;
	if (TalkRevealing()) { SkipReveal(); return; }
	ChooseTalk(Number - 1);
}

void AHollowGameMode::ChooseTalk(int32 Index)
{
	if (!Talk.IsSet()) return;
	const TArray<FTalkChoice> L = TalkChoices();
	if (!L.IsValidIndex(Index)) return;
	const FTalkChoice& C = L[Index];
	if (C.bLocked) { PlaySfx(TEXT("click"), 0.3f); Toast(C.Tag); return; }
	const FNpcDef* N = HollowData::FindNpc(Talk->NpcId);
	if (!N) return;
	Talk->LastCheck.Reset();
	PlaySfx(TEXT("click"), 0.5f);
	switch (C.Kind)
	{
	case FTalkChoice::Topic: AskTopic(C.Id); break;
	case FTalkChoice::Service: Talk->Asked = C.Text; Service(C.Id); break;
	case FTalkChoice::Trade: OpenTrade(); break;
	case FTalkChoice::Say: Talk->bTyping = true; MarkUI(); break;
	case FTalkChoice::Leave: CloseTalk(); break;
	case FTalkChoice::Option:
		if (const FTalkOption* O = N->Options.FindByPredicate([&](const FTalkOption& X) { return X.Id == C.Id; })) UseTalkOption(*N, *O);
		break;
	default: break;
	}
}

void AHollowGameMode::UseTalkOption(const FNpcDef& N, const FTalkOption& O)
{
	const FString Key = TEXT("opt:") + N.Id + TEXT(":") + O.Id;
	Talk->Asked = O.Label;
	Talk->LeadNote.Reset();
	bool bOk = true;
	if (!O.Skill.IsEmpty())
	{
		const int32 Nat = HollowRules::D(20); const int32 Mod = SkillMod(PC, O.Skill);
		const int32 Deg = Degree(Nat + Mod, O.DC, Nat);
		bOk = Deg >= 2;
		Talk->LastCheck = FString::Printf(TEXT("%s  d20 %d %s = %d  vs DC %d  -  %s"), *SkillName(O.Skill), Nat, *Sgn(Mod), Nat + Mod, O.DC, DegreeName(Deg));
		Talk->bLastOk = bOk;
		GrantedOnce.Add(Key);   // a check can't be retried with the same person
	}
	if (O.Cost) { PC.Gold -= O.Cost; }
	if (!bOk) { SetTalkLine(O.Fail); LearnTopics(O.Fail); Talk->History.Add({ O.Label, O.Fail }); MarkUI(); return; }

	int32 Given = 0;
	if (O.bGive && !O.NeedItem.IsEmpty())
	{
		Given = O.bGiveAll ? PC.Count(O.NeedItem) : O.NeedCount;
		PC.Inv.Take(O.NeedItem, Given);
	}
	const int32 Silver = O.bGiveAll ? O.Gold * Given : O.Gold;
	if (Silver) { PC.Gold += Silver; Toast(FString::Printf(TEXT("+%d silver"), Silver)); PlaySfx(TEXT("coins"), 0.6f); }
	if (!O.GiveItem.IsEmpty()) Give(O.GiveItem, O.GiveCount, true);
	if (O.Xp) GainXp(O.Xp);
	if (!O.Lead.IsEmpty() && !LeadsGiven.Contains(O.Lead))
	{
		GrantLead(O.Lead);
		if (const FLeadDef* Ld = HollowData::FindLead(O.Lead)) Talk->LeadNote = FString::Printf(TEXT("New route on your map: %s. Press M to scout it."), *Ld->Name);
	}
	GrantedOnce.Add(Key);
	SetTalkLine(O.Ok);
	LearnTopics(O.Ok);
	Talk->History.Add({ O.Label, O.Ok });
	RefreshFeatureGlow();
	MarkUI();
}

/* ---------- Pathbuilder 2e import ---------- */

void AHollowGameMode::ImportPathbuilder(const FString& BuildId)
{
	const FString Id = BuildId.TrimStartAndEnd();
	if (Id.IsEmpty() || bPbBusy) return;
	if (Id.StartsWith(TEXT("{"))) { ApplyPathbuilder(Id); return; }   // pasted JSON works too
	bPbBusy = true;
	PbStatus = TEXT("Fetching build ") + Id + TEXT("...");
	TWeakObjectPtr<AHollowGameMode> Weak = this;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(TEXT("https://pathbuilder2e.com/json.php?id=") + FGenericPlatformHttp::UrlEncode(Id));
	Req->SetVerb(TEXT("GET"));
	Req->SetTimeout(15.f);
	Req->OnProcessRequestComplete().BindLambda([Weak](FHttpRequestPtr, FHttpResponsePtr Resp, bool bOk)
	{
		AHollowGameMode* G = Weak.Get();
		if (!G) return;
		G->bPbBusy = false;
		if (!bOk || !Resp.IsValid() || Resp->GetResponseCode() != 200) { G->PbStatus = TEXT("Couldn't reach Pathbuilder. Check the id, or paste the exported JSON instead."); G->MarkUI(); return; }
		G->ApplyPathbuilder(Resp->GetContentAsString());
	});
	Req->ProcessRequest();
	MarkUI();
}

void AHollowGameMode::ApplyPathbuilder(const FString& Json)
{
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid()) { PbStatus = TEXT("That isn't Pathbuilder JSON."); MarkUI(); return; }
	const TSharedPtr<FJsonObject>* BuildP = nullptr;
	if (!Root->TryGetObjectField(TEXT("build"), BuildP)) { PbStatus = TEXT("No build found. Export it from Pathbuilder: Menu > Export > Export JSON."); MarkUI(); return; }
	const FJsonObject& B = **BuildP;
	auto Str = [&](const FJsonObject& O, const TCHAR* K) { FString S; O.TryGetStringField(K, S); return S; };
	auto Num = [&](const FJsonObject& O, const TCHAR* K) { double V = 0; O.TryGetNumberField(K, V); return (int32)V; };

	const FString ClassName = Str(B, TEXT("class"));
	const FString Anc = Str(B, TEXT("ancestry"));
	const TSharedPtr<FJsonObject>* AbP = nullptr;
	B.TryGetObjectField(TEXT("abilities"), AbP);
	const FString KeyAb = Str(B, TEXT("keyability")).ToLower();
	const FString Cl = ClassName.ToLower();
	FString Kit = Cl == TEXT("fighter") || Cl == TEXT("rogue") || Cl == TEXT("wizard") || Cl == TEXT("cleric") ? Cl
		: KeyAb == TEXT("dex") ? TEXT("rogue") : KeyAb == TEXT("int") ? TEXT("wizard") : KeyAb == TEXT("wis") || KeyAb == TEXT("cha") ? TEXT("cleric") : TEXT("fighter");
	FString AncId = TEXT("human");
	for (const FAncestry& A : Ancestries()) if (Anc.ToLower().Contains(A.Id)) AncId = A.Id;

	FPC P = BuildPC(Str(B, TEXT("name")), AncId, Kit);
	P.ClassName = ClassName.IsEmpty() ? P.ClassName : ClassName;
	P.Ancestry = Anc.IsEmpty() ? P.Ancestry : Anc;
	P.Level = FMath::Clamp(Num(B, TEXT("level")), 1, 5);
	P.Xp = XpForLevel(P.Level);
	if (AbP)
		for (const TCHAR* K : { TEXT("str"), TEXT("dex"), TEXT("con"), TEXT("int"), TEXT("wis"), TEXT("cha") })
			P.Abil.Add(K, FMath::FloorToInt((Num(**AbP, K) - 10) / 2.f));

	const TSharedPtr<FJsonObject>* PrP = nullptr;
	if (B.TryGetObjectField(TEXT("proficiencies"), PrP))
	{
		const FJsonObject& Pr = **PrP;
		P.Ranks.Reset();
		for (const FString& S : SkillList()) if (const int32 V = Num(Pr, *S)) P.Ranks.Add(S, V / 2);
		P.PerRank = FMath::Max(1, Num(Pr, TEXT("perception")) / 2);
		P.Saves = { { TEXT("fort"), Num(Pr, TEXT("fortitude")) / 2 }, { TEXT("ref"), Num(Pr, TEXT("reflex")) / 2 }, { TEXT("will"), Num(Pr, TEXT("will")) / 2 } };
		P.WeaponRank = FMath::Max(1, FMath::Max(Num(Pr, TEXT("martial")), Num(Pr, TEXT("simple"))) / 2);
		P.ArmorRank = FMath::Max(1, FMath::Max(FMath::Max(Num(Pr, TEXT("heavy")), Num(Pr, TEXT("medium"))), FMath::Max(Num(Pr, TEXT("light")), Num(Pr, TEXT("unarmored")))) / 2);
		P.SpellRank = FMath::Max(1, Num(Pr, TEXT("castingArcane")) / 2 + Num(Pr, TEXT("castingDivine")) / 2 + Num(Pr, TEXT("castingOccult")) / 2 + Num(Pr, TEXT("castingPrimal")) / 2);
	}
	const TSharedPtr<FJsonObject>* AtP = nullptr;
	if (B.TryGetObjectField(TEXT("attributes"), AtP))
	{
		const FJsonObject& At = **AtP;
		P.HpPer = Num(At, TEXT("classhp")) + Num(At, TEXT("bonushpPerLevel")) + P.Abil.FindRef(TEXT("con"));
		P.HpMax = Num(At, TEXT("ancestryhp")) + Num(At, TEXT("bonushp")) + P.HpPer * P.Level;
		P.Hp = P.HpMax;
		if (const int32 Sp = Num(At, TEXT("speed"))) P.Speed = Sp + Num(At, TEXT("speedBonus"));
	}
	const TSharedPtr<FJsonObject>* AcP = nullptr;
	if (B.TryGetObjectField(TEXT("acTotal"), AcP))
		if (const int32 Ac = Num(**AcP, TEXT("acTotal"))) P.AcItem = Ac - Prof(P.ArmorRank, P.Level);
	const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
	if (B.TryGetArrayField(TEXT("weapons"), Arr) && Arr->Num())
	{
		const FJsonObject& Wp = *(*Arr)[0]->AsObject();
		P.WeaponName = Str(Wp, TEXT("display")).IsEmpty() ? Str(Wp, TEXT("name")) : Str(Wp, TEXT("display"));
		const FString Die = Str(Wp, TEXT("die"));
		if (Die.StartsWith(TEXT("d"))) P.WeaponDie = FCString::Atoi(*Die.Mid(1));
	}
	if (B.TryGetArrayField(TEXT("armor"), Arr) && Arr->Num()) P.ArmorName = Str(*(*Arr)[0]->AsObject(), TEXT("name"));
	const TSharedPtr<FJsonObject>* MoP = nullptr;
	if (B.TryGetObjectField(TEXT("money"), MoP)) P.Gold = Num(**MoP, TEXT("pp")) * 100 + Num(**MoP, TEXT("gp")) * 10 + Num(**MoP, TEXT("sp"));

	// everything else Pathbuilder knows, resolved against the rules library the way Pathmuncher does
	HollowLib::Load();
	P.Heritage = Str(B, TEXT("heritage")); P.Background = Str(B, TEXT("background")); P.Deity = Str(B, TEXT("deity"));
	auto Rows = [&](const TCHAR* Field, TFunctionRef<void(const TArray<TSharedPtr<FJsonValue>>&)> Fn)
	{
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		if (B.TryGetArrayField(Field, A))
			for (const TSharedPtr<FJsonValue>& V : *A) { const TArray<TSharedPtr<FJsonValue>>* Row = nullptr; if (V->TryGetArray(Row) && Row->Num()) Fn(*Row); }
	};
	// feats: [name, choice, type, level, ...]
	Rows(TEXT("feats"), [&](const TArray<TSharedPtr<FJsonValue>>& Row)
	{
		const FString Name = Row[0]->AsString();
		const int32 Lv = Row.Num() > 3 && Row[3]->Type == EJson::Number ? (int32)Row[3]->AsNumber() : 1;
		const FLibFeat* F = HollowLib::Feat(Name);
		P.Feats.Add({ Lv, F ? F->Name : Name });
	});
	P.Feats.Sort([](const TPair<int32, FString>& A, const TPair<int32, FString>& Bb) { return A.Key < Bb.Key; });

	// spellcasting: every list of every caster entry, plus focus spells wherever Pathbuilder tucks them
	if (B.TryGetArrayField(TEXT("spellCasters"), Arr))
		for (const TSharedPtr<FJsonValue>& V : *Arr)
		{
			const TSharedPtr<FJsonObject> Sc = V->AsObject();
			if (!Sc) continue;
			if (P.Tradition.IsEmpty()) { P.Tradition = Str(*Sc, TEXT("magicTradition")); P.bCaster = true; P.SpellAbil = Str(*Sc, TEXT("ability")); P.SpellRank = FMath::Max(1, Num(*Sc, TEXT("proficiency")) / 2); }
			const TArray<TSharedPtr<FJsonValue>>* Lists = nullptr;
			if (Sc->TryGetArrayField(TEXT("spells"), Lists))
				for (const TSharedPtr<FJsonValue>& Lv : *Lists)
				{
					const TArray<TSharedPtr<FJsonValue>>* Names = nullptr;
					if (Lv->AsObject() && Lv->AsObject()->TryGetArrayField(TEXT("list"), Names))
						for (const TSharedPtr<FJsonValue>& Nm : *Names) if (const FLibSpell* Sp = HollowLib::Spell(Nm->AsString())) P.Spells.AddUnique(Sp->Name);
				}
		}
	TFunction<void(const TSharedPtr<FJsonObject>&)> Focus = [&](const TSharedPtr<FJsonObject>& O)
	{
		if (!O) return;
		for (const auto& Kv : O->Values)
		{
			if (Kv.Value->Type == EJson::Object) Focus(Kv.Value->AsObject());
			else if (Kv.Value->Type == EJson::Array && (Kv.Key == TEXT("focusSpells") || Kv.Key == TEXT("focusCantrips")))
				for (const TSharedPtr<FJsonValue>& Nm : Kv.Value->AsArray()) if (const FLibSpell* Sp = HollowLib::Spell(Nm->AsString())) P.Spells.AddUnique(Sp->Name);
		}
	};
	const TSharedPtr<FJsonObject>* FocusP = nullptr;
	if (B.TryGetObjectField(TEXT("focus"), FocusP)) Focus(*FocusP);

	// gear: our hand-made items keep their field abilities; everything else comes from the rules library
	TArray<FString> Got, Missed;
	P.Inv.Bags[0].Slots.SetNum(20);   // an imported adventurer arrives with a proper pack
	auto Take = [&](const FString& Name, int32 Qty)
	{
		const FString Id = HollowItems::FromLibrary(Name);
		if (Id.IsEmpty()) { Missed.AddUnique(Name); return; }
		if (P.Inv.Add(Id, Qty) != EAddResult::Failed) Got.AddUnique(HollowItems::Find(Id)->Name);
	};
	Rows(TEXT("equipment"), [&](const TArray<TSharedPtr<FJsonValue>>& Row) { Take(Row[0]->AsString(), Row.Num() > 1 && Row[1]->Type == EJson::Number ? FMath::Max(1, (int32)Row[1]->AsNumber()) : 1); });
	for (const TCHAR* Field : { TEXT("weapons"), TEXT("armor") })
		if (B.TryGetArrayField(Field, Arr))
			for (const TSharedPtr<FJsonValue>& V : *Arr) if (V->AsObject()) Take(Str(*V->AsObject(), TEXT("name")), FMath::Max(1, Num(*V->AsObject(), TEXT("qty"))));
	if (const FLibItem* Wl = HollowLib::Item(P.WeaponName)) if (Wl->Die.StartsWith(TEXT("d"))) P.WeaponDie = FCString::Atoi(*Wl->Die.Mid(1));

	// the deck: your class kit's tricks, then two copies of each cantrip and one card per ranked spell
	const int32 MaxRank = FMath::Max(1, (P.Level + 1) / 2);
	int32 Ranked = 0;
	for (const FString& Sp : P.Spells)
	{
		const FLibSpell* S = HollowLib::Spell(Sp);
		if (!S || (!S->bCantrip && S->Rank > MaxRank)) continue;
		const FString Card = HollowData::SpellCard(S->Name, S->bCantrip || S->bFocus ? MaxRank : S->Rank);
		if (Card.IsEmpty()) continue;
		if (S->bCantrip) { P.Deck.Add(Card); P.Deck.Add(Card); }
		else if (Ranked++ < 12) P.Deck.Add(Card);
	}

	Recompute(P);
	PC = P;
	CreateName = P.Name; CreateAnc = AncId; CreateKit = Kit;
	bImported = true;
	PbStatus = FString::Printf(TEXT("Imported %s, level %d %s %s: %d feats, %d spells (%d as cards), %d kinds of gear.%s%s"), *P.Name, P.Level, *P.Ancestry, *P.ClassName,
		P.Feats.Num(), P.Spells.Num(), P.Deck.Num(), Got.Num(), HollowLib::IsLoaded() ? TEXT("") : TEXT(" (Rules library missing: run Tools/BuildLibrary.mjs.)"),
		Missed.Num() ? *(TEXT(" Not found: ") + FString::Join(Missed, TEXT(", ")) + TEXT(".")) : TEXT(""));
	PlaySfx(TEXT("confirm"));
	MarkUI();
}


/* ---------- Line of sight, cover and light ---------- */

void AHollowGameMode::ToggleLantern()
{
	if (!Hero || Mode != EHollowMode::Explore) return;
	bLanternLit = !bLanternLit;
	if (Hero->Lantern) Hero->Lantern->SetVisibility(bLanternLit);
	Toast(bLanternLit ? TEXT("You unhood your lantern.") : TEXT("You hood your lantern. Darkness hides you, but it hides them too."));
	PlaySfx(TEXT("click"), 0.5f);
	VisClock = 1.f;
	MarkUI();
}

FString AHollowGameMode::VisLabel() const
{
	static const TCHAR* L[] = { TEXT("Dark"), TEXT("Dim"), TEXT("Bright") };
	static const TCHAR* V[] = { TEXT("unseen"), TEXT("hidden"), TEXT("concealed"), TEXT("seen") };
	static const TCHAR* Cv[] = { TEXT(""), TEXT(", lesser cover"), TEXT(", cover"), TEXT(", greater cover") };
	return FString::Printf(TEXT("%s - %s%s"), L[(int32)HeroLight], V[(int32)HeroVis], HeroCover > 0 && HeroVis != EVis::Unseen ? Cv[HeroCover] : TEXT(""));
}

// Light at your feet: every point light on the island (lamps, braziers, campfires, beacons) plus your own lantern.
// Bright inside half a light's reach, dim to its edge. Open ground under the moon is dim; under canopy and in the
// fen it is dark. Then each watcher looks for you: three rays (head, chest, feet) decide line of sight and cover,
// and the light decides whether a line of sight is enough to see you.
void AHollowGameMode::TickVisibility(float Dt)
{
	VisClock += Dt;
	if (VisClock < 0.2f || Mode != EHollowMode::Explore || !Hero) return;
	const float Step = VisClock;
	VisClock = 0.f;
	const FVector Me = Hero->GetActorLocation();
	const FHollowHex* Here = Hexes.Find(Pos);
	ELight Light = Here && (Here->T == ETerr::Forest || Here->T == ETerr::Swamp || Here->T == ETerr::Ruins) ? ELight::Dark : ELight::Dim;
	if (Here && Here->T == ETerr::Town) Light = ELight::Dim;
	auto Lift = [&](ELight L) { if ((int32)L > (int32)Light) Light = L; };
	if (bLanternLit) Lift(ELight::Bright);
	for (AActor* A : LocalActors)
	{
		if (!IsValid(A) || A->IsHidden()) continue;
		TInlineComponentArray<UPointLightComponent*> Lights(A);
		for (UPointLightComponent* Pl : Lights)
		{
			if (!Pl->IsVisible() || Pl->Intensity <= 0.f) continue;
			const float D = FVector::Dist(Pl->GetComponentLocation(), Me);
			const float R = Pl->AttenuationRadius;
			if (D < R * 0.45f) Lift(ELight::Bright);
			else if (D < R) Lift(ELight::Dim);
		}
	}
	// watchers: the foes standing at this hex's landmark
	EVis Best = EVis::Unseen;
	int32 BestCover = 0, Seeing = 0;
	FCollisionQueryParams Q(TEXT("HollowSight"), false, Hero);
	for (const TWeakObjectPtr<AActor>& W : WatcherActors)
	{
		AActor* A = W.Get();
		if (!A) continue;
		Q.ClearIgnoredSourceObjects();
		Q.AddIgnoredActor(Hero);
		Q.AddIgnoredActor(A);
		const FVector Eye = A->GetActorLocation() + FVector(0, 0, 120.f);
		const float Dist = FVector::Dist2D(Eye, Me);
		if (Dist > 2600.f) continue;
		int32 Blocked = 0;
		for (const float Z : { 70.f, 10.f, -70.f })
		{
			FHitResult H;
			if (GetWorld()->LineTraceSingleByChannel(H, Eye, Me + FVector(0, 0, Z), ECC_Visibility, Q)) Blocked++;
		}
		if (Blocked >= 3) continue;   // no line of sight at all
		const int32 Cover = Blocked;  // 0 none, 1 lesser, 2 standard (greater needs walls we place on purpose)
		// how far a watcher can pick you out depends on the light you stand in
		const float Reach = Light == ELight::Bright ? 2400.f : Light == ELight::Dim ? 1100.f : 320.f;
		EVis V = Dist <= Reach ? (Light == ELight::Bright && Cover == 0 ? EVis::Observed : EVis::Concealed) : EVis::Hidden;
		if (Cover >= 2 && V == EVis::Observed) V = EVis::Concealed;
		if (V >= EVis::Concealed) Seeing++;
		if ((int32)V > (int32)Best) { Best = V; BestCover = Cover; }
	}
	const bool bChanged = Best != HeroVis || Light != HeroLight || Seeing != Watchers || BestCover != HeroCover;
	HeroVis = Best; HeroCover = BestCover; Watchers = Seeing; HeroLight = Light;
	// staying in plain view for a moment gets you noticed; concealment only slows it down
	if (!bNoticed && Best >= EVis::Concealed)
	{
		NoticeClock += Step * (Best == EVis::Observed ? 1.f : 0.35f);
		if (NoticeClock > 1.6f)
		{
			bNoticed = true;
			Toast(TEXT("They've spotted you. No surprising them now."));
			Log(TEXT("Spotted by the creatures ahead."));
			PlaySfx(TEXT("alert"), 0.7f);
		}
	}
	else if (!bNoticed) NoticeClock = FMath::Max(0.f, NoticeClock - Step * 0.5f);
	if (bChanged) MarkUI();
}


/* ---------- Availability: shops and treasure ---------- */

FString AHollowGameMode::RollLoot(int32 Level, int32 Seed) const
{
	if (!HollowLib::Load()) return FString();
	FRandomStream Rng(Seed);
	// pf2e treasure leans common; uncommon finds are a treat, rare ones a story
	const float Roll = Rng.FRand();
	const FString Want = Roll < 0.02f ? TEXT("rare") : Roll < 0.15f ? TEXT("uncommon") : TEXT("common");
	TArray<const FLibItem*> Pool;
	for (const FLibItem& I : HollowLib::Items())
		if (I.Rarity == Want && I.Level <= Level + 1 && I.Level >= FMath::Max(0, Level - 3) && I.Price > 0.f && I.Type != TEXT("treasure") && I.Type != TEXT("kit") && I.Type != TEXT("book"))
			Pool.Add(&I);
	if (!Pool.Num()) return FString();
	return HollowItems::FromLibrary(Pool[Rng.RandRange(0, Pool.Num() - 1)]->Name);
}

TArray<FString> AHollowGameMode::ShopStock(const FNpcDef& N) const
{
	TArray<FString> Out = N.Stock;
	if (!HollowLib::Load() || N.Service.IsEmpty() || N.Service == TEXT("inn")) return Out;
	// the stock turns over every day: seeded by the day and the trader
	FRandomStream Rng(Day * 7919 + GetTypeHash(N.Id));
	const int32 Lv = SettlementLevel();
	const bool bCity = Lv >= 6;
	const bool bTemple = N.Service == TEXT("temple");
	TArray<const FLibItem*> Pool;
	for (const FLibItem& I : HollowLib::Items())
	{
		if (I.Level > Lv || I.Price <= 0.f || I.Price > 400.f) continue;
		if (bTemple ? !(I.Type == TEXT("consumable") && (I.Traits.Contains(TEXT("healing")) || I.Traits.Contains(TEXT("holy")) || I.Traits.Contains(TEXT("vitality"))))
			: !(I.Type == TEXT("consumable") || I.Type == TEXT("weapon") || I.Type == TEXT("armor") || I.Type == TEXT("shield") || I.Type == TEXT("equipment"))) continue;
		if (I.Rarity == TEXT("common")) Pool.Add(&I);
		else if (I.Rarity == TEXT("uncommon") && Rng.FRand() < (bCity ? 0.15f : 0.012f)) Pool.Add(&I);   // ~10-15% of restocks show one
		else if (I.Rarity == TEXT("rare") && bCity && Rng.FRand() < 0.002f) Pool.Add(&I);
	}
	for (int32 i = 0; i < (bTemple ? 4 : 10) && Pool.Num(); ++i)
	{
		const int32 K = Rng.RandRange(0, Pool.Num() - 1);
		const FString Id = HollowItems::FromLibrary(Pool[K]->Name);
		if (!Id.IsEmpty()) Out.AddUnique(Id);
		Pool.RemoveAtSwap(K);
	}
	return Out;
}
