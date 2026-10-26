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
class UPointLightComponent;
class UTextRenderComponent;
class UAudioComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTexture2D;
class UNiagaraComponent;
class SHollowUI;

enum class EHollowMode : uint8 { Title, Create, Explore, Map, Combat, End };
// Tabs of the pause screen (Nivalis' InGameMenuTab: Inventory, Journal, Characters, Skills...).
enum class EPauseTab : uint8 { Inventory, Character, Deck, Journal, Library, System };

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
	double LineAt = 0.0;                       // real time the current line started typing out
	bool bTyping = false;                      // the free-text box is open
	bool bLog = false;                         // showing the conversation log
	FString LastCheck;                         // "Diplomacy 14 vs DC 15 - failure" after a checked option
	bool bLastOk = true;
};

// One line in the response list. Built fresh from state so the UI and the number keys always agree.
struct FTalkChoice
{
	enum EKind : uint8 { Topic, Option, Service, Trade, Say, Log, Leave };
	EKind Kind = Topic;
	FString Id, Text;
	FString Tag;            // "Diplomacy 15 · 70%", "Give: Wolf Pelt x3", "5 silver", "Requires Shovel"
	int32 TagStyle = 0;     // 0 none, 1 skill, 2 item, 3 cost, 4 locked
	bool bLocked = false, bSeen = false;
};

struct FTradeView
{
	FString NpcId;
	FString Line;
};

// What this level grants, read from the class's own tables in the rules library (pf2e: boosts at 5/10/15/20,
// skill increases, class / skill / general / ancestry feats on the levels the class lists), and what you picked.
struct FLevelUpView
{
	int32 Level = 2;
	int32 BoostsNeeded = 0, SpellsNeeded = 0;
	bool bSkill = false, bClassFeat = false, bSkillFeat = false, bGeneralFeat = false, bAncestryFeat = false;
	TArray<FString> Boosts, NewSpells;
	FString Skill, ClassFeat, SkillFeat, GeneralFeat, AncestryFeat;
	bool IsComplete() const;
};

// A persistent exploration buff, shown as an aura on the hero and a chip on the HUD.
struct FBuff
{
	FString Id, Name, Desc;
	FLinearColor Col;
	int32 Fights = 0;       // lasts this many fights (0 = until the day ends)
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
	TOptional<FTradeView> Trade;
	TOptional<FLevelUpView> LevelUp;
	TArray<FBuff> Buffs;
	bool bPaused = false;
	// Line of sight, cover and light (Foundry's walls + vision idea, done with Unreal traces). Recomputed a few times a second.
	enum class ELight : uint8 { Dark, Dim, Bright };
	enum class EVis : uint8 { Unseen, Hidden, Concealed, Observed };
	ELight HeroLight = ELight::Dim;
	EVis HeroVis = EVis::Unseen;
	int32 HeroCover = 0;            // from the nearest watcher: 0 none, 1 lesser, 2 standard, 3 greater
	int32 Watchers = 0;             // foes that can currently see you
	bool bNoticed = false;          // a watcher has spotted you on this hex
	bool bLanternLit = true;
	void ToggleLantern();
	FString VisLabel() const;
	EPauseTab PauseTab = EPauseTab::Inventory;
	FIntPoint InvSel = FIntPoint(-1, -1);     // (bag, slot) selected in the inventory
	int32 InvFilter = -1;                      // -1 all, else EItemKind
	int32 InvSort = 0;
	FString PbId, PbStatus;                    // Pathbuilder import box on the character screen
	bool bPbBusy = false;
	FString LibCat = TEXT("Spells"), LibQuery, LibSel;   // pause-screen Library tab
	FString PlayedCard;
	double PlayedAt = -10.0;
	bool bBrainOnline = false;
	FString BrainModel;
	int32 BuffDay = 1;
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
	void ToggleSheet() { TogglePause(EPauseTab::Character); }
	void ToggleDeck() { TogglePause(EPauseTab::Deck); }
	void TogglePause(EPauseTab Tab);
	void SetPaused(bool bOn);
	void SetPauseTab(EPauseTab Tab) { PauseTab = Tab; InvSel = FIntPoint(-1, -1); MarkUI(); }

	/* inventory */
	bool Give(const FString& ItemId, int32 N = 1, bool bToast = false);   // false if some didn't fit (it drops at your feet)
	void InvClick(int32 Bag, int32 Slot);
	void InvUse();
	void InvDrop();
	void InvStow();          // move the selected stack to the other container
	void InvSortBy(int32 By);
	bool HasAbility(const FString& A) const { return PC.Inv.CanGrant(A); }

	/* trade */
	void OpenTrade();
	void CloseTrade() { Trade.Reset(); MarkUI(); }
	void Buy(const FString& ItemId);
	void Sell(const FString& ItemId);
	int32 SellPrice(const FString& ItemId) const;

	/* level up */
	void PickBoost(const FString& Abil);
	void PickSkillUp(const FString& Skill) { if (LevelUp.IsSet()) { LevelUp->Skill = Skill; MarkUI(); } }
	void PickFeat(const FString& Slot, const FString& Feat);
	void PickSpell(const FString& Spell);
	TArray<const struct FLibFeat*> FeatOptions(const FString& Slot) const;
	TArray<const struct FLibSpell*> SpellOptions() const;
	FString CasterTradition() const;
	void ConfirmLevelUp();
	int32 MaxRankAt(int32 Level) const { return Level >= 15 ? 4 : Level >= 7 ? 3 : Level >= 2 ? 2 : 1; }
	void AddBuff(const FString& Id, const FString& Name, const FString& Desc, const FLinearColor& Col, int32 Fights);
	bool HasBuff(const FString& Id) const { return Buffs.ContainsByPredicate([&](const FBuff& B) { return B.Id == Id; }); }
	void RefreshAura();

	/* talk */
	TArray<FTalkChoice> TalkChoices() const;
	void ChooseTalk(int32 Index);
	void TalkNumber(int32 Number);   // 1-9 hotkeys
	bool TalkRevealing() const;
	void SkipReveal() { if (Talk.IsSet()) { Talk->LineAt = -1000.0; MarkUI(); } }
	void SetTalkLine(const FString& Line);
	void ToggleTalkLog() { if (Talk.IsSet()) { Talk->bLog = !Talk->bLog; MarkUI(); } }
	int32 CheckChance(const FString& Skill, int32 DC) const;

	/* Pathbuilder 2e import (its official "Export JSON" build id) */
	void ImportPathbuilder(const FString& BuildId);
	void ApplyPathbuilder(const FString& Json);
	bool bImported = false;
	TSet<FString> SpentCards;   // ranked spell cards cast since your last rest
	void Rest() { SpentCards.Reset(); }
	void CastLibSpell(const FCardDef& Card, int32 Target);
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
	// Hex edges: island side k (outward yaw 60k) leads to one map neighbour. Open neighbours get a walk-out trail,
	// rumoured ones a signpost that scouts the way; the rest stay walled off by the rim.
	static int32 SideToDir(int32 Side);
	FIntPoint SideHex(int32 Side) const;
	bool SideOpen(int32 Side) const;
	void EdgeTravel(int32 Side);
	bool ScoutHex(const FIntPoint& H);
	bool TravelTo(const FIntPoint& H, int32 ExitSide = -1);

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
	void BuildEdges();
	void TickVisibility(float Dt);
	TArray<TWeakObjectPtr<AActor>> WatcherActors;
	float VisClock = 0.f, NoticeClock = 0.f;
	int32 ArriveSide = -1;
	void BuildFow(FHollowHex& Hex);
	void TickFow(float Dt);
	void TickTitle(float Dt);
	void LearnTopics(const FString& Text);
	FString StripBrackets(const FString& Text) const;
	void SetViewTo(AActor* Target, float Blend);
	void UpdatePrompt();
	void Die(const FString& Why);
	void Win();
	void OnLevelUp();
	void BuildFeatures(FHollowHex& Hex, TFunctionRef<bool(float, float, float, FVector&)> Pick);
	void UseFeature(const FInteractable& I);
	FString FeatureLabel(const FInteractable& I) const;
	void FeatureLoot(const FString& Kind, const FVector& At, FString& Msg);
	void RefreshFeatureGlow();
	void UseTalkOption(const FNpcDef& N, const FTalkOption& O);

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
	UPROPERTY() TObjectPtr<UNiagaraComponent> HeroAura;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> AuraRing;
	UPROPERTY() TObjectPtr<UPointLightComponent> AuraLight;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> AuraOrbs;
	float AuraClock = 0.f;
	TArray<TPair<TWeakObjectPtr<UStaticMeshComponent>, FString>> FeatureGlows;   // ring, ability it needs
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
