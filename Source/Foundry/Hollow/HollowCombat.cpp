// Battle: an FFX-style conditional turn timeline (faster creatures act more often, Tempo cards bend it),
// PF2e three-action turns with the multiple attack penalty and degrees of success, and a hand of action cards
// that spend those same actions.
#include "HollowGameMode.h"

#include "Camera/CameraActor.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "HollowArt.h"
#include "HollowHero.h"
#include "HollowRules.h"
#include "HollowLibrary.h"

using namespace HollowRules;
using HollowArt::EShape;

void AHollowGameMode::Beat(float Delay, TFunction<void()> Fn)
{
	if (Beats.Num() == 0) BeatClock = Delay;
	Beats.Add({ Delay, MoveTemp(Fn) });
}

FVector AHollowGameMode::FighterPos(int32 Idx) const
{
	if (!Combat.IsSet() || !Combat->F.IsValidIndex(Idx)) return ArenaOrigin;
	return Combat->F[Idx].Home + FVector(0, 0, Combat->F[Idx].bBoss ? 200.f : 120.f);
}

void AHollowGameMode::CLog(const FString& S)
{
	if (!Combat.IsSet()) return;
	Combat->Log.Insert(S, 0);
	if (Combat->Log.Num() > 30) Combat->Log.SetNum(30);
	MarkUI();
}

bool AHollowGameMode::IsHeroTurn() const
{
	return Combat.IsSet() && !Combat->bOver && Combat->Active == 0 && !Combat->bBusy;
}

int32 AHollowGameMode::HeroAC() const
{
	int32 Ac = AC(PC);
	if (Combat.IsSet())
	{
		const FFighter& H = Combat->F[0];
		Ac += H.GuardAc - H.Frightened - (H.OffGuardTurns > 0 || Flanked(0) ? 2 : 0);
	}
	return Ac;
}

int32 AHollowGameMode::FirstLivingFoe() const
{
	if (!Combat.IsSet()) return INDEX_NONE;
	for (int32 i = 1; i < Combat->F.Num(); ++i) if (!Combat->F[i].bDead) return i;
	return INDEX_NONE;
}

void AHollowGameMode::SetTarget(int32 Idx)
{
	if (!Combat.IsSet() || !Combat->F.IsValidIndex(Idx) || Idx == 0 || Combat->F[Idx].bDead) return;
	Combat->Target = Idx;
	RefreshGridHighlight();
	MarkUI();
}

void AHollowGameMode::CycleTarget()
{
	if (!Combat.IsSet()) return;
	const int32 N = Combat->F.Num();
	for (int32 k = 1; k < N; ++k)
	{
		const int32 i = 1 + (Combat->Target - 1 + k) % (N - 1);
		if (!Combat->F[i].bDead) { SetTarget(i); return; }
	}
}

TArray<int32> AHollowGameMode::TurnPreview(int32 Count) const
{
	TArray<int32> Out;
	if (!Combat.IsSet()) return Out;
	TArray<float> Ctb;
	for (const FFighter& F : Combat->F) Ctb.Add(F.Ctb);
	if (Combat->Active >= 0 && !Combat->bOver) { Out.Add(Combat->Active); Ctb[Combat->Active] += Combat->F[Combat->Active].Delay() * (Combat->Active == 0 && Combat->bQuicken ? 0.5f : 1.f); }
	while (Out.Num() < Count)
	{
		int32 Best = INDEX_NONE;
		for (int32 i = 0; i < Ctb.Num(); ++i)
			if (!Combat->F[i].bDead && (Best == INDEX_NONE || Ctb[i] < Ctb[Best])) Best = i;
		if (Best == INDEX_NONE) break;
		Out.Add(Best);
		Ctb[Best] += Combat->F[Best].Delay();
	}
	return Out;
}

/* ---------- Setup ---------- */

void AHollowGameMode::StartCombat(const FString& FoeIds, bool bAmbush, bool bWeaken)
{
	FCombatState C;
	C.Hex = Pos;
	FFighter Hf;
	Hf.Name = PC.Name; Hf.bHero = true; Hf.Speed = PC.Speed;
	Hf.Per = SkillMod(PC, TEXT("perception"));
	C.F.Add(Hf);
	TArray<FString> Ids;
	FoeIds.ParseIntoArray(Ids, TEXT("+"));
	for (const FString& Id : Ids)
	{
		const FFoeDef* D = HollowData::Foes().Find(Id);
		if (!D) continue;
		FFighter F;
		F.Name = D->Name; F.FoeId = D->Id; F.Hp = F.MaxHp = D->Hp; F.Ac = D->Ac; F.Atk = D->Atk; F.Fort = D->Fort; F.Ref = D->Ref;
		F.Will = D->Will; F.Per = D->Per; F.Speed = D->Speed; F.Dmg = D->Dmg; F.Weapon = D->Weapon; F.Special = D->Special;
		F.bUndead = D->bUndead; F.bBoss = D->bBoss; F.Regen = D->Regen;
		if (bWeaken && F.bBoss) { F.Hp -= 15; F.Regen = 0; }
		if (bAmbush) F.OffGuardTurns = 1;
		C.F.Add(F);
	}
	if (C.F.Num() < 2) return;
	for (int32 i = 0; i < C.F.Num(); ++i)
	{
		FFighter& F = C.F[i];
		const int32 Init = D(20) + F.Per;
		F.Ctb = F.Delay() * (1.f - FMath::Clamp(Init, 0, 32) / 40.f);
		if (bAmbush && i == 0) F.Ctb = 0.f;
	}
	C.Draw = PC.Deck.FilterByPredicate([this](const FString& Id) { return !SpentCards.Contains(Id); });
	for (int32 i = C.Draw.Num() - 1; i > 0; --i) C.Draw.Swap(i, FMath::RandRange(0, i));
	C.Target = 1;
	Combat = C;
	for (int32 i = 1; i < Combat->F.Num(); ++i) ChooseIntent(Combat->F[i]);

	Mode = EHollowMode::Combat;
	BuildArena();
	SetViewTo(ArenaCam, 0.8f);
	CLog(bAmbush ? TEXT("Ambush! You strike before they can react.") : TEXT("Battle begins. Watch the turn order."));
	if (HasBuff(TEXT("blessed"))) { Combat->BlessTurns = 3; CLog(TEXT("The Dawn's blessing is still on you: +1 to attack rolls for 3 turns.")); }
	if (HasBuff(TEXT("resolve"))) { HealHero(5, TEXT("Fresh resolve")); }
	Combat->bBusy = true;
	Beat(1.4f, [this] { if (Combat.IsSet()) { Combat->bBusy = false; AdvanceTurn(); } });
	PlayMusic(TEXT("horror_atmosphere_loop"), 0.6f);
	PlaySfx(TEXT("metal"), 0.7f);
	MarkUI();
}

void AHollowGameMode::BuildArena()
{
	for (AActor* A : ArenaActors) if (IsValid(A)) A->Destroy();
	ArenaActors.Reset();
	UWorld* W = GetWorld();
	const FHollowHex& H = Hexes.FindChecked(Pos);
	const FVector O = ArenaOrigin;
	AActor* G = HollowArt::NewActor(W, O);
	ArenaActors.Add(G);
	HollowArt::HexPrism(G, 1500.f, 400.f, TerrInfo(H.T).Ground, true);
	HollowArt::HexPrism(G, 1530.f, 50.f, TerrInfo(H.T).Ground * 0.55f, false, FVector(0, 0, -350.f));
	HollowArt::Part(G, EShape::Cylinder, FVector(0, 0, 2), FVector(1500, 900, 4), TerrInfo(H.T).Ground * 0.85f);

	FRandomStream Rng(Pos.X * 13 + Pos.Y * 101 + 5);
	for (int32 i = 0; i < 18; ++i)
	{
		const float A = Rng.FRandRange(0.f, 2 * PI);
		const FVector P = O + FVector(FMath::Cos(A) * 1300, FMath::Sin(A) * 1100, 0);
		if (P.Y > O.Y + 300) continue;   // keep the camera side open
		AActor* Prop = nullptr;
		const float Roll = Rng.FRand();
		if (Roll < 0.35f) Prop = HollowArt::Boulders(W, P, Rng.FRandRange(0.9f, 1.4f), Rng.RandRange(0, 99999));
		else if (Roll < 0.55f) Prop = HollowArt::DeadTree(W, P, Rng.FRandRange(1.f, 1.5f), Rng.RandRange(0, 99999));
		else if (H.T == ETerr::Ruins || H.T == ETerr::Keep) Prop = HollowArt::Pillar(W, P, Rng.FRandRange(180.f, 380.f), Rng.FRand() < 0.5f);
		else if (H.T == ETerr::Town) Prop = HollowArt::Lantern(W, P);
		else if (Roll < 0.8f) Prop = HollowArt::PineTree(W, P, Rng.FRandRange(1.f, 1.5f), FLinearColor::Black);
		else Prop = HollowArt::AutumnTree(W, P, Rng.FRandRange(0.9f, 1.3f));
		if (Prop) ArenaActors.Add(Prop);
	}
	for (int32 s2 : { -1, 1 })
	{
		AActor* Torch = HollowArt::Campfire(W, O + FVector(s2 * 700.f, -500.f, 0));
		ArenaActors.Add(Torch);
	}
	ArenaActors.Add(HollowArt::FogBank(W, O + FVector(0, -900, 0), 1500.f, 0.35f, FLinearColor(0.55f, 0.55f, 0.7f)));
	{
		AActor* Rim = HollowArt::NewActor(W, O + FVector(0, 0, 900));
		HollowArt::Light(Rim, FVector::ZeroVector, FLinearColor(0.55f, 0.45f, 1.f), 60.f, 2400.f, true);
		ArenaActors.Add(Rim);
	}

	const int32 NumFoes = Combat->F.Num() - 1;
	for (int32 i = 0; i < Combat->F.Num(); ++i)
	{
		FFighter& F = Combat->F[i];
		F.Cell = F.bHero ? FIntPoint(-1, 3) : FIntPoint((i - 1) * 2 - (NumFoes - 1) + 1, -3);
		const FVector Home = CellWorld(F.Cell);
		AActor* A = HollowArt::NewActor(W, Home, F.bHero ? 0.f : 180.f);
		HollowArt::Part(A, EShape::Cylinder, FVector(0, 0, 100), FVector(150, 150, 200), FLinearColor::Black, FRotator::ZeroRotator, true)->SetVisibility(false);
		HollowArt::Part(A, EShape::Cylinder, FVector(0, 0, 2), FVector(150, 150, 4), FLinearColor(0.08f, 0.08f, 0.1f));
		if (F.bHero)
		{
			const FString Weapon = PC.Kit == TEXT("wizard") ? TEXT("staff") : PC.Kit == TEXT("rogue") ? TEXT("dagger") : PC.Kit == TEXT("cleric") ? TEXT("mace") : TEXT("sword");
			HollowArt::Chibi(A, nullptr, FVector::ZeroVector, PC.Body, PC.Skin, PC.Hat, PC.HatStyle, 1.f, Weapon);
		}
		else HollowArt::Foe(A, nullptr, F.FoeId, FVector::ZeroVector);
		F.Puppet = A;
		F.Home = Home;
		ArenaActors.Add(A);
	}
	BuildBattleGrid();
	ArenaYaw = ArenaYawTarget = 90.f;
}

/* ---------- Turn flow ---------- */

void AHollowGameMode::AdvanceTurn()
{
	if (!Combat.IsSet() || Combat->bOver) return;
	int32 Best = INDEX_NONE;
	for (int32 i = 0; i < Combat->F.Num(); ++i)
		if (!Combat->F[i].bDead && (Best == INDEX_NONE || Combat->F[i].Ctb < Combat->F[Best].Ctb)) Best = i;
	if (Best == INDEX_NONE) return;
	Combat->Active = Best;
	if (Best == 0) StartHeroTurn();
	else StartFoeTurn(Best);
	MarkUI();
}

void AHollowGameMode::DrawCards(int32 N)
{
	FCombatState& C = Combat.GetValue();
	for (int32 i = 0; i < N; ++i)
	{
		if (C.Draw.Num() == 0)
		{
			if (C.Discard.Num() == 0) return;
			C.Draw = MoveTemp(C.Discard);
			C.Discard.Reset();
			for (int32 k = C.Draw.Num() - 1; k > 0; --k) C.Draw.Swap(k, FMath::RandRange(0, k));
		}
		C.Hand.Add(C.Draw.Pop());
		C.HandSerial.Add(C.NextSerial++);
		PlaySfx(TEXT("draw"), 0.5f);
	}
}

void AHollowGameMode::StartHeroTurn()
{
	FCombatState& C = Combat.GetValue();
	FFighter& H = C.F[0];
	C.Actions = FMath::Max(1, 3 - H.Slowed);
	if (H.Slowed) CLog(TEXT("You're slowed and lose an action."));
	H.Slowed = 0;
	C.Attacks = 0;
	H.GuardAc = 0;
	H.ShieldHp = 0;
	C.bRiposte = false;
	C.bQuicken = false;
	if (C.BlessTurns > 0) C.BlessTurns--;
	if (C.HymnTurns > 0) C.HymnTurns--;
	if (C.Hand.Num() < 4) DrawCards(4 - C.Hand.Num());
	if (C.Target <= 0 || !C.F.IsValidIndex(C.Target) || C.F[C.Target].bDead) C.Target = FirstLivingFoe();
	C.bBusy = false;
	CLog(FString::Printf(TEXT("Your turn. %d actions."), C.Actions));
}

void AHollowGameMode::EndHeroTurn()
{
	if (!IsHeroTurn()) return;
	FCombatState& C = Combat.GetValue();
	for (int32 i = 1; i < C.F.Num(); ++i) if (C.F[i].OffGuardTurns > 0) C.F[i].OffGuardTurns--;
	C.F[0].OffGuardTurns = 0;
	EndActorTurn(0);
}

void AHollowGameMode::EndActorTurn(int32 Idx)
{
	if (!Combat.IsSet() || Combat->bOver) return;
	FCombatState& C = Combat.GetValue();
	FFighter& F = C.F[Idx];
	F.Ctb += F.Delay() * (Idx == 0 && C.bQuicken ? 0.5f : 1.f);
	if (F.Frightened > 0) F.Frightened--;
	if (Idx == C.F.Num() - 1 || Idx == 0) C.Round++;
	C.bBusy = true;
	Beat(0.85f, [this] { if (Combat.IsSet() && !Combat->bOver) { Combat->bBusy = false; AdvanceTurn(); } });
	MarkUI();
}

bool AHollowGameMode::Spend(int32 Cost)
{
	if (!IsHeroTurn() || Combat->Actions < Cost) return false;
	Combat->Actions -= Cost;
	return true;
}

int32 AHollowGameMode::RollD20()
{
	int32 N = D(20);
	if (Combat.IsSet() && Combat->bLucky)
	{
		const int32 N2 = D(20);
		CLog(FString::Printf(TEXT("Lucky Coin: rolled %d and %d."), N, N2));
		N = FMath::Max(N, N2);
		Combat->bLucky = false;
	}
	return N;
}

int32 AHollowGameMode::HeroStrike(int32 T, int32 Bonus, int32 ExtraDice, int32 MapWeight, int32 FixedPenalty)
{
	FCombatState& C = Combat.GetValue();
	if (!C.F.IsValidIndex(T) || C.F[T].bDead) T = FirstLivingFoe();
	if (T == INDEX_NONE) return 0;
	FFighter& Foe = C.F[T];
	const int32 Penalty = FixedPenalty != 999 ? FixedPenalty : (C.Attacks == 0 ? 0 : C.Attacks == 1 ? -5 : -10);
	const int32 Nat = RollD20();
	const int32 Mod = AtkMod(PC) + (C.BlessTurns > 0 ? 1 : 0) - C.F[0].Frightened + Penalty + Bonus;
	const int32 Dc = Foe.Ac - Foe.Frightened - (Foe.OffGuardTurns > 0 || Flanked(T) ? 2 : 0) + CoverBetween(0, T);
	const int32 Deg = Degree(Nat + Mod, Dc, Nat);
	C.Attacks += MapWeight;
	Lunge(0, T);
	PlaySfx(TEXT("swing"), 0.8f);
	FString Line = FString::Printf(TEXT("%s: d20 %d %s = %d vs AC %d. %s."), *PC.WeaponName, Nat, *Sgn(Mod), Nat + Mod, Dc, DegreeName(Deg));
	if (Deg >= 2)
	{
		int32 Dmg = 0;
		for (int32 i = 0; i < 1 + ExtraDice; ++i) Dmg += D(PC.WeaponDie);
		Dmg += DmgBonus(PC) + (C.HymnTurns > 0 ? 2 : 0);
		if (PC.Kit == TEXT("rogue") && Foe.OffGuardTurns > 0) { const int32 S = D(6); Dmg += S; Line += FString::Printf(TEXT(" Sneak attack +%d."), S); }
		Dmg = FMath::Max(1, Dmg);
		if (Deg == 3) Dmg *= 2;
		CLog(Line);
		if (Deg == 3) NiagaraAt(TEXT("RadialBurst"), FighterPos(T), 1.2f);
		HurtFoe(T, Dmg, Deg == 3);
	}
	else
	{
		CLog(Line);
		FloatText(T, TEXT("miss"), FColor(220, 220, 220));
	}
	return Deg;
}

int32 AHollowGameMode::HeroCheck(const FString& Skill, int32 Dc, const FString& What)
{
	const int32 Nat = RollD20();
	const int32 Mod = SkillMod(PC, Skill) - Combat->F[0].Frightened;
	const int32 Deg = Degree(Nat + Mod, Dc, Nat);
	CLog(FString::Printf(TEXT("%s (%s): d20 %d %s = %d vs DC %d. %s."), *What, *SkillName(Skill), Nat, *Sgn(Mod), Nat + Mod, Dc, DegreeName(Deg)));
	return Deg;
}

int32 AHollowGameMode::HeroSave(const FString& Save, int32 Dc, const FString& What)
{
	const int32 Nat = D(20);
	const int32 Mod = SaveMod(PC, Save) - Combat->F[0].Frightened;
	const int32 Deg = Degree(Nat + Mod, Dc, Nat);
	static const TMap<FString, FString> Names = { { TEXT("fort"), TEXT("Fortitude") }, { TEXT("ref"), TEXT("Reflex") }, { TEXT("will"), TEXT("Will") } };
	CLog(FString::Printf(TEXT("%s: your %s save d20 %d %s = %d vs DC %d. %s."), *What, *Names.FindRef(Save), Nat, *Sgn(Mod), Nat + Mod, Dc, DegreeName(Deg)));
	return Deg;
}

void AHollowGameMode::HurtFoe(int32 Idx, int32 Dmg, bool bCrit)
{
	FFighter& F = Combat->F[Idx];
	if (F.bDead) return;
	F.Hp -= Dmg;
	FloatText(Idx, FString::Printf(TEXT("%d%s"), Dmg, bCrit ? TEXT("!") : TEXT("")), bCrit ? FColor(255, 200, 60) : FColor::White);
	Shake(Idx);
	PlaySfx(bCrit ? TEXT("metal") : TEXT("hit"));
	NiagaraAt(bCrit ? TEXT("/Game/Sparks_Embers/Niagara/NS_Sparks03") : TEXT("/Game/NiagaraExamples/FX_Weapons/Impacts/NS_Impact_Metal"), FighterPos(Idx), 1.f);
	Burst(FighterPos(Idx), TEXT("spark_05"), bCrit ? FLinearColor(1.f, 0.7f, 0.2f) : FLinearColor(1.f, 0.85f, 0.7f), bCrit ? 22 : 12, 420.f, 0.5f, 34.f, 60.f, 600.f);
	if (F.Hp <= 0)
	{
		F.Hp = 0;
		F.bDead = true;
		CLog(FString::Printf(TEXT("%s falls!"), *F.Name));
		if (AActor* P = F.Puppet.Get()) { FAnim A; A.A = P; A.Dur = 0.9f; A.Kind = 3; Anims.Add(A); }
		Burst(FighterPos(Idx), TEXT("smoke_07"), FLinearColor(0.35f, 0.15f, 0.5f), 18, 200.f, 1.6f, 120.f, 120.f, 0.f, 300.f);
		NiagaraAt(TEXT("SimpleExplosion"), FighterPos(Idx), 0.8f);
		if (Combat->Target == Idx) Combat->Target = FirstLivingFoe();
	}
	CheckCombatEnd();
}

void AHollowGameMode::HurtHero(int32 Dmg, bool bCrit, int32 FromIdx)
{
	FFighter& H = Combat->F[0];
	if (H.ShieldHp > 0)
	{
		const int32 Absorb = FMath::Min(H.ShieldHp, Dmg);
		H.ShieldHp -= Absorb;
		Dmg -= Absorb;
		if (Absorb) CLog(FString::Printf(TEXT("Your shield absorbs %d."), Absorb));
	}
	PC.Hp = FMath::Max(0, PC.Hp - Dmg);
	FloatText(0, FString::Printf(TEXT("%d%s"), Dmg, bCrit ? TEXT("!") : TEXT("")), FColor(255, 110, 90));
	Shake(0);
	PlaySfx(TEXT("hit"));
	Burst(FighterPos(0), TEXT("spark_06"), FLinearColor(1.f, 0.25f, 0.15f), 12, 380.f, 0.5f, 34.f, 60.f, 600.f);
	CheckCombatEnd();
}

void AHollowGameMode::HealHero(int32 Amount, const FString& Why)
{
	const int32 H = FMath::Min(Amount, PC.HpMax - PC.Hp);
	PC.Hp += H;
	CLog(FString::Printf(TEXT("%s: +%d HP."), *Why, H));
	FloatText(0, FString::Printf(TEXT("+%d"), H), FColor(140, 255, 140));
	PlaySfx(TEXT("magic"), 0.7f);
	NiagaraAt(TEXT("/Game/NiagaraExamples/FX_PickUp/NS_Pickup_Success"), FighterPos(0) - FVector(0, 0, 100), 1.f);
	Burst(FighterPos(0) - FVector(0, 0, 80), TEXT("light_02"), FLinearColor(0.4f, 1.f, 0.5f), 16, 80.f, 1.4f, 40.f, 160.f);
}

/* ---------- Hero commands ---------- */

void AHollowGameMode::CombatStrike()
{
	if (!IsHeroTurn() || !CanReach(Combat->Target, HeroWeaponRange()) || !Spend(1)) return;
	HeroStrike(Combat->Target, 0, 0, 1);
	if (Combat.IsSet() && !Combat->bOver && Combat->Actions == 0) CombatEndTurn();
	MarkUI();
}

void AHollowGameMode::CombatGuard()
{
	if (!Spend(1)) return;
	Combat->F[0].GuardAc += 2;
	CLog(TEXT("You raise your guard: +2 AC until your next turn."));
	if (Combat->Actions == 0) CombatEndTurn();
	MarkUI();
}

void AHollowGameMode::CombatPotion()
{
	if (!PC.Inv.Has(TEXT("potion")) || !Spend(1)) return;
	PC.Inv.Take(TEXT("potion"));
	HealHero(R(TEXT("2d8+4")), TEXT("Healing potion"));
	if (Combat->Actions == 0) CombatEndTurn();
	MarkUI();
}

void AHollowGameMode::CombatHoly()
{
	if (!PC.Inv.Has(TEXT("holy")) || !Spend(1)) return;
	PC.Inv.Take(TEXT("holy"));
	const int32 T = Combat->Target;
	FFighter& F = Combat->F[T];
	Lunge(0, T);
	if (F.bUndead)
	{
		F.bRegenBlocked = true;
		CLog(FString::Printf(TEXT("Holy water sears %s. Its wounds can't knit this round."), *F.Name));
		HurtFoe(T, R(TEXT("2d6+2")), false);
	}
	else
	{
		CLog(FString::Printf(TEXT("Holy water splashes %s. It's just wet now."), *F.Name));
		HurtFoe(T, 1, false);
	}
	if (Combat.IsSet() && !Combat->bOver && Combat->Actions == 0) CombatEndTurn();
	MarkUI();
}

void AHollowGameMode::CombatFlee()
{
	if (!IsHeroTurn() || Combat->Actions < 2) return;
	for (int32 i = 1; i < Combat->F.Num(); ++i)
		if (Combat->F[i].bBoss && !Combat->F[i].bDead) { Toast(TEXT("Varek bars the way. There is no running from this.")); return; }
	Spend(2);
	int32 Dc = 0;
	for (int32 i = 1; i < Combat->F.Num(); ++i) if (!Combat->F[i].bDead) Dc = FMath::Max(Dc, 10 + Combat->F[i].Per);
	const FString Skill = BestSkill(PC, { TEXT("acrobatics"), TEXT("athletics"), TEXT("stealth") });
	if (HeroCheck(Skill, Dc, TEXT("Flee")) >= 2)
	{
		Combat->bOver = true;
		Combat->bFled = true;
		CLog(TEXT("You escape!"));
		Combat->bBusy = true;
		Beat(0.8f, [this] { FinishCombat(); });
	}
	else if (Combat->Actions == 0) CombatEndTurn();
	MarkUI();
}

void AHollowGameMode::CombatEndTurn()
{
	if (!IsHeroTurn()) return;
	EndHeroTurn();
}

void AHollowGameMode::PlayCard(int32 HandIndex)
{
	if (!IsHeroTurn() || !Combat->Hand.IsValidIndex(HandIndex)) return;
	const FString Id = Combat->Hand[HandIndex];
	const FCardDef* Card = HollowData::Cards().Find(Id);
	if (!Card) return;
	if (!Combat->F.IsValidIndex(Combat->Target) || Combat->Target == 0 || Combat->F[Combat->Target].bDead) Combat->Target = FirstLivingFoe();
	if (Combat->Target == INDEX_NONE) return;
	if (Card->bNeedsTarget)
	{
		int32 RangeFt = Card->Kind == TEXT("Attack") ? HeroWeaponRange() : 30;
		if (Card->Fx == ECardFx::LibSpell)
			if (const FLibSpell* Sp = HollowLib::Spell(Card->Spell)) RangeFt = Sp->Range.Contains(TEXT("touch")) ? 5 : FMath::Max(5, FCString::Atoi(*Sp->Range));
		if (!CanReach(Combat->Target, RangeFt)) return;
	}
	if (!Spend(Card->Cost)) return;
	FCombatState& C = Combat.GetValue();
	C.Hand.RemoveAt(HandIndex);
	if (C.HandSerial.IsValidIndex(HandIndex)) C.HandSerial.RemoveAt(HandIndex);
	if (Card->bExhaust) { SpentCards.Add(Id); CLog(FString::Printf(TEXT("%s is spent until you rest."), *Card->Name)); }
	else C.Discard.Add(Id);
	PlayedCard = Id;
	PlayedAt = Now();
	PlaySfx(TEXT("card"));
	Burst(FighterPos(0) - FVector(0, 0, 110), TEXT("circle_05"), Card->Col, 1, 0.f, 0.6f, 60.f, 0.f, 0.f, 520.f);
	Burst(FighterPos(0), Card->Kind == TEXT("Spell") ? TEXT("magic_03") : Card->Kind == TEXT("Attack") ? TEXT("slash_02") : TEXT("star_06"), Card->Col, 14, 260.f, 0.9f, 50.f, 120.f);
	if (Card->bNeedsTarget && C.F.IsValidIndex(C.Target)) Burst(FighterPos(C.Target), TEXT("twirl_02"), Card->Col, 8, 160.f, 0.8f, 70.f, 80.f);
	const int32 T = C.Target;
	CLog(FString::Printf(TEXT("You play %s."), *Card->Name));
	auto Foe = [&]() -> FFighter& { return Combat->F[T]; };
	switch (Card->Fx)
	{
	case ECardFx::PowerAttack: HeroStrike(T, 0, 1, 2); break;
	case ECardFx::DoubleSlice:
	{
		const int32 Pen = C.Attacks == 0 ? 0 : C.Attacks == 1 ? -5 : -10;
		HeroStrike(T, 0, 0, 0, Pen);
		if (Combat.IsSet() && !Combat->bOver) Beat(0.75f, [this, Pen] { if (Combat.IsSet() && !Combat->bOver) { HeroStrike(Combat->Target, 0, 0, 0, Pen); MarkUI(); } });
		C.Attacks += 2;
		break;
	}
	case ECardFx::SuddenCharge:
		if (HeroStrike(T, 2, 0, 1) >= 2 && Combat.IsSet() && T != INDEX_NONE && Combat->F.IsValidIndex(T))
		{
			Combat->F[T].Ctb += Combat->F[T].Delay() * 0.35f;
			CLog(TEXT("The charge knocks it back on the timeline."));
		}
		break;
	case ECardFx::Intimidate:
	{
		const int32 Deg = HeroCheck(TEXT("intimidation"), Foe().Will + 10, TEXT("Demoralize"));
		if (Deg >= 2) { Foe().Frightened = FMath::Max(Foe().Frightened, Deg == 3 ? 2 : 1); FloatText(T, TEXT("frightened"), FColor(200, 160, 255)); if (AActor* Pp = Foe().Puppet.Get()) HollowArt::NiagaraPart(Pp, TEXT("/Game/NiagaraExamples/FX_Player/NS_Player_DeBuff_Looping"), FVector::ZeroVector, 1.f); }
		break;
	}
	case ECardFx::Feint:
	{
		const int32 Deg = HeroCheck(TEXT("deception"), Foe().Per + 10, TEXT("Feint"));
		if (Deg >= 2) { Foe().OffGuardTurns = FMath::Max(Foe().OffGuardTurns, Deg == 3 ? 2 : 1); FloatText(T, TEXT("off-guard"), FColor(255, 220, 120)); }
		else if (Deg == 0) { C.F[0].OffGuardTurns = 1; CLog(TEXT("Your feint leaves you exposed.")); }
		break;
	}
	case ECardFx::Tumble:
	{
		const int32 Deg = HeroCheck(TEXT("acrobatics"), Foe().Ref + 10, TEXT("Tumble Through"));
		if (Deg >= 2) { Foe().OffGuardTurns = FMath::Max(Foe().OffGuardTurns, 1); C.F[0].GuardAc += 1; FloatText(T, TEXT("off-guard"), FColor(255, 220, 120)); }
		break;
	}
	case ECardFx::Hamstring:
	{
		const int32 Deg = HeroCheck(TEXT("athletics"), Foe().Fort + 10, TEXT("Hamstring"));
		if (Deg >= 2) { Foe().Ctb += Foe().Delay() * 0.6f; CLog(TEXT("It staggers; its next turn slips back.")); }
		if (Deg == 3) Foe().OffGuardTurns = FMath::Max(Foe().OffGuardTurns, 1);
		break;
	}
	case ECardFx::ForceBarrage:
		for (int32 i = 0; i < 2; ++i)
		{
			const int32 Bolt = D(4) + 1;
			Beat(i == 0 ? 0.15f : 0.5f, [this, Bolt] { if (Combat.IsSet() && !Combat->bOver) { const int32 Tg = Combat->Target; if (Tg != INDEX_NONE) { CLog(FString::Printf(TEXT("Force bolt hits for %d."), Bolt)); HurtFoe(Tg, Bolt, false); } MarkUI(); } });
		}
		Lunge(0, T);
		break;
	case ECardFx::ElectricArc:
		for (int32 i = 1; i < C.F.Num(); ++i)
		{
			FFighter& F = C.F[i];
			if (F.bDead) continue;
			const int32 Nat = D(20);
			const int32 Deg = Degree(Nat + F.Ref, SpellDC(PC), Nat);
			int32 Dmg = R(TEXT("2d4")) + CastMod(PC);
			Dmg = Deg == 3 ? 0 : Deg == 2 ? Dmg / 2 : Deg == 0 ? Dmg * 2 : Dmg;
			CLog(FString::Printf(TEXT("%s Reflex save %d vs DC %d: %s."), *F.Name, Nat + F.Ref, SpellDC(PC), DegreeName(Deg)));
			if (Dmg > 0) HurtFoe(i, Dmg, Deg == 0);
			if (!Combat.IsSet() || Combat->bOver) break;
		}
		break;
	case ECardFx::Fear:
	{
		const int32 Nat = D(20);
		const int32 Deg = Degree(Nat + Foe().Will, SpellDC(PC), Nat);
		const int32 Fr = Deg == 3 ? 0 : Deg == 2 ? 1 : Deg == 1 ? 2 : 3;
		CLog(FString::Printf(TEXT("%s Will save %d vs DC %d: %s."), *Foe().Name, Nat + Foe().Will, SpellDC(PC), DegreeName(Deg)));
		if (Fr) { Foe().Frightened = FMath::Max(Foe().Frightened, Fr); FloatText(T, FString::Printf(TEXT("frightened %d"), Fr), FColor(200, 160, 255)); if (AActor* Pp = Foe().Puppet.Get()) HollowArt::NiagaraPart(Pp, TEXT("/Game/NiagaraExamples/FX_Player/NS_Player_DeBuff_Looping"), FVector::ZeroVector, 1.f); }
		break;
	}
	case ECardFx::ShieldSpell: C.F[0].GuardAc += 1; C.F[0].ShieldHp = 5; CLog(TEXT("A shimmering shield: +1 AC, absorbs 5.")); break;
	case ECardFx::Heal: HealHero(R(TEXT("1d8+8")), TEXT("Heal")); break;
	case ECardFx::DivineLance:
	case ECardFx::HolyFlame:
	{
		const bool bFlame = Card->Fx == ECardFx::HolyFlame;
		const int32 Nat = RollD20();
		const int32 Mod = SpellAtk(PC) + (C.BlessTurns > 0 ? 1 : 0) - C.F[0].Frightened;
		const int32 Dc = Foe().Ac - Foe().Frightened - (Foe().OffGuardTurns > 0 ? 2 : 0);
		const int32 Deg = Degree(Nat + Mod, Dc, Nat);
		CLog(FString::Printf(TEXT("%s: d20 %d %s = %d vs AC %d. %s."), *Card->Name, Nat, *Sgn(Mod), Nat + Mod, Dc, DegreeName(Deg)));
		Lunge(0, T);
		if (bFlame && Foe().bUndead) Foe().bRegenBlocked = true;
		if (Deg >= 2)
		{
			int32 Dmg = bFlame ? R(TEXT("3d6")) : R(TEXT("2d4")) + CastMod(PC) + (Foe().bUndead ? D(4) : 0);
			Dmg += C.HymnTurns > 0 ? 2 : 0;
			if (Deg == 3) Dmg *= 2;
			HurtFoe(T, Dmg, Deg == 3);
		}
		else FloatText(T, TEXT("miss"), FColor(220, 220, 220));
		break;
	}
	case ECardFx::Bless: if (AActor* Pp = C.F[0].Puppet.Get()) HollowArt::NiagaraPart(Pp, TEXT("/Game/NiagaraExamples/FX_Player/NS_Player_Buff_Looping"), FVector::ZeroVector, 1.f); C.BlessTurns = 3; CLog(TEXT("Blessed: +1 to attack rolls for 3 turns.")); break;
	case ECardFx::BattleHymn: if (AActor* Pp = C.F[0].Puppet.Get()) HollowArt::NiagaraPart(Pp, TEXT("/Game/NiagaraExamples/FX_Player/NS_Player_Buff_Looping"), FVector::ZeroVector, 1.f); C.HymnTurns = 3; CLog(TEXT("Battle Hymn: +2 damage for 3 turns.")); break;
	case ECardFx::Quicken: C.bQuicken = true; CLog(TEXT("Quickened: your next turn comes much sooner.")); break;
	case ECardFx::LuckyCoin: C.bLucky = true; CLog(TEXT("You flip the coin. Fortune favours your next roll.")); break;
	case ECardFx::SecondWind: HealHero(D(8) + 2 * PC.Level, TEXT("Second Wind")); break;
	case ECardFx::Insight: DrawCards(2); CLog(TEXT("You read the fight and draw two cards.")); break;
	case ECardFx::Riposte: C.bRiposte = true; CLog(TEXT("Riposte ready: misses against you will be punished.")); break;
	case ECardFx::LibSpell: CastLibSpell(*Card, T); break;
	}
	if (Combat.IsSet() && !Combat->bOver && Combat->Actions == 0)
	{
		bool bFree = false;
		for (const FString& H : Combat->Hand) if (const FCardDef* Cd = HollowData::Cards().Find(H)) if (Cd->Cost == 0) bFree = true;
		if (!bFree) Beat(1.1f, [this] { CombatEndTurn(); });
	}
	MarkUI();
}

/* ---------- Foes ---------- */

void AHollowGameMode::ChooseIntent(FFighter& F)
{
	const FString Dm = F.Dmg;
	F.IntentStep++;
	if (F.Special == TEXT("knockdown")) { F.Intent = TEXT("knockdown"); F.IntentText = FString::Printf(TEXT("Bite x2 (%s). A hit knocks you off-guard."), *Dm); }
	else if (F.Special == TEXT("dirty")) { if (F.IntentStep % 2) { F.Intent = TEXT("dirty"); F.IntentText = FString::Printf(TEXT("Dirty trick, then stab (%s)."), *Dm); } else { F.Intent = TEXT("strike2"); F.IntentText = FString::Printf(TEXT("Stab x2 (%s)."), *Dm); } }
	else if (F.Special == TEXT("paralyze")) { F.Intent = TEXT("paralyze"); F.IntentText = FString::Printf(TEXT("Claws x2 (%s). Fort DC 15 or slowed."), *Dm); }
	else if (F.Special == TEXT("grab")) { if (F.IntentStep % 2) { F.Intent = TEXT("grab"); F.IntentText = FString::Printf(TEXT("Drag under (%s), Reflex DC 17."), *Dm); } else { F.Intent = TEXT("strike2"); F.IntentText = FString::Printf(TEXT("Grasp x2 (%s)."), *Dm); } }
	else if (F.Special == TEXT("lord"))
	{
		const int32 S = F.IntentStep % 3;
		if (S == 1) { F.Intent = TEXT("strike2"); F.IntentText = FString::Printf(TEXT("Cold talons x2 (%s)."), *Dm); }
		else if (S == 2) { F.Intent = TEXT("dread"); F.IntentText = TEXT("Dread gaze (Will DC 18), then talons."); }
		else { F.Intent = TEXT("drain"); F.IntentText = TEXT("Drain life: heals by the damage dealt."); }
	}
	else if (F.Hp * 10 < F.MaxHp * 4 && FMath::FRand() < 0.35f) { F.Intent = TEXT("defend"); F.IntentText = FString::Printf(TEXT("Defend (+2 AC) and strike (%s)."), *Dm); }
	else { F.Intent = TEXT("strike2"); F.IntentText = FString::Printf(TEXT("%s x2 (%s)."), *F.Weapon, *Dm); }
}

void AHollowGameMode::StartFoeTurn(int32 Idx)
{
	FCombatState& C = Combat.GetValue();
	C.bBusy = true;
	FFighter& F = C.F[Idx];
	F.GuardAc = 0;
	if (F.Regen > 0)
	{
		if (F.bRegenBlocked) CLog(FString::Printf(TEXT("%s's wounds smoke and refuse to close."), *F.Name));
		else if (F.Hp < F.MaxHp) { F.Hp = FMath::Min(F.MaxHp, F.Hp + F.Regen); CLog(FString::Printf(TEXT("%s regenerates %d."), *F.Name, F.Regen)); FloatText(Idx, FString::Printf(TEXT("+%d"), F.Regen), FColor(180, 120, 255)); }
	}
	F.bRegenBlocked = false;
	CLog(FString::Printf(TEXT("%s: %s"), *F.Name, *F.IntentText));
	const FString Act = F.Intent;
	TArray<FString> Steps;
	if (Act == TEXT("dirty")) Steps = { TEXT("trick"), TEXT("strike") };
	else if (Act == TEXT("grab")) Steps = { TEXT("grabstrike") };
	else if (Act == TEXT("dread")) Steps = { TEXT("gaze"), TEXT("strike") };
	else if (Act == TEXT("drain")) Steps = { TEXT("drain") };
	else if (Act == TEXT("defend")) Steps = { TEXT("defend"), TEXT("strike") };
	else Steps = { TEXT("strike"), TEXT("strike") };
	if (!InReach(Idx, 0, F.Reach)) { Steps.Insert(TEXT("close"), 0); if (Steps.Num() > 2) Steps.SetNum(2); }
	for (int32 s = 0; s < Steps.Num(); ++s)
	{
		const FString Step = Steps[s];
		Beat(s == 0 ? 1.1f : 1.25f, [this, Idx, Step, s] { if (Combat.IsSet() && !Combat->bOver && !Combat->F[Idx].bDead) FoeAction(Idx, Step, s); });
	}
	Beat(0.95f, [this, Idx]
	{
		if (!Combat.IsSet() || Combat->bOver) return;
		if (!Combat->F[Idx].bDead) ChooseIntent(Combat->F[Idx]);
		EndActorTurn(Idx);
	});
	MarkUI();
}

void AHollowGameMode::FoeAction(int32 Idx, const FString& Act, int32 Step)
{
	FCombatState& C = Combat.GetValue();
	FFighter& F = C.F[Idx];
	FFighter& H = C.F[0];
	if (Act == TEXT("trick"))
	{
		if (HeroSave(TEXT("ref"), F.Atk + 10, TEXT("Dirty trick")) < 2) { H.OffGuardTurns = 1; FloatText(0, TEXT("off-guard"), FColor(255, 220, 120)); }
		return;
	}
	if (Act == TEXT("gaze"))
	{
		const int32 Deg = HeroSave(TEXT("will"), 18, TEXT("Dread gaze"));
		if (Deg < 2) { H.Frightened = FMath::Max(H.Frightened, Deg == 0 ? 2 : 1); FloatText(0, TEXT("frightened"), FColor(200, 160, 255)); }
		return;
	}
	if (Act == TEXT("defend")) { F.GuardAc = 2; CLog(FString::Printf(TEXT("%s braces itself."), *F.Name)); return; }
	if (Act == TEXT("close") || !InReach(Idx, 0, F.Reach)) { FoeClose(Idx); return; }

	// a strike of some flavour
	const int32 Penalty = Step == 0 ? 0 : -5;
	const int32 Nat = D(20);
	const int32 Mod = F.Atk - F.Frightened + Penalty;
	const int32 Dc = HeroAC();
	const int32 Deg = Degree(Nat + Mod, Dc, Nat);
	Lunge(Idx, 0);
	CLog(FString::Printf(TEXT("%s %s: d20 %d %s = %d vs your AC %d. %s."), *F.Name, *F.Weapon, Nat, *Sgn(Mod), Nat + Mod, Dc, DegreeName(Deg)));
	if (Deg >= 2)
	{
		int32 Dmg = R(F.Dmg);
		if (Deg == 3) Dmg *= 2;
		HurtHero(Dmg, Deg == 3, Idx);
		if (!Combat.IsSet() || Combat->bOver) return;
		if (F.Intent == TEXT("knockdown")) { C.F[0].OffGuardTurns = 1; CLog(TEXT("You're knocked off your feet: off-guard.")); }
		if (F.Intent == TEXT("paralyze") && HeroSave(TEXT("fort"), 15, TEXT("Paralysis")) < 2) { C.F[0].Slowed = 1; FloatText(0, TEXT("slowed"), FColor(160, 220, 160)); }
		if (Act == TEXT("grabstrike") && HeroSave(TEXT("ref"), 17, TEXT("Drag under")) < 2) { HurtHero(D(6), false, Idx); if (Combat.IsSet() && !Combat->bOver) C.F[0].OffGuardTurns = 1; }
		if (Act == TEXT("drain")) { const int32 Gain = FMath::Min(Dmg, F.MaxHp - F.Hp); F.Hp += Gain; if (Gain) CLog(FString::Printf(TEXT("%s drinks %d of your life."), *F.Name, Gain)); }
	}
	else
	{
		FloatText(0, TEXT("miss"), FColor(220, 220, 220));
		if (C.bRiposte)
		{
			const int32 Dmg = D(PC.WeaponDie) + DmgBonus(PC);
			CLog(FString::Printf(TEXT("Riposte! You punish the miss for %d."), Dmg));
			HurtFoe(Idx, FMath::Max(1, Dmg), false);
		}
	}
	MarkUI();
}

/* ---------- Ending ---------- */

void AHollowGameMode::CheckCombatEnd()
{
	if (!Combat.IsSet() || Combat->bOver) return;
	FCombatState& C = Combat.GetValue();
	if (PC.Hp <= 0)
	{
		C.bOver = true; C.bWon = false;
		CLog(TEXT("You fall..."));
		PlaySfx(TEXT("defeat"), 0.8f);
		Beats.Reset();
		Beat(1.4f, [this] { FinishCombat(); });
		return;
	}
	if (FirstLivingFoe() == INDEX_NONE)
	{
		C.bOver = true; C.bWon = true;
		for (int32 i = 1; i < C.F.Num(); ++i)
			if (const FFoeDef* D = HollowData::Foes().Find(C.F[i].FoeId)) { C.XpGain += D->Xp; C.GoldGain += D->Gold; }
		CLog(FString::Printf(TEXT("Victory! +%d XP, +%d silver."), C.XpGain, C.GoldGain));
		PlaySfx(TEXT("victory"), 0.8f);
		Beats.Reset();
		C.bBusy = true;
		Beat(1.2f, [this] { if (Combat.IsSet()) { Combat->bBusy = false; MarkUI(); } });
	}
	MarkUI();
}

void AHollowGameMode::FinishCombat()
{
	if (!Combat.IsSet()) return;
	const FCombatState C = Combat.GetValue();
	Combat.Reset();
	Beats.Reset();
	for (AActor* A : ArenaActors) if (IsValid(A)) A->Destroy();
	ArenaActors.Reset();
	if (PC.Hp <= 0) { Die(TEXT("The fight went the wrong way.")); return; }
	Mode = EHollowMode::Explore;
	SetViewTo(Hero, 0.8f);
	if (const FHollowHex* Here = Hexes.Find(Pos)) PlayMusic(Here->T == ETerr::Town ? TEXT("dark_cavern_ambient_002") : TEXT("horror_atmosphere_loop"), Here->T == ETerr::Town ? 0.45f : 0.35f);
	// fight-limited buffs tick down; the "until the day ends" ones (Fights == 0) wait for the next day
	for (FBuff& B : Buffs) if (B.Fights > 0 && --B.Fights == 0) B.Fights = -1;
	Buffs.RemoveAll([](const FBuff& B) { return B.Fights < 0; });
	RefreshAura();
	if (C.bWon)
	{
		bool bBoss = false;
		int32 Pelts = 0;
		for (const FFighter& F : C.F) if (F.FoeId == TEXT("wolf") || F.FoeId == TEXT("alpha")) Pelts++;
		if (Pelts && Give(TEXT("pelt"), Pelts)) Log(FString::Printf(TEXT("Skinned %d wolf pelt%s."), Pelts, Pelts > 1 ? TEXT("s") : TEXT("")));
		if (FMath::FRand() < 0.2f) Give(TEXT("trinket"), 1, true);
		for (const FFighter& F : C.F) if (F.bBoss) bBoss = true;
		PC.Gold += C.GoldGain;
		GainXp(C.XpGain);
		Log(FString::Printf(TEXT("Won a fight. +%d XP, +%d silver."), C.XpGain, C.GoldGain));
		if (C.bFromEvent) ResolveEventDone();
		if (bBoss) { ResolveEventDone(); Win(); return; }
		FRewardView Rv;
		Rv.Title = TEXT("Spoils of battle: add a card to your deck");
		const TArray<FString>& Pool = HollowData::RewardPool(PC.Kit);
		while (Rv.Options.Num() < 3 && Pool.Num()) Rv.Options.AddUnique(Pool[FMath::RandRange(0, Pool.Num() - 1)]);
		Reward = Rv;
	}
	else if (C.bFled) Log(TEXT("Fled from a fight."));
	MarkUI();
}

/* ---------- Puppet animation ---------- */

void AHollowGameMode::Lunge(int32 Idx, int32 TargetIdx)
{
	if (!Combat.IsSet() || !Combat->F.IsValidIndex(Idx) || !Combat->F.IsValidIndex(TargetIdx)) return;
	AActor* P = Combat->F[Idx].Puppet.Get();
	if (!P) return;
	const FVector From = Combat->F[Idx].Home;
	const FVector Dir = (Combat->F[TargetIdx].Home - From).GetSafeNormal2D();
	FAnim A; A.A = P; A.From = From; A.To = From + Dir * 240.f; A.Dur = 0.6f; A.Kind = 0;
	Anims.Add(A);
}

void AHollowGameMode::Shake(int32 Idx)
{
	if (!Combat.IsSet() || !Combat->F.IsValidIndex(Idx)) return;
	AActor* P = Combat->F[Idx].Puppet.Get();
	if (!P) return;
	FAnim A; A.A = P; A.From = Combat->F[Idx].Home; A.Dur = 0.4f; A.Kind = 1;
	Anims.Add(A);
}

void AHollowGameMode::FloatText(int32 Idx, const FString& Text, const FColor& Col)
{
	if (!Combat.IsSet() || !Combat->F.IsValidIndex(Idx)) return;
	const FVector At = Combat->F[Idx].Home + FVector(FMath::FRandRange(-30.f, 30.f), 60.f, Combat->F[Idx].bBoss ? 330.f : 230.f);
	AActor* A = HollowArt::NewActor(GetWorld(), At, 0.f);
	UTextRenderComponent* T = NewObject<UTextRenderComponent>(A);
	T->SetupAttachment(A->GetRootComponent());
	T->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	T->SetHorizontalAlignment(EHTA_Center);
	T->SetWorldSize(Text.Len() > 4 ? 46.f : 80.f);
	T->SetTextRenderColor(Col);
	T->SetText(FText::FromString(Text));
	T->RegisterComponent();
	ArenaActors.Add(A);
	FAnim Fa; Fa.A = A; Fa.From = At; Fa.To = At + FVector(0, 0, 140.f); Fa.Dur = 1.7f; Fa.Kind = 2;
	Anims.Add(Fa);
}

/* ---------- Library spells ---------- */

// Any pf2e spell from the rules library, resolved from its data: a basic save halves or doubles, a spell attack
// rolls against AC, no defense means it simply lands. Areas hit every foe. Non-damaging saves leave a condition
// picked from the spell's traits. The effect family (fire, lightning, impact, curse, blessing) follows damage type.
void AHollowGameMode::CastLibSpell(const FCardDef& Card, int32 T)
{
	const FLibSpell* S = HollowLib::Spell(Card.Spell);
	if (!S || !Combat.IsSet()) { CLog(TEXT("The words come out wrong.")); return; }
	FCombatState& C = Combat.GetValue();
	const FString Ty = S->Damage.Num() ? S->Damage[0].Value : FString();

	auto Fx = [this, Ty, S](int32 Idx)
	{
		const FVector At = FighterPos(Idx);
		if (Ty == TEXT("fire")) { NiagaraAt(TEXT("/Game/NiagaraExamples/FX_Misc/NS_Fire"), At - FVector(0, 0, 80), 0.6f); Burst(At, TEXT("fire_01"), FLinearColor(1.f, 0.45f, 0.15f), 18, 320.f, 0.8f, 60.f, 160.f); }
		else if (Ty == TEXT("electricity")) { NiagaraAt(TEXT("/Game/NiagaraExamples/FX_Sparks/NS_Spark_Burst"), At, 1.f); Burst(At, TEXT("spark_05"), FLinearColor(0.8f, 0.85f, 1.f), 16, 420.f, 0.5f, 40.f, 80.f); }
		else if (Ty == TEXT("cold")) Burst(At, TEXT("star_03"), FLinearColor(0.6f, 0.85f, 1.f), 20, 260.f, 1.f, 45.f, 60.f, 120.f);
		else if (Ty == TEXT("mental") || Ty == TEXT("void")) { NiagaraAt(TEXT("/Game/NiagaraExamples/FX_Player/NS_Player_DeBuff_Looping"), At - FVector(0, 0, 80), 1.f); Burst(At, TEXT("twirl_03"), FLinearColor(0.6f, 0.3f, 0.9f), 14, 200.f, 1.f, 70.f, 60.f); }
		else if (Ty == TEXT("vitality") || S->bHeal) { NiagaraAt(TEXT("/Game/NiagaraExamples/FX_Player/NS_Player_Buff_Looping"), At - FVector(0, 0, 80), 1.f); Burst(At, TEXT("light_02"), FLinearColor(1.f, 0.9f, 0.55f), 18, 220.f, 1.1f, 50.f, 220.f); }
		else { NiagaraAt(TEXT("/Game/NiagaraExamples/FX_Weapons/Impacts/NS_Impact_Metal"), At, 1.f); Burst(At, TEXT("magic_02"), FLinearColor(0.55f, 0.6f, 1.f), 16, 300.f, 0.8f, 55.f, 100.f); }
	};
	auto Roll = [&]() { int32 Dm = 0; for (int32 k = 0; k < S->Damage.Num(); ++k) Dm += R(HollowLib::HeightenedFormula(*S, Card.Rank, k)); return Dm; };

	// healing magic mends you (a 2-action Heal adds 8 per rank, as in the rules)
	if (S->bHeal || (!S->Damage.Num() && S->Traits.Contains(TEXT("healing"))))
	{
		int32 H = S->Damage.Num() ? Roll() : HollowRules::D(8) * FMath::Max(1, Card.Rank);
		if (S->Name == TEXT("Heal") && Card.Cost >= 2) H += 8 * FMath::Max(1, Card.Rank);
		Fx(0);
		HealHero(H, S->Name);
		return;
	}

	TArray<int32> Targets;
	if (!S->AreaType.IsEmpty()) { for (int32 i = 1; i < C.F.Num(); ++i) if (!C.F[i].bDead) Targets.Add(i); }
	else if (C.F.IsValidIndex(T) && !C.F[T].bDead) Targets.Add(T);
	if (!S->AreaType.IsEmpty()) CLog(FString::Printf(TEXT("%s fills a %d-foot %s."), *S->Name, S->AreaSize, *S->AreaType));
	Lunge(0, Targets.Num() ? Targets[0] : 1);

	for (int32 i : Targets)
	{
		if (!Combat.IsSet() || Combat->bOver) return;
		FFighter& F = Combat->F[i];
		if (F.bDead) continue;
		int32 Dmg = S->Damage.Num() ? Roll() : 0;
		bool bCrit = false;
		// vitality only harms the undead; void only harms the living
		if ((Ty == TEXT("vitality") && !F.bUndead) || (Ty == TEXT("void") && F.bUndead)) { CLog(FString::Printf(TEXT("%s shrugs off the %s energy."), *F.Name, *Ty)); continue; }
		if (!S->Save.IsEmpty())
		{
			const int32 Mod = S->Save == TEXT("fortitude") ? F.Fort : S->Save == TEXT("reflex") ? F.Ref : F.Will;
			const int32 Nat = HollowRules::D(20);
			const int32 Deg = Degree(Nat + Mod, SpellDC(PC), Nat);
			CLog(FString::Printf(TEXT("%s %s save %d vs DC %d: %s."), *F.Name, *SkillName(S->Save), Nat + Mod, SpellDC(PC), DegreeName(Deg)));
			if (Dmg > 0) Dmg = Deg == 3 ? 0 : Deg == 2 ? (S->bBasic ? Dmg / 2 : 0) : Deg == 0 ? Dmg * 2 : Dmg;
			bCrit = Deg == 0;
			if (!S->Damage.Num() && Deg <= 1)
			{
				// a condition by the spell's nature
				if (S->Traits.Contains(TEXT("fear")) || S->Traits.Contains(TEXT("emotion"))) { F.Frightened = FMath::Max(F.Frightened, Deg == 0 ? 2 : 1); FloatText(i, TEXT("frightened"), FColor(200, 160, 255)); }
				else if (S->Name.Contains(TEXT("Slow")) || S->Traits.Contains(TEXT("incapacitation"))) { F.Ctb += F.Delay() * (Deg == 0 ? 1.f : 0.5f); FloatText(i, TEXT("slowed"), FColor(160, 200, 255)); }
				else { F.OffGuardTurns = FMath::Max(F.OffGuardTurns, Deg == 0 ? 2 : 1); FloatText(i, TEXT("off-guard"), FColor(255, 220, 120)); }
			}
		}
		else if (S->Vs == TEXT("ac"))
		{
			const int32 Nat = RollD20();
			const int32 Mod = SpellAtk(PC) + (C.BlessTurns > 0 ? 1 : 0) - C.F[0].Frightened;
			const int32 Dc = F.Ac - F.Frightened - (F.OffGuardTurns > 0 ? 2 : 0);
			const int32 Deg = Degree(Nat + Mod, Dc, Nat);
			CLog(FString::Printf(TEXT("%s: spell attack %d %s = %d vs AC %d. %s."), *S->Name, Nat, *Sgn(Mod), Nat + Mod, Dc, DegreeName(Deg)));
			Dmg = Deg == 3 ? Dmg * 2 : Deg == 2 ? Dmg : 0;
			bCrit = Deg == 3;
		}
		if (Dmg > 0) { Fx(i); HurtFoe(i, Dmg + (C.HymnTurns > 0 ? 2 : 0), bCrit); }
		else if (S->Damage.Num()) FloatText(i, TEXT("resisted"), FColor(220, 220, 220));
		else Fx(i);
	}

	// no damage and no save: a buff on yourself
	if (!S->Damage.Num() && S->Save.IsEmpty() && Combat.IsSet())
	{
		Fx(0);
		if (S->Name.Contains(TEXT("Shield"))) { Combat->F[0].GuardAc += 1; Combat->F[0].ShieldHp = 5 * FMath::Max(1, Card.Rank); CLog(TEXT("A shield of force: +1 AC and it soaks the next hit.")); }
		else if (S->Traits.Contains(TEXT("fortune")) || S->Name.Contains(TEXT("Guidance"))) { Combat->bLucky = true; CLog(TEXT("Fortune favours your next roll.")); }
		else if (S->Traits.Contains(TEXT("morale")) || S->Name.Contains(TEXT("Bless"))) { Combat->BlessTurns = 3; CLog(TEXT("+1 to attack rolls for 3 turns.")); }
		else if (S->Traits.Contains(TEXT("illusion"))) { Combat->F[0].GuardAc += 2; CLog(TEXT("Your outline blurs: +2 AC until your next turn.")); }
		else CLog(FString::Printf(TEXT("%s takes hold."), *S->Name));
	}
}


// A foe out of reach strides toward the nearest open hex next to you (one action, up to its Speed).
void AHollowGameMode::FoeClose(int32 Idx)
{
	if (!Combat.IsSet()) return;
	const FFighter& F = Combat->F[Idx];
	const int32 Steps = FMath::Max(1, F.Speed / 5);
	TArray<FIntPoint> Best;
	int32 BestLeft = 999;
	for (int32 d = 0; d < 6; ++d)
	{
		static const FIntPoint Ds[6] = { { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, 0 }, { -1, 1 }, { 0, 1 } };
		const FIntPoint Goal = Combat->F[0].Cell + Ds[d];
		if (!CellFree(Goal, Idx)) continue;
		const TArray<FIntPoint> P = PathTo(Idx, Goal, 40);
		if (P.Num() && P.Num() < BestLeft) { Best = P; BestLeft = P.Num(); }
	}
	if (!Best.Num()) { CLog(FString::Printf(TEXT("%s can't find a way to you."), *F.Name)); return; }
	if (Best.Num() > Steps) Best.SetNum(Steps);
	MoveFighter(Idx, Best);
	CLog(FString::Printf(TEXT("%s closes %d feet."), *F.Name, Best.Num() * 5));
	RefreshGridHighlight();
}
