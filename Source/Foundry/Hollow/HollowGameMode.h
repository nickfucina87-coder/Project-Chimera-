// The Hollowmere director. Owns every piece of game state and builds the three stages the game moves between:
//   the local area (an explorable hex island the hero walks around),
//   the march board (the hex map where talk-revealed routes are scouted and travelled),
//   the battle arena (FF-style turn order + PF2e three-action turns + an action-card hand).
// The Slate UI (SHollowUI) reads this state directly and calls the public verbs below.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HollowTypes.h"
#include "HollowGameMode.generated.h"

class AHollowHero;
class ACameraActor;
class UMaterialInterface;
class UPrimitiveComponent;
class UTextRenderComponent;
class UAudioComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTexture2D;
class SHollowUI;

enum class EHollowMode : uint8 { Title, Create, Explore, Map, Combat, End };

struct FInteractable
{
	TWeakObjectPtr<AActor> Actor;
	FVector Loc = FVector::ZeroVector;
	FString Kind;     // npc, event, chest, exit, examine
	FString Id;
	FString Label;
	int32 Index = 0;
	float Radius = 280.f;
};

struct FEventView
{
	FString EventId;
	int32 Choice = -1;
	FString RollLine;
	FString Result;
	TArray<TPair<FString, bool>> Chips;   // text, good?
	FString PendingCombat;
	bool bAmbush = false, bWeaken = false, bClears = true, bHexEvent = false;
};

struct FTalkView
{
	FString NpcId;
	FString Line;
	FString Asked;
	FString LeadNote;
	bool bThinking = false;
	bool bFromBrain = false;
	int32 Serial = 0;
	TArray<TPair<FString, FString>> History;   // player line, npc line
};

struct FNoteView
{
	FString Title, Text;
};

struct FFighter
{
	FString Name, FoeId;
	bool bHero = false, bDead = false, bUndead = false, bBoss = false;
	int32 Hp = 0, MaxHp = 0, Ac = 10, Atk = 0, Fort = 0, Ref = 0, Will = 0, Per = 0, Speed = 25;
	FString Dmg, Weapon, Special;
	float Ctb = 0.f;
	int32 Frightened = 0, OffGuardTurns = 0, GuardAc = 0, ShieldHp = 0, Slowed = 0, Regen = 0;
	bool bRegenBlocked = false;
	FString Intent, IntentText;
	int32 IntentStep = 0;
	TWeakObjectPtr<AActor> Puppet;
	FVector Home = FVector::ZeroVector;
	float Delay() const { return 1000.f / FMath::Max(5, Speed); }
};

struct FCombatState
{
	TArray<FFighter> F;           // F[0] is always the hero
	int32 Active = -1;
	int32 Actions = 0;
	int32 Attacks = 0;            // multiple attack penalty counter this turn
	int32 Target = 1;
	TArray<FString> Hand, Draw, Discard;
	TArray<int32> HandSerial;
	int32 NextSerial = 1;
	bool bLucky = false, bQuicken = false, bRiposte = false;
	int32 BlessTurns = 0, HymnTurns = 0;
	bool bBusy = false, bOver = false, bWon = false, bFled = false;
	int32 XpGain = 0, GoldGain = 0;
	TArray<FString> Log;
	FIntPoint Hex = FIntPoint::ZeroValue;
	bool bFromEvent = false;
	int32 Round = 1;
};

struct FRewardView
{
	FString Title;
	TArray<FString> Options;
};

UCLASS()
class AHollowGameMode : public AGameModeBase
{
	GENERATED_BODY()
	friend struct FHollowCheats;

public:
	AHollowGameMode();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void RestartPlayer(AController* NewPlayer) override {}

	/* ---------- State the UI reads ---------- */
	EHollowMode Mode = EHollowMode::Title;
	FPC PC;
	int32 Day = 1;
	TMap<FIntPoint, FHollowHex> Hexes;
	FIntPoint Pos = FIntPoint::ZeroValue;
	TArray<FString> Journal;
	TSet<FString> KnownTopics;
	TSet<FString> GrantedOnce;
	TSet<FString> LeadsGiven;
	TOptional<FEventView> Event;
	TOptional<FTalkView> Talk;
	TOptional<FCombatState> Combat;
	TOptional<FRewardView> Reward;
	TOptional<FNoteView> Note;
	FString PlayedCard;
	double PlayedAt = -10.0;
	bool bBrainOnline = false;
	FString BrainModel;
	bool bSheetOpen = false, bDeckOpen = false;
	FString Prompt;
	FString ToastText;
	double ToastUntil = 0.0;
	TOptional<FIntPoint> MapSelected;
	FString CreateName = TEXT("Aldric Vane"), CreateAnc = TEXT("human"), CreateKit = TEXT("fighter");
	bool bVictory = false;
	FString EndTitle, EndText;

	/* ---------- Verbs ---------- */
	bool CanWalk() const;
	void BeginAdventure();
	void NewGame() { if (Mode == EHollowMode::Title) { Mode = EHollowMode::Create; PlaySfx(TEXT("confirm")); MarkUI(); } }
	void QuitToDesktop();
	void Restart();
	void ClickWorld(const FHitResult& Hit);
	void Interact();
	void ToggleMap();
	void ToggleSheet() { if (Mode != EHollowMode::Create) { bSheetOpen = !bSheetOpen; bDeckOpen = false; MarkUI(); } }
	void ToggleDeck() { if (Mode != EHollowMode::Create) { bDeckOpen = !bDeckOpen; bSheetOpen = false; MarkUI(); } }
	void Escape();

	void ChooseEventOption(int32 Index);
	void CloseEvent();

	void AskText(const FString& Text);
	void AskTopic(const FString& TopicId);
	void CloseTalk();
	void Service(const FString& What);

	void SelectHex(const FIntPoint& H);
	void MapAction(const FIntPoint& H);
	bool CanScout(const FIntPoint& H) const;
	bool CanTravel(const FIntPoint& H) const;
	void ScoutCost(const FIntPoint& H, int32& Gold, int32& Rations) const;
	void MakeCamp();

	void CombatStrike();
	void CombatGuard();
	void CombatPotion();
	void CombatHoly();
	void CombatFlee();
	void CombatEndTurn();
	void PlayCard(int32 HandIndex);
	void SetTarget(int32 FighterIndex);
	void CycleTarget();
	bool IsHeroTurn() const;
	TArray<int32> TurnPreview(int32 Count) const;
	int32 HeroAC() const;
	void PickReward(int32 Index);
	void CombatContinue() { if (Combat.IsSet() && Combat->bOver) FinishCombat(); }

	void CloseNote() { Note.Reset(); MarkUI(); }
	void Zoom(float Delta);
	void OnFootstep(const FVector& At);
	void PlaySfx(const FString& Name, float Volume = 1.f);
	void PlayMusic(const FString& Track, float Volume);
	void Burst(const FVector& At, const FString& Tex, const FLinearColor& Col, int32 Count, float Speed, float Life, float Size, float Up = 0.f, float Grav = 0.f, float SizeEnd = -1.f);
	void NiagaraAt(const FString& System, const FVector& At, float Scale = 1.f);
	double Now() const;
	void Toast(const FString& S);
	void Log(const FString& S);
	void MarkUI() { bUIDirty = true; }
	const FHollowHex* HexAt(const FIntPoint& H) const { return Hexes.Find(H); }
	FString Compass(const FIntPoint& H) const;
	void GainXp(int32 Xp);

private:
	/* stages */
	void GenMap();
	void GrantLead(const FString& LeadId);
	FIntPoint FindLeadTarget(const FLeadDef& L);
	TArray<FIntPoint> HexLine(const FIntPoint& A, const FIntPoint& B) const;
	void BuildBoard();
	void EnterHex(const FIntPoint& H, bool bTravel);
	void ClearLocal();
	void BuildLocal(const FIntPoint& H);
	void BuildTown();
	void BuildShrine();
	void BuildKeepArea();
	void BuildWild(const FHollowHex& Hex);
	void BuildPoi(const FHollowHex& Hex, const FVector& At);
	void Scatter(const FHollowHex& Hex, FRandomStream& Rng);
	void AddInteract(AActor* A, const FString& Kind, const FString& Id, const FString& Label, int32 Index = 0, float Radius = 280.f);
	AActor* Local(AActor* A) { if (A) LocalActors.Add(A); return A; }
	void SpawnHero(const FVector& At);
	void ShowEvent(const FString& Id);
	void ApplyOutcome(const FOutcome& O, FEventView& V);
	void ResolveEventDone();
	void OpenTalk(const FString& NpcId);
	void Reply(const FNpcDef& Npc, const FString& TopicId, bool bSetLine = true);
	void ProbeBrain(bool bLaunch);
	void BuildAtmosphere();
	void TickFx(float Dt);
	void TickAmbience(float Dt);
	void BuildRim(const FHollowHex& Hex, FRandomStream& Rng);
	void BuildFow(FHollowHex& Hex);
	void TickFow(float Dt);
	void TickTitle(float Dt);
	void LearnTopics(const FString& Text);
	FString StripBrackets(const FString& Text) const;
	void SetViewTo(AActor* Target, float Blend);
	void UpdatePrompt();
	void Die(const FString& Why);
	void Win();

	/* combat */
	void StartCombat(const FString& FoeIds, bool bAmbush, bool bWeaken);
	void BuildArena();
	void AdvanceTurn();
	void StartHeroTurn();
	void EndHeroTurn();
	void StartFoeTurn(int32 Idx);
	void EndActorTurn(int32 Idx);
	void ChooseIntent(FFighter& F);
	void FoeAction(int32 Idx, const FString& Act, int32 Step);
	int32 HeroStrike(int32 Target, int32 Bonus, int32 ExtraDice, int32 MapWeight, int32 FixedPenalty = 999);
	int32 RollD20();
	int32 HeroCheck(const FString& Skill, int32 DC, const FString& What);
	int32 HeroSave(const FString& Save, int32 DC, const FString& What);
	void HurtFoe(int32 Idx, int32 Dmg, bool bCrit);
	void HurtHero(int32 Dmg, bool bCrit, int32 FromIdx);
	void HealHero(int32 Amount, const FString& Why);
	void DrawCards(int32 N);
	bool Spend(int32 Cost);
	void CheckCombatEnd();
	void FinishCombat();
	int32 FirstLivingFoe() const;
	void CLog(const FString& S);
	FVector FighterPos(int32 Idx) const;
	void Lunge(int32 Idx, int32 TargetIdx);
	void Shake(int32 Idx);
	void FloatText(int32 Idx, const FString& Text, const FColor& Col);
	void Beat(float Delay, TFunction<void()> Fn);

	/* runtime */
	TSharedPtr<SHollowUI> UI;
	TSharedPtr<SWidget> UIHost;
	bool bUIDirty = true;
	bool bStarted = false;

	UPROPERTY() TArray<TObjectPtr<AActor>> LocalActors;
	UPROPERTY() TArray<TObjectPtr<AActor>> BoardActors;
	UPROPERTY() TArray<TObjectPtr<AActor>> ArenaActors;
	UPROPERTY() TObjectPtr<AActor> HeroToken;
	UPROPERTY() TObjectPtr<ACameraActor> MapCam;
	UPROPERTY() TObjectPtr<ACameraActor> ArenaCam;
	UPROPERTY() TObjectPtr<ACameraActor> ShowCam;
	UPROPERTY() TObjectPtr<AHollowHero> Hero;
	UPROPERTY() TObjectPtr<UMaterialInterface> ToonMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> InkMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> GrimMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> SpriteMat;
	UPROPERTY() TObjectPtr<UAudioComponent> Music;
	UPROPERTY() TObjectPtr<AActor> FxActor;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> FxMats;
	UPROPERTY() TArray<TObjectPtr<UObject>> KeepAlive;
	FString MusicTrack;

	TArray<FInteractable> Interactables;
	TMap<TObjectPtr<UPrimitiveComponent>, FIntPoint> TileOf;
	int32 NearIdx = INDEX_NONE;

	struct FBeat { float Delay = 0.f; TFunction<void()> Fn; };
	TArray<FBeat> Beats;
	float BeatClock = 0.f;

	struct FSprite { TWeakObjectPtr<UStaticMeshComponent> C; UMaterialInstanceDynamic* M = nullptr; FVector P, V; float Age = 0.f, Life = 1.f, Size0 = 10.f, Size1 = 10.f, Spin = 0.f, Rot = 0.f, Grav = 0.f; FLinearColor Col; bool bActive = false; };
	TArray<FSprite> Sprites;
	struct FFow { TWeakObjectPtr<UStaticMeshComponent> C; UMaterialInstanceDynamic* M = nullptr; FVector P; float Alpha = 1.f, Base = 1.f, Phase = 0.f; bool bFading = false; int32 Cell = 0; };
	TArray<FFow> Fow;
	float OrbitAngle = 200.f;
	float EmberClock = 0.f;
	TMap<FString, TWeakObjectPtr<UTexture2D>> TexCache;
	float MoteClock = 0.f;
	struct FAnim { TWeakObjectPtr<AActor> A; FVector From, To; float T = 0.f, Dur = 0.35f; int32 Kind = 0; };
	TArray<FAnim> Anims;

public:
	static const FVector LocalOrigin;
	static const FVector BoardOrigin;
	static const FVector ArenaOrigin;
	static constexpr float LocalRadius = 2600.f;
	static constexpr float BoardHex = 520.f;
	static constexpr int32 MapRadius = 3;
};
