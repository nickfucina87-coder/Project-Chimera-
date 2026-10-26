#include "HollowGameMode.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Widgets/SWeakWidget.h"
#include "HollowArt.h"
#include "HollowHero.h"
#include "GameFramework/SpringArmComponent.h"
#include "HollowPlayerController.h"
#include "HollowRules.h"
#include "HollowUI.h"

using namespace HollowRules;
using HollowArt::EShape;

const FVector AHollowGameMode::LocalOrigin(0.f, 0.f, 0.f);
const FVector AHollowGameMode::BoardOrigin(80000.f, 0.f, 0.f);
const FVector AHollowGameMode::ArenaOrigin(0.f, 80000.f, 0.f);

AHollowGameMode::AHollowGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;   // the pause screen keeps the UI alive
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AHollowPlayerController::StaticClass();
}

void AHollowGameMode::BeginPlay()
{
	Super::BeginPlay();
	ToonMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Hollow/Materials/M_HollowLit.M_HollowLit"));
	if (!ToonMat) ToonMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Hollow/Materials/M_HollowToon.M_HollowToon"));
	GrimMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Hollow/Materials/M_KenneyGrim.M_KenneyGrim"));
	SpriteMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Hollow/Materials/M_HollowSprite.M_HollowSprite"));
	InkMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Hollow/Materials/PP_HollowInk.PP_HollowInk"));
	HollowArt::Init(ToonMat, GrimMat);

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (APostProcessVolume* PPV = GetWorld()->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FTransform::Identity, SP))
	{
		PPV->bUnbound = true;
		FPostProcessSettings& S = PPV->Settings;
		S.bOverride_AutoExposureMethod = true; S.AutoExposureMethod = AEM_Histogram;
		S.bOverride_AutoExposureMinBrightness = true; S.AutoExposureMinBrightness = -4.f;
		S.bOverride_AutoExposureMaxBrightness = true; S.AutoExposureMaxBrightness = 1.f;
		S.bOverride_AutoExposureBias = true; S.AutoExposureBias = -1.3f;
		S.bOverride_AutoExposureSpeedUp = true; S.AutoExposureSpeedUp = 2.f;
		S.bOverride_AutoExposureSpeedDown = true; S.AutoExposureSpeedDown = 1.f;
		S.bOverride_BloomIntensity = true; S.BloomIntensity = 0.f;
		S.bOverride_VignetteIntensity = true; S.VignetteIntensity = 0.f;
		S.bOverride_MotionBlurAmount = true; S.MotionBlurAmount = 0.f;
		S.bOverride_SceneFringeIntensity = true; S.SceneFringeIntensity = 0.6f;
		S.bOverride_FilmGrainIntensity = true; S.FilmGrainIntensity = 0.18f;
		if (InkMat) S.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, InkMat));
	}

	ShowCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(),
		FTransform(FRotator(-34.f, 12.f, 0.f), LocalOrigin + FVector(-2900.f, -1100.f, 2100.f)), SP);
	MapCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(),
		FTransform(FRotator(-60.f, 0.f, 0.f), BoardOrigin + FVector(-3300.f, 0.f, 5800.f)), SP);
	ArenaCam = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(),
		FTransform(FRotator(-17.f, -90.f, 0.f), ArenaOrigin + FVector(0.f, 1250.f, 480.f)), SP);
	for (ACameraActor* C : { ShowCam.Get(), MapCam.Get(), ArenaCam.Get() })
	{
		C->GetCameraComponent()->bConstrainAspectRatio = false;
		C->GetCameraComponent()->SetFieldOfView(60.f);
	}

	BuildAtmosphere();
	GenMap();
	BuildLocal(Pos);
	BuildBoard();
	ProbeBrain(true);
	PlayMusic(TEXT("dark_cavern_ambient_002"), 0.45f);
}

void AHollowGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UIHost.IsValid() && GEngine && GEngine->GameViewport)
		GEngine->GameViewport->RemoveViewportWidgetContent(UIHost.ToSharedRef());
	UIHost.Reset();
	UI.Reset();
	Super::EndPlay(Reason);
}

void AHollowGameMode::Tick(float Dt)
{
	Super::Tick(Dt);

	if (!bStarted)
	{
		APlayerController* P = GetWorld()->GetFirstPlayerController();
		if (P && GEngine && GEngine->GameViewport)
		{
			bStarted = true;
			P->SetViewTarget(ShowCam);
			UI = SNew(SHollowUI).GM(this);
			UIHost = SNew(SWeakWidget).PossiblyNullContent(UI.ToSharedRef());
			GEngine->GameViewport->AddViewportWidgetContent(UIHost.ToSharedRef(), 10);
			bUIDirty = true;
		}
	}

	if (bPaused)
	{
		if (bUIDirty && UI.IsValid()) { bUIDirty = false; UI->Rebuild(); }
		return;
	}
	if (Day != BuffDay)
	{
		BuffDay = Day;
		const int32 Before = Buffs.Num();
		Buffs.RemoveAll([](const FBuff& B) { return B.Fights == 0; });
		if (Buffs.Num() != Before) { RefreshAura(); MarkUI(); }
	}

	if (Beats.Num())
	{
		BeatClock -= Dt;
		while (BeatClock <= 0.f && Beats.Num())
		{
			FBeat B = MoveTemp(Beats[0]);
			Beats.RemoveAt(0);
			B.Fn();
			BeatClock = Beats.Num() ? Beats[0].Delay : 0.f;
		}
	}

	for (int32 i = Anims.Num() - 1; i >= 0; --i)
	{
		FAnim& A = Anims[i];
		A.T += Dt;
		const float U = FMath::Clamp(A.T / A.Dur, 0.f, 1.f);
		if (AActor* Act = A.A.Get())
		{
			switch (A.Kind)
			{
			case 0: Act->SetActorLocation(FMath::Lerp(A.From, A.To, FMath::Sin(U * PI))); break;           // lunge out and back
			case 1: Act->SetActorLocation(A.From + FVector(0, FMath::Sin(U * PI * 8) * 18.f * (1 - U), 0)); break; // shake
			case 2: Act->SetActorLocation(FMath::Lerp(A.From, A.To, U)); break;                                // float up
			case 3: Act->SetActorScale3D(FVector(FMath::Max(0.01f, 1.f - U), FMath::Max(0.01f, 1.f - U), FMath::Max(0.01f, 1.f - U * 0.9f))); break;
			}
		}
		if (U >= 1.f)
		{
			if (AActor* Act = A.A.Get())
			{
				if (A.Kind == 2) Act->Destroy();
				else if (A.Kind == 3) Act->SetActorHiddenInGame(true);
				else Act->SetActorLocation(A.From);
			}
			Anims.RemoveAt(i);
		}
	}

	UpdatePrompt();
	TickVisibility(Dt);
	TickFx(Dt);
	TickAmbience(Dt);
	TickFow(Dt);
	TickTitle(Dt);

	// buff aura: three motes circling the hero, bobbing out of step
	if (AuraRing && AuraRing->IsVisible())
	{
		AuraClock += Dt;
		for (int32 i = 0; i < AuraOrbs.Num(); ++i)
			if (UStaticMeshComponent* O = AuraOrbs[i])
			{
				const float A = AuraClock * 1.6f + i * 2.094f;
				O->SetRelativeLocation(FVector(FMath::Cos(A) * 70.f, FMath::Sin(A) * 70.f, -20.f + FMath::Sin(AuraClock * 2.3f + i) * 30.f));
			}
		const float Pulse = 1.f + FMath::Sin(AuraClock * 3.f) * 0.06f;
		AuraRing->SetRelativeScale3D(FVector(1.5f * Pulse, 1.5f * Pulse, 0.02f));
	}

	// Hide whatever stands between the top-down camera and the hero (houses, crypts, trees), restore it when clear.
	{
		static TSet<TWeakObjectPtr<AActor>> Occluders;
		TSet<TWeakObjectPtr<AActor>> Blocking;
		APlayerController* P = GetWorld()->GetFirstPlayerController();
		if (Mode == EHollowMode::Explore && Hero && P && P->PlayerCameraManager)
		{
			const FVector Cam = P->PlayerCameraManager->GetCameraLocation();
			const FVector Target = Hero->GetActorLocation() + FVector(0, 0, 40.f);
			TArray<FHitResult> Hits;
			FCollisionQueryParams Q(TEXT("HollowOcclusion"), false, Hero);
			GetWorld()->SweepMultiByChannel(Hits, Cam, Target, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(60.f), Q);
			for (const FHitResult& H : Hits)
				if (AActor* A = H.GetActor())
					if (LocalActors.Contains(A) && (H.ImpactPoint - Target).Size() > 120.f) Blocking.Add(A);
		}
		for (const TWeakObjectPtr<AActor>& A : Occluders) if (A.IsValid() && !Blocking.Contains(A)) A->SetActorHiddenInGame(false);
		for (const TWeakObjectPtr<AActor>& A : Blocking) if (A.IsValid()) A->SetActorHiddenInGame(true);
		Occluders = MoveTemp(Blocking);
	}

	if (bUIDirty && UI.IsValid())
	{
		bUIDirty = false;
		UI->Rebuild();
	}
}

bool AHollowGameMode::CanWalk() const
{
	return Mode == EHollowMode::Explore && !Event.IsSet() && !Note.IsSet() && !Talk.IsSet() && !Reward.IsSet() && !Trade.IsSet() && !LevelUp.IsSet() && !bPaused;
}

void AHollowGameMode::Toast(const FString& S)
{
	ToastText = S;
	ToastUntil = GetWorld()->GetTimeSeconds() + 4.0;
}

void AHollowGameMode::Log(const FString& S)
{
	Journal.Insert(FString::Printf(TEXT("Day %d  %s"), Day, *S), 0);
	if (Journal.Num() > 40) Journal.SetNum(40);
	MarkUI();
}

void AHollowGameMode::SetViewTo(AActor* Target, float Blend)
{
	if (APlayerController* P = GetWorld()->GetFirstPlayerController())
		P->SetViewTargetWithBlend(Target, Blend, VTBlend_Cubic);
}

void AHollowGameMode::GainXp(int32 Xp)
{
	if (Xp <= 0) return;
	PC.Xp += Xp;
	while (PC.Level < 20 && PC.Xp >= XpForLevel(PC.Level + 1))
	{
		PC.Level++;
		PC.HpMax += PC.HpPer;
		PC.Hp += PC.HpPer;
		Toast(FString::Printf(TEXT("Level up! %s is now level %d."), *PC.Name, PC.Level));
		PlaySfx(TEXT("levelup"), 0.8f);
		Log(FString::Printf(TEXT("Reached level %d."), PC.Level));
		OnLevelUp();
	}
}

/* ---------- Flow ---------- */

void AHollowGameMode::BeginAdventure()
{
	if (Mode != EHollowMode::Create) return;
	if (!bImported) PC = BuildPC(CreateName.TrimStartAndEnd(), CreateAnc, CreateKit);
	Mode = EHollowMode::Explore;
	SpawnHero(LocalOrigin + FVector(-1700.f, 0.f, 150.f));
	Log(FString::Printf(TEXT("%s the %s %s arrives in Brindlemark."), *PC.Name, *PC.Ancestry, *PC.ClassName));
	Toast(TEXT("Talk to the townsfolk. What you ask about opens roads on the map."));
	MarkUI();
}

void AHollowGameMode::Restart()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void AHollowGameMode::SpawnHero(const FVector& At)
{
	APlayerController* P = GetWorld()->GetFirstPlayerController();
	if (!Hero)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Hero = GetWorld()->SpawnActor<AHollowHero>(AHollowHero::StaticClass(), FTransform(FRotator::ZeroRotator, At), SP);
		if (P) P->Possess(Hero);
	}
	Hero->SetActorLocation(At, false, nullptr, ETeleportType::ResetPhysics);
	Hero->SetActorRotation(FRotator::ZeroRotator);
	Hero->StopMove();
	Hero->ClampCenter = LocalOrigin;
	Hero->ClampRadius = LocalRadius * 0.84f;
	Hero->SetLook(PC);
	if (Hero->Boom && Hero->Boom->TargetArmLength < 2400.f) Hero->ZoomBy(500.f);
	SetViewTo(Hero, 0.6f);
	RefreshAura();
}

void AHollowGameMode::ClickWorld(const FHitResult& Hit)
{
	if (Mode == EHollowMode::Map)
	{
		if (const FIntPoint* H = TileOf.Find(Hit.GetComponent()))
		{
			if (MapSelected.IsSet() && MapSelected.GetValue() == *H) MapAction(*H);
			else SelectHex(*H);
		}
		return;
	}
	if (Mode == EHollowMode::Combat && Combat.IsSet())
	{
		for (int32 i = 1; i < Combat->F.Num(); ++i)
			if (Combat->F[i].Puppet.Get() == Hit.GetActor()) { SetTarget(i); return; }
		return;
	}
	if (!CanWalk() || !Hero) return;
	for (int32 i = 0; i < Interactables.Num(); ++i)
	{
		if (Interactables[i].Actor.Get() == Hit.GetActor() && Interactables[i].Actor.IsValid())
		{
			if (FVector::Dist2D(Hero->GetActorLocation(), Interactables[i].Loc) <= Interactables[i].Radius) { NearIdx = i; Interact(); }
			else Hero->MoveTo(Interactables[i].Loc);
			return;
		}
	}
	Hero->MoveTo(Hit.ImpactPoint);
}

void AHollowGameMode::UpdatePrompt()
{
	FString NewPrompt;
	NearIdx = INDEX_NONE;
	if (CanWalk() && Hero)
	{
		float Best = TNumericLimits<float>::Max();
		for (int32 i = 0; i < Interactables.Num(); ++i)
		{
			const FInteractable& I = Interactables[i];
			if (!I.Actor.IsValid()) continue;
			const float D = FVector::Dist2D(Hero->GetActorLocation(), I.Loc);
			if (D <= I.Radius && D < Best) { Best = D; NearIdx = i; }
		}
		if (NearIdx != INDEX_NONE) NewPrompt = FString::Printf(TEXT("[E]  %s"), Interactables[NearIdx].Kind == TEXT("feature") ? *FeatureLabel(Interactables[NearIdx]) : *Interactables[NearIdx].Label);
		// walking right into a scouted trail takes it, no key needed
		if (NearIdx != INDEX_NONE && Interactables[NearIdx].Kind == TEXT("edge") && Best < 150.f)
		{
			const FHollowHex* N = Hexes.Find(SideHex(Interactables[NearIdx].Index));
			if (N && N->IsOpen()) { const int32 Side = Interactables[NearIdx].Index; Prompt = FString(); EdgeTravel(Side); return; }
		}
	}
	Prompt = NewPrompt;
}

void AHollowGameMode::Interact()
{
	if (Note.IsSet()) { CloseNote(); return; }
	if (Talk.IsSet() && TalkRevealing()) { SkipReveal(); return; }
	if (Event.IsSet())
	{
		if (Event->Choice >= 0) CloseEvent();
		return;
	}
	if (!CanWalk() || NearIdx == INDEX_NONE || !Interactables.IsValidIndex(NearIdx)) return;
	const FInteractable I = Interactables[NearIdx];
	if (Hero) Hero->StopMove();
	if (I.Kind == TEXT("npc")) { OpenTalk(I.Id); PlaySfx(TEXT("creak"), 0.6f); }
	else if (I.Kind == TEXT("examine"))
	{
		if (const TPair<FString, FString>* L = HollowData::Lore().Find(I.Id)) { FNoteView Nv; Nv.Title = L->Key; Nv.Text = L->Value; Note = Nv; PlaySfx(TEXT("book"), 0.7f); }
	}
	else if (I.Kind == TEXT("exit")) ToggleMap();
	else if (I.Kind == TEXT("feature")) UseFeature(I);
	else if (I.Kind == TEXT("edge")) EdgeTravel(I.Index);
	else if (I.Kind == TEXT("event"))
	{
		if (FHollowHex* H = Hexes.Find(Pos))
		{
			ShowEvent(H->EventId);
			if (Event.IsSet()) Event->bHexEvent = true;
			PlaySfx(TEXT("book"), 0.7f);
		}
	}
	else if (I.Kind == TEXT("chest"))
	{
		FHollowHex& H = Hexes.FindChecked(Pos);
		if (H.ChestTaken.IsValidIndex(I.Index)) H.ChestTaken[I.Index] = true;
		const int32 G = D(10) + 4;
		PC.Gold += G;
		if (FMath::FRand() < 0.15f) { Give(TEXT("trinket"), 1); }
		FString Msg = FString::Printf(TEXT("You find %d silver"), G);
		if (FMath::FRand() < 0.3f) { Give(TEXT("potion"), 1); Msg += TEXT(" and a healing potion"); }
		else if (FMath::FRand() < 0.4f) { Give(TEXT("ration"), 2); Msg += TEXT(" and 2 rations"); }
		else if (FMath::FRand() < 0.3f) { Give(TEXT("torch"), 1); Msg += TEXT(" and a torch"); }
		Toast(Msg + TEXT("."));
		Log(Msg + TEXT("."));
		PlaySfx(TEXT("coins"));
		Burst(I.Loc + FVector(0, 0, 60), TEXT("star_04"), FLinearColor(1.f, 0.75f, 0.3f), 16, 260.f, 1.f, 40.f, 200.f, 400.f);
		if (AActor* A = I.Actor.Get()) A->Destroy();
		Interactables.RemoveAt(NearIdx);
		NearIdx = INDEX_NONE;
	}
	MarkUI();
}

void AHollowGameMode::ToggleMap()
{
	if (Mode == EHollowMode::Explore && CanWalk())
	{
		Mode = EHollowMode::Map;
		MapSelected.Reset();
		BuildBoard();
		SetViewTo(MapCam, 0.7f);
	}
	else if (Mode == EHollowMode::Map)
	{
		Mode = EHollowMode::Explore;
		SetViewTo(Hero, 0.7f);
	}
	MarkUI();
}

void AHollowGameMode::Escape()
{
	if (bPaused) { SetPaused(false); return; }
	if (LevelUp.IsSet() || Reward.IsSet() || (Event.IsSet() && Event->Choice < 0)) return;
	if (Note.IsSet()) CloseNote();
	else if (Trade.IsSet()) CloseTrade();
	else if (Talk.IsSet() && Talk->bLog) ToggleTalkLog();
	else if (Talk.IsSet() && Talk->bTyping) { Talk->bTyping = false; MarkUI(); }
	else if (Event.IsSet()) CloseEvent();
	else if (Talk.IsSet()) CloseTalk();
	else if (Mode == EHollowMode::Map) ToggleMap();
	else if (Mode == EHollowMode::Explore || Mode == EHollowMode::Combat) TogglePause(EPauseTab::System);
}

void AHollowGameMode::Die(const FString& Why)
{
	Mode = EHollowMode::End;
	bVictory = false;
	EndTitle = TEXT("Fallen in the March");
	EndText = FString::Printf(TEXT("%s On day %d, %s's lantern goes out. Brindlemark will need another hero."), *Why, Day, *PC.Name);
	Event.Reset(); Talk.Reset(); Reward.Reset();
	SetViewTo(ShowCam, 1.5f);
	MarkUI();
}

void AHollowGameMode::Win()
{
	Mode = EHollowMode::End;
	bVictory = true;
	EndTitle = TEXT("The Hollow Lord Falls");
	EndText = FString::Printf(TEXT("Varek's stolen years pour out of him like sand. On day %d the lanterns of Brindlemark burn a little brighter, and %s walks home a legend."), Day, *PC.Name);
	MarkUI();
}

/* ---------- Event cards ---------- */

void AHollowGameMode::ShowEvent(const FString& Id)
{
	if (!HollowData::FindEvent(Id)) return;
	FEventView V;
	V.EventId = Id;
	Event = V;
	MarkUI();
}

void AHollowGameMode::ChooseEventOption(int32 Index)
{
	if (!Event.IsSet() || Event->Choice >= 0) return;
	const FEventDef* E = HollowData::FindEvent(Event->EventId);
	if (!E || !E->Choices.IsValidIndex(Index)) return;
	const FChoice& C = E->Choices[Index];
	if (C.Cost > PC.Gold) { Toast(TEXT("You don't have enough silver.")); return; }
	FEventView& V = Event.GetValue();
	V.Choice = Index;
	if (C.Cost) { PC.Gold -= C.Cost; V.Chips.Add({ FString::Printf(TEXT("-%d silver"), C.Cost), false }); }
	FOutcome O;
	if (C.IsCheck())
	{
		const FString Skill = BestSkill(PC, C.Skills);
		const int32 Mod = SkillMod(PC, Skill);
		const int32 Nat = D(20);
		const int32 Total = Nat + Mod;
		const int32 Deg = Degree(Total, C.DC, Nat);
		V.RollLine = FString::Printf(TEXT("%s: d20 %d %s = %d vs DC %d  -  %s"), *SkillName(Skill), Nat, *Sgn(Mod), Total, C.DC, DegreeName(Deg));
		O = Deg >= 2 ? C.Ok : (Deg == 0 && C.bHasCritFail ? C.CritFail : C.Fail);
		if (Deg == 3) { O.Gold = O.Gold * 3 / 2; O.Xp = O.Xp * 3 / 2; }
	}
	else O = C.Out;
	ApplyOutcome(O, V);
	MarkUI();
}

void AHollowGameMode::ApplyOutcome(const FOutcome& O, FEventView& V)
{
	V.Result = O.Text;
	if (!O.Dmg.IsEmpty())
	{
		const int32 Dm = R(O.Dmg);
		PC.Hp = FMath::Max(0, PC.Hp - Dm);
		V.Chips.Add({ FString::Printf(TEXT("-%d HP"), Dm), false });
	}
	if (O.Gold) { PC.Gold = FMath::Max(0, PC.Gold + O.Gold); V.Chips.Add({ FString::Printf(TEXT("%s silver"), *Sgn(O.Gold)), O.Gold > 0 }); }
	if (O.Rations) { if (O.Rations > 0) Give(TEXT("ration"), O.Rations); else PC.Inv.Take(TEXT("ration"), FMath::Min(-O.Rations, PC.Count(TEXT("ration")))); V.Chips.Add({ FString::Printf(TEXT("%s rations"), *Sgn(O.Rations)), O.Rations > 0 }); }
	if (O.Potions) { Give(TEXT("potion"), O.Potions); V.Chips.Add({ FString::Printf(TEXT("+%d potion"), O.Potions), true }); }
	if (O.Holy) { Give(TEXT("holy"), O.Holy); V.Chips.Add({ FString::Printf(TEXT("+%d holy water"), O.Holy), true }); }
	if (O.Heal == TEXT("full")) { V.Chips.Add({ FString::Printf(TEXT("+%d HP"), PC.HpMax - PC.Hp), true }); PC.Hp = PC.HpMax; }
	else if (O.Heal == TEXT("half")) { const int32 Hh = FMath::Min(PC.HpMax - PC.Hp, PC.HpMax / 2); PC.Hp += Hh; V.Chips.Add({ FString::Printf(TEXT("+%d HP"), Hh), true }); }
	if (O.Xp) { GainXp(O.Xp); V.Chips.Add({ FString::Printf(TEXT("+%d XP"), O.Xp), true }); }
	if (!O.Combat.IsEmpty()) { V.PendingCombat = O.Combat; V.bAmbush = O.bAmbush; V.bWeaken = O.bWeaken; }
	if (O.bNoClear) V.bClears = false;
	// shrine-style outcomes leave a blessing that carries into the next fight
	if (O.bSlots) { AddBuff(TEXT("blessed"), TEXT("Dawn's Blessing"), TEXT("+1 to attack rolls for the first 3 turns of your next fight."), FLinearColor(1.f, 0.82f, 0.4f), 1); V.Chips.Add({ TEXT("Blessed"), true }); }
}

void AHollowGameMode::CloseEvent()
{
	if (!Event.IsSet() || Event->Choice < 0) return;
	const FEventView V = Event.GetValue();
	const FEventDef* E = HollowData::FindEvent(V.EventId);
	Event.Reset();
	if (E) Log(FString::Printf(TEXT("%s: %s"), *E->Title, *V.Result));
	if (PC.Hp <= 0) { Die(TEXT("The wilds were too much.")); return; }
	if (!V.PendingCombat.IsEmpty())
	{
		// slipping up unseen turns any fight into your ambush; being spotted first takes the edge away
		const bool bSneak = !bNoticed && (HeroVis == EVis::Unseen || HeroVis == EVis::Hidden);
		if (bSneak && !V.bAmbush) Log(TEXT("They never saw you coming."));
		StartCombat(V.PendingCombat, V.bAmbush || bSneak, V.bWeaken);
		if (Combat.IsSet()) Combat->bFromEvent = V.bHexEvent && V.bClears;
	}
	else if (V.bHexEvent && V.bClears) ResolveEventDone();
	MarkUI();
}

void AHollowGameMode::ResolveEventDone()
{
	FHollowHex& H = Hexes.FindChecked(Pos);
	H.bEventDone = true;
	H.State = EHexState::Cleared;
	for (int32 i = Interactables.Num() - 1; i >= 0; --i)
		if (Interactables[i].Kind == TEXT("event"))
		{
			if (AActor* A = Interactables[i].Actor.Get()) A->Destroy();
			Interactables.RemoveAt(i);
		}
}

/* ---------- Talk ---------- */

FString AHollowGameMode::StripBrackets(const FString& Text) const
{
	return Text.Replace(TEXT("["), TEXT("")).Replace(TEXT("]"), TEXT(""));
}

void AHollowGameMode::LearnTopics(const FString& Text)
{
	int32 From = 0;
	while (true)
	{
		const int32 Open = Text.Find(TEXT("["), ESearchCase::IgnoreCase, ESearchDir::FromStart, From);
		if (Open == INDEX_NONE) break;
		const int32 Close = Text.Find(TEXT("]"), ESearchCase::IgnoreCase, ESearchDir::FromStart, Open);
		if (Close == INDEX_NONE) break;
		const FString Word = Text.Mid(Open + 1, Close - Open - 1).ToLower();
		for (const FTopicDef& T : HollowData::Topics())
			if (T.Id == Word || T.Keywords.Contains(Word) || T.Label.ToLower() == Word) { KnownTopics.Add(T.Id); break; }
		From = Close + 1;
	}
}

void AHollowGameMode::OpenTalk(const FString& NpcId)
{
	const FNpcDef* N = HollowData::FindNpc(NpcId);
	if (!N) return;
	FTalkView T;
	T.NpcId = NpcId;
	Talk = T;
	SetTalkLine(N->Greeting);
	LearnTopics(N->Greeting);
	MarkUI();
}

void AHollowGameMode::CloseTalk()
{
	Talk.Reset();
	MarkUI();
}

void AHollowGameMode::AskTopic(const FString& TopicId)
{
	if (!Talk.IsSet()) return;
	const FNpcDef* N = HollowData::FindNpc(Talk->NpcId);
	if (!N) return;
	const FTopicDef* T = HollowData::FindTopic(TopicId);
	Talk->Asked = T ? T->Label : TopicId;
	Talk->LeadNote.Reset();
	if (N->Replies.Contains(TopicId)) Reply(*N, TopicId);
	else SetTalkLine(N->Unknown);
	Talk->History.Add({ Talk->Asked, Talk->Line });
	MarkUI();
}

void AHollowGameMode::Reply(const FNpcDef& Npc, const FString& TopicId, bool bSetLine)
{
	const FNpcReply& Rp = Npc.Replies.FindChecked(TopicId);
	if (bSetLine) SetTalkLine(Rp.Text);
	KnownTopics.Add(TopicId);
	LearnTopics(Rp.Text);
	const FString OnceKey = Npc.Id + TEXT(":") + TopicId;
	if (!GrantedOnce.Contains(OnceKey))
	{
		GrantedOnce.Add(OnceKey);
		if (Rp.Gold) { PC.Gold += Rp.Gold; Toast(FString::Printf(TEXT("+%d silver"), Rp.Gold)); }
		if (Rp.Rations) { Give(TEXT("ration"), Rp.Rations); Toast(FString::Printf(TEXT("+%d rations"), Rp.Rations)); }
		if (Rp.Holy) { Give(TEXT("holy"), Rp.Holy); Toast(TEXT("Received a flask of holy water.")); }
	}
	if (!Rp.Lead.IsEmpty() && !LeadsGiven.Contains(Rp.Lead))
	{
		GrantLead(Rp.Lead);
		if (const FLeadDef* L = HollowData::FindLead(Rp.Lead))
			Talk->LeadNote = FString::Printf(TEXT("New route on your map: %s. Press M to scout it."), *L->Name);
	}
}

void AHollowGameMode::Service(const FString& What)
{
	auto Pay = [&](int32 Cost) { if (PC.Gold < Cost) { Toast(TEXT("Not enough silver.")); return false; } PC.Gold -= Cost; return true; };
	if (What == TEXT("rest") && Pay(5))
	{
		PC.Hp = PC.HpMax; Day++; Rest();
		Toast(TEXT("You sleep like the dead. HP fully restored."));
		Log(TEXT("Rested at the Sleeping Lantern."));
	}
	else if (What == TEXT("holy") && Pay(10)) { Give(TEXT("holy"), 1); Toast(TEXT("+1 holy water")); }
	else if (What == TEXT("prayer") && Pay(6)) { const int32 H = FMath::Min(PC.HpMax - PC.Hp, R(TEXT("2d8+5"))); PC.Hp += H; Toast(FString::Printf(TEXT("The Dawn mends you: +%d HP"), H)); }
	else if (What == TEXT("potion") && Pay(8)) { Give(TEXT("potion"), 1); Toast(TEXT("+1 healing potion")); }
	else if (What == TEXT("ration") && Pay(2)) { Give(TEXT("ration"), 1); Toast(TEXT("+1 ration")); }
	else if (What == TEXT("pack") && Pay(12))
	{
		FRewardView Rv;
		Rv.Title = TEXT("Pip's card pack: keep one");
		TArray<FString> Pool = HollowData::RewardPool(PC.Kit);
		for (int32 i = 0; i < 3 && Pool.Num(); ++i) Rv.Options.Add(Pool[FMath::RandRange(0, Pool.Num() - 1)]);
		Reward = Rv;
	}
	MarkUI();
}

void AHollowGameMode::PickReward(int32 Index)
{
	if (!Reward.IsSet()) return;
	if (Reward->Options.IsValidIndex(Index))
	{
		const FString Id = Reward->Options[Index];
		PC.Deck.Add(Id);
		if (const FCardDef* C = HollowData::Cards().Find(Id))
		{
			Toast(FString::Printf(TEXT("%s added to your deck."), *C->Name));
			Log(FString::Printf(TEXT("New card: %s."), *C->Name));
		}
	}
	Reward.Reset();
	MarkUI();
}

/* ---------- Local areas ---------- */

void AHollowGameMode::AddInteract(AActor* A, const FString& Kind, const FString& Id, const FString& Label, int32 Index, float Radius)
{
	FInteractable I;
	I.Actor = A; I.Loc = A->GetActorLocation(); I.Kind = Kind; I.Id = Id; I.Label = Label; I.Index = Index; I.Radius = Radius;
	Interactables.Add(I);
}

void AHollowGameMode::ClearLocal()
{
	FeatureGlows.Reset();
	WatcherActors.Reset();
	bNoticed = false;
	NoticeClock = 0.f;
	for (AActor* A : LocalActors) if (IsValid(A)) A->Destroy();
	LocalActors.Reset();
	Fow.Reset();
	Interactables.Reset();
	NearIdx = INDEX_NONE;
}

void AHollowGameMode::EnterHex(const FIntPoint& Where, bool bTravel)
{
	Pos = Where;
	FHollowHex& H = Hexes.FindChecked(Where);
	const bool bFirst = !H.IsVisited();
	if (H.State < EHexState::Explored) H.State = EHexState::Explored;
	if (H.bEventDone) H.State = EHexState::Cleared;
	ClearLocal();
	BuildLocal(Where);
	Mode = EHollowMode::Explore;
	FVector Spawn = LocalOrigin + FVector(-1700.f, 0.f, 150.f);
	if (ArriveSide >= 0)
	{
		const float Yaw = FMath::DegreesToRadians(60.f * ArriveSide);
		Spawn = LocalOrigin + FVector(FMath::Cos(Yaw), FMath::Sin(Yaw), 0.f) * LocalRadius * 0.866f * 0.66f + FVector(0, 0, 150.f);
		ArriveSide = -1;
	}
	SpawnHero(Spawn);
	Log(FString::Printf(TEXT("Entered %s."), *TerrInfo(H.T).Name));
	PlayMusic(H.T == ETerr::Town ? TEXT("dark_cavern_ambient_002") : TEXT("horror_atmosphere_loop"), H.T == ETerr::Town ? 0.45f : 0.35f);
	if (bTravel && bFirst && H.T != ETerr::Town && H.T != ETerr::Keep && H.T != ETerr::Shrine && FMath::FRand() < 0.22f)
		ShowEvent(TEXT("ambush_wolf"));
	MarkUI();
}

void AHollowGameMode::BuildLocal(const FIntPoint& Where)
{
	const FHollowHex& H = Hexes.FindChecked(Where);
	AActor* G = Local(HollowArt::NewActor(GetWorld(), LocalOrigin));
	HollowArt::HexPrism(G, LocalRadius, 500.f, TerrInfo(H.T).Ground, true);
	HollowArt::HexPrism(G, LocalRadius * 1.015f, 520.f, TerrInfo(H.T).Ground * 0.45f, false, FVector(0, 0, -20.f));
	FRandomStream Rng(Where.X * 7919 + Where.Y * 104729 + 17);
	BuildRim(H, Rng);

	switch (H.T)
	{
	case ETerr::Town: BuildTown(); break;
	case ETerr::Shrine: BuildShrine(); break;
	case ETerr::Keep: BuildKeepArea(); break;
	default: BuildWild(H); break;
	}
	BuildEdges();
	BuildFow(Hexes.FindChecked(Where));

	// real grass and shrubs from the local Fab packs (skipped silently if they are not installed)
	const FVector O = LocalOrigin;
	const ETerr T = H.T;
	auto Reject = [&](const FVector& P)
	{
		const FVector Rl = P - O;
		if (FMath::Abs(Rl.Y) < 200.f && FMath::Abs(Rl.X) > 500.f) return true;           // main road strip
		if (T == ETerr::Town) return Rl.Size2D() < 1560.f;
		if (T == ETerr::Shrine || T == ETerr::Keep) return Rl.Size2D() < 650.f || (Rl.X > 300.f && T == ETerr::Keep);
		return Rl.Size2D() < 480.f;
	};
	const int32 Count = T == ETerr::Town ? 900 : T == ETerr::Swamp ? 1100 : T == ETerr::Ruins || T == ETerr::Keep ? 1000 : 2600;
	Local(HollowArt::GroundCover(GetWorld(), O, LocalRadius * 0.866f, Count, Where.X * 131 + Where.Y * 17 + 3, Reject));
	FRandomStream PR(Where.X * 59 + Where.Y * 211 + 9);
	const FString Kind = T == ETerr::Forest || T == ETerr::River ? TEXT("lingon") : T == ETerr::Shrine ? TEXT("anemone") : TEXT("heather");
	for (int32 i = 0; i < (T == ETerr::Town ? 6 : 10); ++i)
	{
		const float A = PR.FRandRange(0.f, 2 * PI);
		const float Rr = T == ETerr::Town ? PR.FRandRange(1600.f, 2000.f) : PR.FRandRange(600.f, 1900.f);
		const FVector P = O + FVector(FMath::Cos(A) * Rr, FMath::Sin(A) * Rr, 0);
		if (!Reject(P)) Local(HollowArt::Plants(GetWorld(), P, Kind, PR.RandRange(3, 7), 140.f, PR.RandRange(0, 99999)));
	}
}

// Mossy boulders and pooled fog around the island edge, leaving the south and north road gaps open.
void AHollowGameMode::BuildRim(const FHollowHex& Hex, FRandomStream& Rng)
{
	UWorld* W = GetWorld();
	const float R = LocalRadius;
	const float Apothem = R * 0.866f;
	const FLinearColor FogCol = Hex.T == ETerr::Swamp ? FLinearColor(0.55f, 0.65f, 0.58f) : FLinearColor(0.6f, 0.62f, 0.72f);
	int32 Count = 0;
	for (int32 i = 0; i < 6; ++i)
	{
		const float A0 = FMath::DegreesToRadians(60.f * i + 30.f), A1 = FMath::DegreesToRadians(60.f * (i + 1) + 30.f);
		const FVector C0(FMath::Cos(A0) * R, FMath::Sin(A0) * R, 0), C1(FMath::Cos(A1) * R, FMath::Sin(A1) * R, 0);
		const int32 Steps = FMath::RoundToInt(R / 380.f);
		for (int32 k = 0; k < Steps; ++k)
		{
			const FVector P = FMath::Lerp(C0, C1, (k + 0.5f) / Steps) * Rng.FRandRange(0.88f, 0.96f);
			// leave a gap wherever a trail leads out (and the old north/south road gaps in town)
			bool bGap = Hex.T == ETerr::Town && (FVector::Dist2D(P, FVector(-Apothem, 0, 0)) < 520.f || FVector::Dist2D(P, FVector(Apothem, 0, 0)) < 520.f);
			for (int32 s = 0; s < 6 && !bGap; ++s)
				if (SideOpen(s)) bGap = FVector::Dist2D(P, FVector(FMath::Cos(FMath::DegreesToRadians(60.f * s)), FMath::Sin(FMath::DegreesToRadians(60.f * s)), 0) * Apothem) < 560.f;
			if (bGap) continue;
			Local(HollowArt::Boulders(W, LocalOrigin + P, Rng.FRandRange(0.8f, 1.3f), Rng.RandRange(0, 99999)));
			if (Count++ % 2 == 0) Local(HollowArt::FogBank(W, LocalOrigin + P * 1.02f + FVector(0, 0, 40), Rng.FRandRange(600.f, 900.f), Hex.T == ETerr::Swamp ? 0.5f : 0.3f, FogCol));
		}
	}
}

void AHollowGameMode::BuildTown()
{
	UWorld* W = GetWorld();
	const FVector O = LocalOrigin;
	const FLinearColor Cobble(0.13f, 0.125f, 0.13f), Dirt(0.11f, 0.085f, 0.06f);
	AActor* G = Local(HollowArt::NewActor(W, O));
	HollowArt::Part(G, HollowArt::EShape::Cylinder, FVector(0, 0, 2), FVector(1500, 1500, 4), Cobble);
	HollowArt::Part(G, HollowArt::EShape::Cube, FVector(-1300, 0, 2), FVector(1100, 260, 4), Dirt);
	HollowArt::Part(G, HollowArt::EShape::Cube, FVector(1350, 0, 2), FVector(1100, 260, 4), Dirt);
	AActor* Fountain = Local(HollowArt::EldritchFountain(W, O));
	AddInteract(Fountain, TEXT("examine"), TEXT("fountain"), TEXT("Examine the tentacle fountain"), 0, 520.f);

	auto Polar = [&](float Deg, float R) { const float A = FMath::DegreesToRadians(Deg); return O + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0); };
	auto FaceIn = [](float Deg) { return Deg + 180.f; };

	AActor* Shard = Local(HollowArt::Monolith(W, Polar(140, 620), 0.8f));
	AddInteract(Shard, TEXT("examine"), TEXT("monolith"), TEXT("Examine the Fallen Shard"), 0, 340.f);

	AActor* Tavern = Local(HollowArt::House(W, Polar(-48, 1250), FaceIn(-48), FLinearColor(0.3f, 0.24f, 0.18f), FLinearColor(0.16f, 0.07f, 0.06f), FVector(460, 460, 400), true));
	AddInteract(Tavern, TEXT("examine"), TEXT("tavern"), TEXT("Look over the Sleeping Lantern"), 0, 420.f);
	AActor* Temple = Local(HollowArt::House(W, Polar(48, 1250), FaceIn(48), FLinearColor(0.3f, 0.3f, 0.32f), FLinearColor(0.12f, 0.1f, 0.16f), FVector(460, 460, 420), false, true));
	HollowArt::GlowPart(Temple, HollowArt::EShape::Sphere, FVector(0, 0, 720), FVector(80, 80, 80), FLinearColor(0.55f, 0.25f, 1.f));
	HollowArt::Light(Temple, FVector(0, 0, 720), FLinearColor(0.55f, 0.25f, 1.f), 40.f, 1400.f);
	AddInteract(Temple, TEXT("examine"), TEXT("temple"), TEXT("Examine the temple"), 0, 420.f);

	const TArray<TPair<float, float>> Homes = { { -105, 1250 }, { -150, 1200 }, { 105, 1250 }, { 150, 1200 }, { -80, 1850 }, { 80, 1850 } };
	const TArray<FLinearColor> Walls = { FLinearColor(0.3f, 0.26f, 0.2f), FLinearColor(0.24f, 0.2f, 0.16f), FLinearColor(0.28f, 0.27f, 0.25f) };
	const TArray<FLinearColor> Roofs = { FLinearColor(0.09f, 0.1f, 0.14f), FLinearColor(0.16f, 0.07f, 0.06f), FLinearColor(0.1f, 0.12f, 0.08f) };
	for (int32 i = 0; i < Homes.Num(); ++i)
		Local(HollowArt::House(W, Polar(Homes[i].Key, Homes[i].Value), FaceIn(Homes[i].Key), Walls[i % 3], Roofs[(i + 1) % 3], FVector(460, 460, i % 2 ? 400 : 240)));

	for (int32 i = 0; i < 10; ++i)
		if (i != 0 && i != 5) Local(HollowArt::Lantern(W, Polar(i * 36.f + 8.f, 830)));
	Local(HollowArt::Stall(W, Polar(-90, 760), 90.f, FLinearColor(0.5f, 0.1f, 0.08f)));
	Local(HollowArt::Stall(W, Polar(-122, 700), 120.f, FLinearColor(0.1f, 0.3f, 0.12f)));

	AActor* Board = Local(HollowArt::NewActor(W, Polar(-18, 760), 162.f));
	HollowArt::Part(Board, HollowArt::EShape::Cube, FVector(0, 0, 90), FVector(10, 140, 100), FLinearColor(0.16f, 0.1f, 0.06f), FRotator::ZeroRotator, true);
	for (int32 k = 0; k < 4; ++k)
		HollowArt::Part(Board, HollowArt::EShape::Cube, FVector(7, -45 + k * 30.f, 95 + (k % 2) * 12.f), FVector(2, 24, 30), FLinearColor(0.55f, 0.5f, 0.4f));
	AddInteract(Board, TEXT("examine"), TEXT("notice"), TEXT("Read the notice board"), 0, 260.f);

	// the new graves and the Alderwick crypt, south-east of the square
	FRandomStream Rng(1337);
	AActor* GraveAnchor = nullptr;
	for (int32 i = 0; i < 8; ++i)
	{
		AActor* Gr = Local(HollowArt::Grave(W, Polar(122 + (i % 4) * 9.f, 1580 + (i / 4) * 200.f), 122.f + 180.f));
		if (!GraveAnchor) GraveAnchor = Gr;
	}
	if (GraveAnchor) AddInteract(GraveAnchor, TEXT("examine"), TEXT("graves"), TEXT("Examine the fresh graves"), 0, 360.f);
	AActor* Crypt = Local(HollowArt::NewActor(W, Polar(152, 1820), 152.f + 180.f));
	if (!HollowArt::KPart(Crypt, TEXT("Graveyard/crypt-large"), FVector::ZeroVector, 0.f, 2.4f))
		HollowArt::Part(Crypt, HollowArt::EShape::Cube, FVector(0, 0, 120), FVector(300, 400, 240), FLinearColor(0.2f, 0.2f, 0.22f));
	HollowArt::Collider(Crypt, FVector(0, 0, 120), FVector(420, 560, 240));
	HollowArt::Light(Crypt, FVector(300, 0, 120), FLinearColor(0.35f, 1.f, 0.55f), 8.f, 500.f);
	AddInteract(Crypt, TEXT("examine"), TEXT("crypt"), TEXT("Examine the crypt"), 0, 520.f);

	for (int32 i = 0; i < 18; ++i)
	{
		const float A0 = i * 20.f + 10.f, A1 = A0 + 20.f;
		if (FMath::Abs(FRotator::NormalizeAxis(A0 + 10.f)) < 15.f || FMath::Abs(FRotator::NormalizeAxis(A0 + 10.f - 180.f)) < 15.f) continue;
		Local(HollowArt::Fence(W, Polar(A0, 2020), Polar(A1, 2020)));
	}
	for (int32 i = 0; i < 16; ++i)
	{
		const float A = Rng.FRandRange(0.f, 360.f);
		if (FMath::Abs(FRotator::NormalizeAxis(A)) < 18.f || FMath::Abs(FRotator::NormalizeAxis(A - 180.f)) < 18.f) continue;
		const FVector P = Polar(A, Rng.FRandRange(2120.f, 2260.f));
		if (Rng.FRand() < 0.4f) Local(HollowArt::DeadTree(W, P, Rng.FRandRange(0.9f, 1.3f), Rng.RandRange(0, 99999)));
		else Local(HollowArt::PineTree(W, P, Rng.FRandRange(0.9f, 1.2f), FLinearColor::Black));
	}
	for (int32 i = 0; i < 30; ++i) Local(HollowArt::Tuft(W, Polar(Rng.FRandRange(0.f, 360.f), Rng.FRandRange(1550.f, 2000.f)), FLinearColor(0.22f, 0.2f, 0.1f)));
	for (int32 i = 0; i < 10; ++i) Local(HollowArt::Shrub(W, Polar(Rng.FRandRange(0.f, 360.f), Rng.FRandRange(1500.f, 1980.f)), Rng.FRandRange(0.8f, 1.3f)));
	Local(HollowArt::FogBank(W, O, 2600.f, 0.025f, FLinearColor(0.45f, 0.38f, 0.6f)));
	for (float Ang : { 25.f, -25.f, 62.f, -62.f, 140.f, -140.f })
		Local(HollowArt::RuneObelisk(W, Polar(Ang, 2160.f), FMath::FRandRange(0.9f, 1.15f)));
	for (float Ang : { 8.f, 116.f, -116.f, 224.f })
		Local(HollowArt::Banner(W, Polar(Ang, 900.f), Ang + 90.f));

	auto Place = [&](const FString& Id, const FVector& At, float Yaw)
	{
		const FNpcDef* N = HollowData::FindNpc(Id);
		AActor* A = Local(HollowArt::Npc(W, At, Yaw, N->Body, N->Skin, N->Hat, N->HatStyle));
		HollowArt::Light(A, FVector(60, 0, 220), FLinearColor(1.f, 0.75f, 0.5f), 4.f, 320.f);
		AddInteract(A, TEXT("npc"), Id, FString::Printf(TEXT("Talk to %s, %s"), *N->Name, *N->Role));
	};
	Place(TEXT("tobb"), Polar(-48, 880), FaceIn(-48));
	Place(TEXT("ysolde"), Polar(48, 880), FaceIn(48));
	Place(TEXT("pip"), Polar(-90, 600), 90.f + 180.f);
	Place(TEXT("maren"), O + FVector(1750, 220, 0), 180.f);

	Local(HollowArt::Lantern(W, O + FVector(2000, 260, 0)));
	Local(HollowArt::Lantern(W, O + FVector(2000, -260, 0)));
	AActor* Sign = Local(HollowArt::Signpost(W, O + FVector(1950, -120, 0), 0.f));
	AddInteract(Sign, TEXT("exit"), TEXT(""), TEXT("North gate: open the map of the March"));
	AActor* Sign2 = Local(HollowArt::Signpost(W, O + FVector(-1950, 160, 0), 180.f));
	AddInteract(Sign2, TEXT("exit"), TEXT(""), TEXT("South road: open the map of the March"));
}

void AHollowGameMode::BuildShrine()
{
	UWorld* W = GetWorld();
	const FVector O = LocalOrigin;
	AActor* G = Local(HollowArt::NewActor(W, O));
	HollowArt::Part(G, HollowArt::EShape::Cube, FVector(-1100, 0, 2), FVector(1500, 240, 4), FLinearColor(0.12f, 0.1f, 0.08f));
	Local(HollowArt::Shrine(W, O + FVector(350, 0, 0)));
	Local(HollowArt::RitualCircle(W, O + FVector(-650, 900, 0), 360.f, FLinearColor(0.55f, 0.25f, 1.f)));
	FHollowHex& H = Hexes.FindChecked(Pos);
	if (!H.bEventDone)
	{
		AActor* M = Local(HollowArt::Marker(W, O + FVector(-50, 0, 0), FLinearColor(0.6f, 0.35f, 1.f)));
		AddInteract(M, TEXT("event"), TEXT("shrine"), TEXT("Approach the Starfall Altar"), 0, 450.f);
	}
	AActor* Sky = Local(HollowArt::NewActor(W, O + FVector(-300, -300, 0)));
	AddInteract(Sky, TEXT("examine"), TEXT("sky"), TEXT("Look up at the Sleeping Star"), 0, 300.f);
	Local(HollowArt::Tent(W, O + FVector(-300, -900, 0), 60.f, FLinearColor(0.5f, 0.45f, 0.4f)));
	Local(HollowArt::Tent(W, O + FVector(-650, -700, 0), -30.f, FLinearColor(0.4f, 0.3f, 0.25f)));
	Local(HollowArt::Campfire(W, O + FVector(-650, -350, 0)));
	Local(HollowArt::Npc(W, O + FVector(-500, -550, 0), 30.f, FLinearColor(0.2f, 0.15f, 0.25f), FLinearColor(0.6f, 0.45f, 0.35f), FLinearColor(0.15f, 0.1f, 0.2f), 4));
	Local(HollowArt::Npc(W, O + FVector(-820, -260, 0), -20.f, FLinearColor(0.22f, 0.18f, 0.14f), FLinearColor(0.7f, 0.55f, 0.45f), FLinearColor(0.12f, 0.1f, 0.08f), 4));
	FRandomStream Rng(77);
	for (int32 i = 0; i < 14; ++i)
	{
		const float A = Rng.FRandRange(0.f, 2 * PI);
		const FVector P = O + FVector(FMath::Cos(A), FMath::Sin(A), 0) * Rng.FRandRange(1300.f, 1850.f);
		if (P.X < -900 && FMath::Abs(P.Y) < 400) continue;
		Local(HollowArt::DeadTree(W, P, Rng.FRandRange(0.8f, 1.3f), Rng.RandRange(0, 99999)));
	}
	for (int32 i = 0; i < 16; ++i)
	{
		const float A = Rng.FRandRange(0.f, 2 * PI);
		Local(HollowArt::Shrub(W, O + FVector(FMath::Cos(A), FMath::Sin(A), 0) * Rng.FRandRange(800.f, 1700.f), Rng.FRandRange(0.8f, 1.2f)));
	}
	AActor* Sign = Local(HollowArt::Signpost(W, O + FVector(-1850, 250, 0), 0.f));
	AddInteract(Sign, TEXT("exit"), TEXT(""), TEXT("Pilgrim road: open the map"));
}

void AHollowGameMode::BuildKeepArea()
{
	UWorld* W = GetWorld();
	const FVector O = LocalOrigin;
	AActor* G = Local(HollowArt::NewActor(W, O));
	HollowArt::Part(G, HollowArt::EShape::Cube, FVector(-600, 0, 2), FVector(2400, 300, 4), FLinearColor(0.09f, 0.085f, 0.095f));
	Local(HollowArt::Keep(W, O + FVector(1250, 0, 0), 180.f));
	FHollowHex& H = Hexes.FindChecked(Pos);
	if (!H.bEventDone)
	{
		AActor* M = Local(HollowArt::Marker(W, O + FVector(650, 0, 0), FLinearColor(0.95f, 0.25f, 0.2f)));
		AddInteract(M, TEXT("event"), TEXT("keep"), TEXT("Approach the gates of Hollowmere"), 0, 420.f);
	}
	FRandomStream Rng(666);
	for (int32 i = 0; i < 16; ++i)
		Local(HollowArt::Grave(W, O + FVector(Rng.FRandRange(-1400.f, 200.f), (Rng.FRand() < 0.5f ? -1 : 1) * Rng.FRandRange(450.f, 1400.f), 0), Rng.FRandRange(-20.f, 20.f)));
	for (int32 i = 0; i < 10; ++i)
	{
		const float A = Rng.FRandRange(0.f, 2 * PI);
		const FVector P = O + FVector(FMath::Cos(A), FMath::Sin(A), 0) * Rng.FRandRange(1400.f, 1850.f);
		if (P.X < -900 && FMath::Abs(P.Y) < 400) continue;
		Local(HollowArt::DeadTree(W, P, Rng.FRandRange(1.f, 1.5f), Rng.RandRange(0, 99999)));
	}
	Local(HollowArt::BrokenWall(W, O + FVector(300, -700, 0), 80.f, 520.f));
	Local(HollowArt::BrokenWall(W, O + FVector(200, 750, 0), 100.f, 420.f));
	Local(HollowArt::RuinArch(W, O + FVector(-300, 0, 0), 0.f, true));
	for (int32 i = 0; i < 4; ++i)
		Local(HollowArt::Pillar(W, O + FVector(-1100 + i * 380.f, i % 2 ? 300.f : -300.f, 0), 300.f, i % 2 == 0));
	Local(HollowArt::FogBank(W, O + FVector(600, 0, 0), 1800.f, 0.25f, FLinearColor(0.55f, 0.45f, 0.6f)));
	AActor* Sign = Local(HollowArt::Signpost(W, O + FVector(-1850, 250, 0), 0.f));
	AddInteract(Sign, TEXT("exit"), TEXT(""), TEXT("Retreat: open the map"));
}

void AHollowGameMode::Scatter(const FHollowHex& Hex, FRandomStream& Rng) {}

void AHollowGameMode::BuildPoi(const FHollowHex& Hex, const FVector& At)
{
	UWorld* W = GetWorld();
	const FString& Id = Hex.EventId;
	auto FoeProp = [&](const FString& FoeId, const FVector& L, float Yaw)
	{
		AActor* A = Local(HollowArt::NewActor(W, L, Yaw));
		HollowArt::Foe(A, nullptr, FoeId, FVector::ZeroVector);
		WatcherActors.Add(A);
		return A;
	};
	if (Id == TEXT("snare") || Id == TEXT("ridge")) { FoeProp(TEXT("wolf"), At + FVector(150, 80, 0), 200.f); Local(HollowArt::Rock(W, At + FVector(300, -200, 0), 1.4f, 30.f)); }
	else if (Id == TEXT("widow")) { Local(HollowArt::House(W, At + FVector(350, 0, 0), 180.f, FLinearColor(0.55f, 0.4f, 0.25f), FLinearColor(0.3f, 0.45f, 0.25f), FVector(300, 260, 200))); Local(HollowArt::Npc(W, At + FVector(120, 120, 0), 180.f, FLinearColor(0.4f, 0.3f, 0.45f), FLinearColor(0.9f, 0.75f, 0.65f), FLinearColor(0.85f, 0.85f, 0.85f), 0)); }
	else if (Id == TEXT("barrow")) Local(HollowArt::BarrowDoor(W, At + FVector(380, 0, 0), 180.f));
	else if (Id == TEXT("rockslide")) { for (int32 i = 0; i < 6; ++i) Local(HollowArt::Rock(W, At + FVector(250 + i * 40.f, -250 + i * 90.f, 0), 1.2f + (i % 3) * 0.4f, i * 50.f)); }
	else if (Id == TEXT("wisps"))
	{
		Local(HollowArt::Pool(W, At + FVector(250, 0, 0), 320.f, FLinearColor(0.1f, 0.2f, 0.2f)));
		AActor* A = Local(HollowArt::NewActor(W, At));
		for (int32 i = 0; i < 5; ++i) HollowArt::GlowPart(A, EShape::Sphere, FVector(200 + i * 60.f, (i - 2) * 110.f, 120 + (i % 2) * 60.f), FVector(28, 28, 28), FLinearColor(0.6f, 1.f, 0.9f));
	}
	else if (Id == TEXT("drowned") || Id == TEXT("fish") || Id == TEXT("ford")) { Local(HollowArt::Pool(W, At + FVector(300, 0, 0), 330.f, FLinearColor(0.15f, 0.35f, 0.5f))); Local(HollowArt::Reeds(W, At + FVector(120, 280, 0))); Local(HollowArt::Reeds(W, At + FVector(160, -300, 0))); }
	else if (Id == TEXT("choir"))
	{
		for (int32 i = 0; i < 6; ++i) Local(HollowArt::Pillar(W, At + FVector(250 + FMath::Cos(i * PI / 3) * 330, FMath::Sin(i * PI / 3) * 330, 0), 280.f, i % 3 == 0));
		FoeProp(TEXT("skeleton"), At + FVector(300, -80, 0), 180.f);
		FoeProp(TEXT("skeleton"), At + FVector(300, 90, 0), 180.f);
	}
	else if (Id == TEXT("reliquary")) { Local(HollowArt::Pillar(W, At + FVector(300, 0, 0), 140.f, true)); AActor* B = Local(HollowArt::NewActor(W, At + FVector(220, 0, 0))); HollowArt::Part(B, EShape::Cube, FVector(0, 0, 30), FVector(70, 90, 60), FLinearColor(0.25f, 0.25f, 0.3f)); }
	else if (Id == TEXT("toll")) { FoeProp(TEXT("bandit"), At + FVector(260, -90, 0), 180.f); FoeProp(TEXT("bandit"), At + FVector(300, 110, 0), 180.f); Local(HollowArt::Fence(W, At + FVector(420, -300, 0), At + FVector(420, 300, 0))); }
	else if (Id == TEXT("pilgrims")) { Local(HollowArt::Tent(W, At + FVector(350, -200, 0), 90.f, FLinearColor(0.95f, 0.9f, 0.75f))); Local(HollowArt::Campfire(W, At + FVector(250, 150, 0))); Local(HollowArt::Npc(W, At + FVector(150, 300, 0), 200.f, FLinearColor(0.9f, 0.85f, 0.7f), FLinearColor(0.8f, 0.6f, 0.45f), FLinearColor(0.95f, 0.8f, 0.3f), 3)); }
	else if (Id == TEXT("abandoned")) { Local(HollowArt::Tent(W, At + FVector(350, 0, 0), 70.f, FLinearColor(0.6f, 0.5f, 0.35f))); Local(HollowArt::Campfire(W, At + FVector(180, 200, 0))); }
	else if (Id == TEXT("feeding")) { FoeProp(TEXT("ghoul"), At + FVector(250, 0, 0), 90.f); AActor* B = Local(HollowArt::NewActor(W, At + FVector(300, 60, 0))); for (int32 i = 0; i < 4; ++i) HollowArt::Part(B, EShape::Cylinder, FVector(i * 15.f, i * 20.f - 30, 6), FVector(10, 10, 70), FLinearColor(0.92f, 0.9f, 0.82f), FRotator(90, i * 40.f, 0)); }
	else { Local(HollowArt::Haystack(W, At + FVector(300, 120, 0))); Local(HollowArt::Signpost(W, At + FVector(200, -150, 0), 0.f)); }

	if (!Hex.bEventDone)
	{
		const FEventDef* E = HollowData::FindEvent(Id);
		AActor* M = Local(HollowArt::Marker(W, At, FLinearColor(1.f, 0.8f, 0.3f)));
		AddInteract(M, TEXT("event"), Id, E ? FString::Printf(TEXT("Investigate: %s"), *E->Title) : FString(TEXT("Investigate")), 0, 420.f);
	}
}

// Wild hexes follow the concept art (Docs/Concept): a winding road from the south gap to the north gap, a landmark beside it,
// a rocky ridge on the far edge with a waterfall feeding a stream under a stone bridge, set pieces (ruined arch with a lone
// fire, ritual circle, broken wagon and bones), then dark pines, autumn trees, gnarled oaks, rust shrubs and dry grass.
void AHollowGameMode::BuildWild(const FHollowHex& Hex)
{
	UWorld* W = GetWorld();
	const FVector O = LocalOrigin;
	FRandomStream Rng(Hex.Q * 7919 + Hex.R * 104729 + 31);
	const float Ap = LocalRadius * 0.866f;

	// the road
	TArray<FVector> Road;
	const float Phase = Rng.FRandRange(0.f, 2 * PI), Sway = Rng.FRandRange(220.f, 420.f);
	for (float X = -Ap - 50.f; X <= Ap + 50.f; X += 260.f)
		Road.Add(O + FVector(X, FMath::Sin(X / 800.f + Phase) * Sway * FMath::Clamp(1.f - FMath::Abs(X) / (Ap * 1.1f), 0.2f, 1.f), 0));
	const FLinearColor Dirt = TerrInfo(Hex.T).Ground * 0.6f + FLinearColor(0.12f, 0.08f, 0.04f);
	Local(HollowArt::PathStrip(W, Road, 230.f, Dirt));

	struct FBlock { FVector P; float R; };
	TArray<FBlock> Blocks;
	auto DistRoad = [&](const FVector& P)
	{
		float Best = 1e9f;
		for (int32 i = 0; i + 1 < Road.Num(); ++i) Best = FMath::Min(Best, FMath::PointDistToSegment(P, Road[i], Road[i + 1]));
		return Best;
	};
	auto Free = [&](const FVector& P, float R)
	{
		if ((P - O).Size2D() > Ap * 0.84f) return false;
		if (DistRoad(P) < 230.f + R) return false;
		for (const FBlock& B : Blocks) if (FVector::Dist2D(B.P, P) < B.R + R) return false;
		return true;
	};
	FVector P;
	auto Pick = [&](float MinR, float MaxR, float R, FVector& Out)
	{
		for (int32 Try = 0; Try < 40; ++Try)
		{
			const float A = Rng.FRandRange(0.f, 2 * PI);
			const FVector C = O + FVector(FMath::Cos(A), FMath::Sin(A), 0) * FMath::Sqrt(Rng.FRandRange(MinR * MinR, MaxR * MaxR));
			if (!Free(C, R)) continue;
			Blocks.Add({ C, R });
			Out = C;
			return true;
		}
		return false;
	};

	// landmark beside the road at the middle of the island
	const FVector Mid = Road[Road.Num() / 2];
	const FVector PoiAt = Mid + FVector(0, (Mid.Y - O.Y) > 0 ? -420.f : 420.f, 0);
	Blocks.Add({ PoiAt, 520.f });
	BuildPoi(Hex, PoiAt);

	// a ridge on the far (north) edge to one side of the road, sometimes with a waterfall
	const float RidgeSide = Rng.FRand() < 0.5f ? 1.f : -1.f;
	const FVector RidgeAt = O + FVector(Ap * 0.62f, RidgeSide * Rng.FRandRange(700.f, 1000.f), 0);
	const bool bRidge = Hex.T != ETerr::Plains || Rng.FRand() < 0.4f;
	if (bRidge)
	{
		Local(HollowArt::Ridge(W, RidgeAt, 90.f + RidgeSide * 20.f, Rng.FRandRange(0.9f, 1.2f), Rng.RandRange(0, 99999)));
		Blocks.Add({ RidgeAt, 900.f });
		for (int32 i = 0; i < 5; ++i)
			Local(HollowArt::PineTree(W, RidgeAt + FVector(Rng.FRandRange(-150.f, 150.f), Rng.FRandRange(-600.f, 600.f), Rng.FRandRange(450.f, 560.f)), Rng.FRandRange(0.9f, 1.3f), FLinearColor::Black));
	}

	// a stream from the ridge (or across the island) crossing the road under a bridge
	const bool bStream = Hex.T == ETerr::River || Hex.T == ETerr::Swamp || (bRidge && Rng.FRand() < 0.6f) || Rng.FRand() < 0.25f;
	if (bStream)
	{
		const float Width = Hex.T == ETerr::River ? 420.f : 280.f;
		TArray<FVector> Water;
		const FVector Start = bRidge ? RidgeAt + FVector(-450.f, -RidgeSide * 380.f, 0) : O + FVector(Ap * 0.8f, RidgeSide * 900.f, 0);
		const FVector End = O + FVector(-Ap * 0.35f, -RidgeSide * Ap * 1.05f, 0);
		for (int32 i = 0; i <= 12; ++i)
		{
			const float T = i / 12.f;
			FVector C = FMath::Lerp(Start, End, T);
			C += FVector(-(End - Start).Y, (End - Start).X, 0).GetSafeNormal() * FMath::Sin(T * PI * 2.f + Phase) * 220.f;
			Water.Add(C);
		}
		Local(HollowArt::Stream(W, Water, Width));
		for (const FVector& C : Water) Blocks.Add({ C, Width * 0.6f });
		if (bRidge)
		{
			Local(HollowArt::Waterfall(W, Start + FVector(160, 0, 520), Start + FVector(40, 0, 0), Width * 0.7f));
			Local(HollowArt::FogBank(W, Start + FVector(0, 0, 60), 560.f, 1.2f, FLinearColor(0.75f, 0.8f, 0.85f)));
		}
		// bridge where the stream meets the road
		float BestD = 1e9f;
		FVector Cross = Road[0];
		for (const FVector& R : Road) for (const FVector& C : Water) { const float Dd = FVector::Dist2D(R, C); if (Dd < BestD) { BestD = Dd; Cross = R; } }
		int32 Ci = 0;
		for (int32 i = 0; i < Road.Num(); ++i) if (Road[i] == Cross) Ci = i;
		const FVector Dir = Road[FMath::Min(Ci + 1, Road.Num() - 1)] - Road[FMath::Max(Ci - 1, 0)];
		Local(HollowArt::Bridge(W, Cross, Dir.Rotation().Yaw, Width + 260.f));
	}

	// set pieces
	const bool bRuins = Hex.T == ETerr::Ruins || Hex.T == ETerr::Hills;
	if ((bRuins || Rng.FRand() < 0.55f) && Pick(800.f, 1700.f, 450.f, P))
	{
		AActor* Arch = Local(HollowArt::RuinArch(W, P, Rng.FRandRange(0.f, 360.f), true));
		Local(HollowArt::BrokenWall(W, P + FVector(Rng.FRandRange(-250.f, 250.f), 300.f, 0), Rng.FRandRange(0.f, 180.f), 460.f));
		AddInteract(Arch, TEXT("examine"), TEXT("arch"), TEXT("Examine the ruined arch"), 0, 420.f);
	}
	if ((Hex.T == ETerr::Ruins || Rng.FRand() < 0.45f) && Pick(800.f, 1700.f, 520.f, P))
	{
		AActor* Circle = Local(HollowArt::RitualCircle(W, P, 340.f, Rng.FRand() < 0.5f ? FLinearColor(0.55f, 0.25f, 1.f) : FLinearColor(0.35f, 1.f, 0.55f)));
		AddInteract(Circle, TEXT("examine"), TEXT("circle"), TEXT("Examine the ritual circle"), 0, 480.f);
	}
	{
		const FVector Near = Road[FMath::Clamp(Road.Num() / 4 + Rng.RandRange(-1, 1), 0, Road.Num() - 1)];
		const FVector WagonAt = Near + FVector(0, Rng.FRand() < 0.5f ? 380.f : -380.f, 0);
		if (Free(WagonAt, 200.f))
		{
			Blocks.Add({ WagonAt, 260.f });
			AActor* Wg = Local(HollowArt::Wagon(W, WagonAt, Rng.FRandRange(0.f, 360.f)));
			AddInteract(Wg, TEXT("examine"), TEXT("wagon"), TEXT("Search the abandoned wagon"), 0, 320.f);
		}
		const FVector BonesAt = Road[FMath::Clamp(Road.Num() / 3, 0, Road.Num() - 1)] + FVector(60, 260, 0);
		AActor* Bn = Local(HollowArt::Bones(W, BonesAt, Rng.FRandRange(0.f, 360.f)));
		AddInteract(Bn, TEXT("examine"), TEXT("bones"), TEXT("Examine the bones"), 0, 240.f);
	}

	// tool-gated spots: what you carry decides what you can do here
	BuildFeatures(Hexes.FindChecked(FIntPoint(Hex.Q, Hex.R)), [&](float MinR, float MaxR, float R, FVector& Out) { return Pick(MinR, MaxR, R, Out); });

	// loot caches
	for (int32 i = 0; i < Hex.ChestTaken.Num(); ++i)
		if (Pick(900.f, 1800.f, 160.f, P) && !Hex.ChestTaken[i])
		{
			AActor* C = Local(HollowArt::Chest(W, P, Rng.FRandRange(0.f, 360.f)));
			AddInteract(C, TEXT("chest"), TEXT(""), TEXT("Search the cache"), i, 220.f);
		}

	// terrain dressing
	auto N = [&](int32 Count, float MinR, float MaxR, float R, TFunctionRef<void(const FVector&)> Fn)
	{
		for (int32 i = 0; i < Count; ++i) if (Pick(MinR, MaxR, R, P)) Fn(P);
	};
	bool bFirstOak = true;
	auto Oak = [&](const FVector& L)
	{
		AActor* T = Local(HollowArt::DeadTree(W, L, Rng.FRandRange(0.9f, 1.6f), Rng.RandRange(0, 99999)));
		if (bFirstOak) { AddInteract(T, TEXT("examine"), TEXT("deadtree"), TEXT("Examine the gnarled oak"), 0, 300.f); bFirstOak = false; }
	};
	auto Pine = [&](const FVector& L) { Local(HollowArt::PineTree(W, L, Rng.FRandRange(0.9f, 1.5f), FLinearColor::Black)); };
	auto Autumn = [&](const FVector& L) { Local(HollowArt::AutumnTree(W, L, Rng.FRandRange(0.8f, 1.2f))); };
	switch (Hex.T)
	{
	case ETerr::Forest:
		N(26, 500, 1900, 150, Pine);
		N(8, 500, 1900, 170, Autumn);
		N(3, 500, 1900, 220, Oak);
		break;
	case ETerr::Plains:
		N(5, 600, 1900, 220, Oak);
		N(6, 600, 1900, 170, Autumn);
		N(4, 600, 1900, 220, [&](const FVector& L) { Local(HollowArt::Haystack(W, L)); });
		N(3, 900, 1800, 260, [&](const FVector& L) { Local(HollowArt::Fence(W, L, L + FVector(Rng.FRandRange(-300.f, 300.f), Rng.FRandRange(-300.f, 300.f), 0))); });
		break;
	case ETerr::Hills:
		N(6, 700, 1800, 380, [&](const FVector& L) { Local(HollowArt::Boulders(W, L, Rng.FRandRange(0.9f, 1.4f), Rng.RandRange(0, 99999))); });
		N(12, 600, 1900, 150, Pine);
		N(4, 600, 1900, 170, Autumn);
		N(3, 600, 1900, 220, Oak);
		break;
	case ETerr::Swamp:
		N(7, 600, 1800, 300, [&](const FVector& L) { Local(HollowArt::Pool(W, L, Rng.FRandRange(160.f, 300.f), FLinearColor(0.02f, 0.04f, 0.035f))); Local(HollowArt::FogBank(W, L + FVector(0, 0, 30), 500.f, 0.9f, FLinearColor(0.5f, 0.6f, 0.55f))); });
		N(26, 500, 1900, 80, [&](const FVector& L) { Local(HollowArt::Reeds(W, L)); });
		N(7, 600, 1900, 220, Oak);
		break;
	case ETerr::Ruins:
		N(10, 600, 1800, 160, [&](const FVector& L) { Local(HollowArt::Pillar(W, L, Rng.FRandRange(160.f, 360.f), Rng.FRand() < 0.6f)); });
		N(5, 700, 1800, 330, [&](const FVector& L) { Local(HollowArt::BrokenWall(W, L, Rng.FRandRange(0.f, 180.f), Rng.FRandRange(300.f, 560.f))); });
		N(8, 600, 1900, 150, Pine);
		N(3, 600, 1900, 220, Oak);
		break;
	case ETerr::River:
		N(18, 500, 1900, 80, [&](const FVector& L) { Local(HollowArt::Reeds(W, L)); });
		N(10, 600, 1900, 150, Pine);
		N(6, 600, 1900, 170, Autumn);
		break;
	default: break;
	}
	N(16, 400, 1950, 60, [&](const FVector& L) { Local(HollowArt::Shrub(W, L, Rng.FRandRange(0.7f, 1.3f))); });
	N(36, 400, 1950, 30, [&](const FVector& L) { Local(HollowArt::Tuft(W, L, FLinearColor(0.26f, 0.22f, 0.1f))); });
	N(6, 500, 1900, 120, [&](const FVector& L) { Local(HollowArt::Rock(W, L, Rng.FRandRange(0.5f, 1.f), Rng.FRandRange(0.f, 360.f))); });

	AActor* Sign = Local(HollowArt::Signpost(W, O + FVector(-Ap + 250.f, 300, 0), 0.f));
	AddInteract(Sign, TEXT("exit"), TEXT(""), TEXT("Trail marker: open the map"));
}
