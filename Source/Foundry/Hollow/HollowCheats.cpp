// Debug console verbs for driving a playtest without the mouse: `hollow <verb> [args]`.
// Examples: hollow begin | hollow talk maren | hollow ask wolves on the ridge | hollow map | hollow go 1 -1
//           hollow event | hollow choose 0 | hollow fight wolf | hollow card 0 | hollow strike | hollow endturn
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HollowGameMode.h"
#include "HollowHero.h"

struct FHollowCheats
{
	static AHollowGameMode* Find(UWorld* World)
	{
		if (World) if (AHollowGameMode* GM = World->GetAuthGameMode<AHollowGameMode>()) return GM;
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
			if (UWorld* W = Ctx.World()) if (AHollowGameMode* GM = W->GetAuthGameMode<AHollowGameMode>()) return GM;
		return nullptr;
	}

	static void Run(const TArray<FString>& Args, UWorld* World)
	{
		AHollowGameMode* G = Find(World);
		if (!G || !Args.Num()) return;
		const FString V = Args[0].ToLower();
		auto Int = [&](int32 I, int32 Def = 0) { return Args.IsValidIndex(I) ? FCString::Atoi(*Args[I]) : Def; };
		auto Rest = [&](int32 From) { FString S; for (int32 i = From; i < Args.Num(); ++i) S += (i > From ? TEXT(" ") : TEXT("")) + Args[i]; return S; };

		if (V == TEXT("begin")) { G->NewGame(); if (Args.Num() > 1) G->CreateKit = Args[1]; if (Args.Num() > 2) G->CreateAnc = Args[2]; G->BeginAdventure(); }
		else if (V == TEXT("new")) G->NewGame();
		else if (V == TEXT("talk")) G->OpenTalk(Rest(1));
		else if (V == TEXT("ask")) G->AskText(Rest(1));
		else if (V == TEXT("topic")) G->AskTopic(Rest(1));
		else if (V == TEXT("service")) G->Service(Rest(1));
		else if (V == TEXT("close")) { G->CloseTalk(); G->CloseEvent(); G->SetPaused(false); G->CloseTrade(); }
		else if (V == TEXT("map")) G->ToggleMap();
		else if (V == TEXT("give")) { if (Args.Num() > 1) G->Give(Args[1].ToLower(), FMath::Max(1, Int(2, 1)), true); }
		else if (V == TEXT("pause")) G->TogglePause(EPauseTab::Inventory);
		else if (V == TEXT("xp")) G->GainXp(FMath::Max(1, Int(1)));
		else if (V == TEXT("reply")) G->ChooseTalk(Int(1) - 1);
		else if (V == TEXT("builder")) G->OpenBuilder();
		else if (V == TEXT("cottage") && G->Hero) G->BuildPZCottage(G->Hero->GetActorLocation() + FVector(900, 0, -85), 1);
		else if (V == TEXT("lot") && G->Hero) G->BuildPZLot(G->Hero->GetActorLocation() + FVector(0, 0, -85), Int(1));
		else if (V == TEXT("bstep")) G->BuilderStep(Int(1));
		else if (V == TEXT("bpick") && Args.Num() > 2) G->BuilderPick(Args[1], Rest(2));
		else if (V == TEXT("bfinish")) G->BuilderFinish();
		else if (V == TEXT("move")) G->CombatMove(false);
		else if (V == TEXT("cell")) G->ClickCell(FIntPoint(Int(1), Int(2)));
		else if (V == TEXT("spot"))
		{
			// jump to the Nth tool-gated spot on this hex and refresh the aura
			int32 N = Int(1);
			for (const FInteractable& I : G->Interactables)
				if (I.Kind == TEXT("feature") && N-- <= 0 && G->Hero) { G->Hero->SetActorLocation(I.Loc + FVector(-200, 0, 150)); UE_LOG(LogTemp, Display, TEXT("hollow spot: %s at %s"), *I.Id, *I.Loc.ToString()); break; }
			G->RefreshAura();
		}
		else if (V == TEXT("levelup")) { if (Args.Num() > 2) { G->PickBoost(Args[1].ToLower()); G->PickSkillUp(Args[2].ToLower()); } G->ConfirmLevelUp(); }
		else if (V == TEXT("select")) G->SelectHex(FIntPoint(Int(1), Int(2)));
		else if (V == TEXT("go")) G->MapAction(FIntPoint(Int(1), Int(2)));
		else if (V == TEXT("enter")) G->EnterHex(FIntPoint(Int(1), Int(2)), false);
		else if (V == TEXT("lead")) G->GrantLead(Rest(1));
		else if (V == TEXT("event")) { if (FHollowHex* H = G->Hexes.Find(G->Pos)) { G->ShowEvent(Args.Num() > 1 ? Args[1] : H->EventId); if (G->Event.IsSet()) G->Event->bHexEvent = Args.Num() == 1; } }
		else if (V == TEXT("choose")) G->ChooseEventOption(Int(1));
		else if (V == TEXT("fight")) G->StartCombat(Args.Num() > 1 ? Args[1] : FString(TEXT("wolf")), false, false);
		else if (V == TEXT("strike")) G->CombatStrike();
		else if (V == TEXT("guard")) G->CombatGuard();
		else if (V == TEXT("card")) G->PlayCard(Int(1));
		else if (V == TEXT("endturn")) G->CombatEndTurn();
		else if (V == TEXT("continue")) G->CombatContinue();
		else if (V == TEXT("reward")) G->PickReward(Int(1));
		else if (V == TEXT("sheet")) G->ToggleSheet();
		else if (V == TEXT("deck")) G->ToggleDeck();
		else if (V == TEXT("gold")) { G->PC.Gold += Int(1, 50); G->MarkUI(); }
		else if (V == TEXT("heal")) { G->PC.Hp = G->PC.HpMax; G->MarkUI(); }
		else if (V == TEXT("tp"))
		{
			if (APlayerController* P = G->GetWorld()->GetFirstPlayerController())
				if (APawn* Pawn = P->GetPawn()) Pawn->SetActorLocation(FVector(Int(1), Int(2), 150));
		}
	}
};

static FAutoConsoleCommandWithWorldAndArgs GHollowCommand(
	TEXT("hollow"),
	TEXT("Hollowmere playtest verbs: begin, talk <npc>, ask <text>, topic <id>, map, go <q> <r>, event, choose <i>, fight <foe>, strike, card <i>, endturn, continue, reward <i>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FHollowCheats::Run));
