// The march board: map generation, conversation-driven route leads, scouting/travel costs and the 3D hex board.
#include "HollowGameMode.h"

#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "HollowArt.h"
#include "HollowHero.h"
#include "HollowRules.h"
#include "ProceduralMeshComponent.h"

using namespace HollowRules;

namespace
{
	const FIntPoint Dirs[6] = { { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, 0 }, { -1, 1 }, { 0, 1 } };
	int32 HDist(const FIntPoint& A, const FIntPoint& B)
	{
		return (FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y) + FMath::Abs(A.X + A.Y - B.X - B.Y)) / 2;
	}
	// axial (q, r) to world: screen-up is +X, screen-right is +Y
	FVector HexWorld(const FIntPoint& H, float Size)
	{
		return FVector(-1.5f * H.Y * Size, FMath::Sqrt(3.f) * (H.X + H.Y * 0.5f) * Size, 0.f);
	}
}

FString AHollowGameMode::Compass(const FIntPoint& H) const
{
	const float X = FMath::Sqrt(3.f) * (H.X + H.Y / 2.f), Y = 1.5f * H.Y;
	const float A = FMath::Atan2(-Y, X);
	static const TCHAR* Names[8] = { TEXT("east"), TEXT("north-east"), TEXT("north"), TEXT("north-west"), TEXT("west"), TEXT("south-west"), TEXT("south"), TEXT("south-east") };
	return Names[((FMath::RoundToInt(A / (PI / 4)) % 8) + 8) % 8];
}

void AHollowGameMode::GenMap()
{
	Hexes.Reset();
	FRandomStream Rng(FMath::Rand());
	const TArray<ETerr> Pool = { ETerr::Forest, ETerr::Plains, ETerr::Hills, ETerr::Swamp, ETerr::Ruins, ETerr::River };
	float Total = 0.f;
	for (ETerr T : Pool) Total += TerrInfo(T).Weight;
	auto PickT = [&]
	{
		float X = Rng.FRand() * Total;
		for (ETerr T : Pool) { X -= TerrInfo(T).Weight; if (X <= 0.f) return T; }
		return ETerr::Plains;
	};
	for (int32 Q = -MapRadius; Q <= MapRadius; ++Q)
		for (int32 R = -MapRadius; R <= MapRadius; ++R)
		{
			if (FMath::Abs(Q + R) > MapRadius) continue;
			FHollowHex H; H.Q = Q; H.R = R; H.T = PickT();
			Hexes.Add(FIntPoint(Q, R), H);
		}
	// clump regions: copy a neighbour's terrain sometimes
	for (auto& P : Hexes)
		if (Rng.FRand() < 0.45f)
			if (const FHollowHex* N = Hexes.Find(P.Key + Dirs[Rng.RandRange(0, 5)])) P.Value.T = N->T;

	auto Ring = [&](int32 N) { TArray<FIntPoint> L; for (auto& P : Hexes) if (HDist(P.Key, FIntPoint(0, 0)) == N) L.Add(P.Key); return L; };
	TArray<FIntPoint> R3 = Ring(3);
	const FIntPoint Keep = R3[Rng.RandRange(0, R3.Num() - 1)];
	Hexes[Keep].T = ETerr::Keep;
	TArray<FIntPoint> R2 = Ring(2).FilterByPredicate([&](const FIntPoint& H) { return HDist(H, Keep) >= 3; });
	Hexes[R2[Rng.RandRange(0, R2.Num() - 1)]].T = ETerr::Shrine;
	for (const FIntPoint& D : Dirs)
		if (FHollowHex* N = Hexes.Find(Keep + D)) if (N->T != ETerr::Shrine && Rng.FRand() < 0.6f) N->T = ETerr::Ruins;
	FHollowHex& Town = Hexes[FIntPoint(0, 0)];
	Town.T = ETerr::Town; Town.State = EHexState::Explored; Town.bEventDone = true;

	TSet<FString> Used;
	for (auto& P : Hexes)
	{
		FHollowHex& H = P.Value;
		if (H.T == ETerr::Town) continue;
		if (H.T == ETerr::Shrine) { H.EventId = TEXT("shrine"); continue; }
		if (H.T == ETerr::Keep) { H.EventId = TEXT("keep"); continue; }
		TArray<const FEventDef*> Cand;
		for (const FEventDef& E : HollowData::Events())
			if (E.Terr.Contains(H.T) && !Used.Contains(E.Id)) Cand.Add(&E);
		if (Cand.Num()) { H.EventId = Cand[Rng.RandRange(0, Cand.Num() - 1)]->Id; Used.Add(H.EventId); }
		else H.EventId = TEXT("quiet");
		H.ChestTaken.Init(false, Rng.RandRange(1, 2));
	}
	Pos = FIntPoint(0, 0);
}

TArray<FIntPoint> AHollowGameMode::HexLine(const FIntPoint& A, const FIntPoint& B) const
{
	TArray<FIntPoint> Out;
	const int32 N = HDist(A, B);
	for (int32 i = 0; i <= N; ++i)
	{
		const float T = N == 0 ? 0.f : (float)i / N;
		// cube lerp with a tiny nudge so ties round consistently
		const float X = FMath::Lerp((float)A.X, (float)B.X, T) + 1e-4f;
		const float Z = FMath::Lerp((float)A.Y, (float)B.Y, T) + 1e-4f;
		const float Y = -X - Z;
		int32 RX = FMath::RoundToInt(X), RY = FMath::RoundToInt(Y), RZ = FMath::RoundToInt(Z);
		const float DX = FMath::Abs(RX - X), DY = FMath::Abs(RY - Y), DZ = FMath::Abs(RZ - Z);
		if (DX > DY && DX > DZ) RX = -RY - RZ;
		else if (DY <= DZ) RZ = -RX - RY;
		Out.AddUnique(FIntPoint(RX, RZ));
	}
	return Out;
}

FIntPoint AHollowGameMode::FindLeadTarget(const FLeadDef& L)
{
	for (ETerr Pref : L.Prefer)
		if (Pref == ETerr::Shrine || Pref == ETerr::Keep)
			for (auto& P : Hexes) if (P.Value.T == Pref) return P.Key;

	const FVector Want = HexWorld(Dirs[L.Dir], 1.f).GetSafeNormal();
	FIntPoint Best = FIntPoint(0, 0);
	float BestScore = -1e9f;
	for (auto& P : Hexes)
	{
		const FHollowHex& H = P.Value;
		if (H.T == ETerr::Town || H.T == ETerr::Keep || H.T == ETerr::Shrine || !H.LeadName.IsEmpty()) continue;
		const int32 Dist = HDist(P.Key, FIntPoint(0, 0));
		if (Dist < 2) continue;
		float Score = FVector::DotProduct(HexWorld(P.Key, 1.f).GetSafeNormal(), Want) * 2.f;
		Score -= FMath::Abs(Dist - L.Ring) * 0.6f;
		if (L.Prefer.Contains(H.T)) Score += 0.5f;
		if (Score > BestScore) { BestScore = Score; Best = P.Key; }
	}
	FHollowHex& T = Hexes[Best];
	if (L.Prefer.Num() && !L.Prefer.Contains(T.T))
	{
		// the rumour says it's hills, so it is: retune terrain and pick a fitting event
		T.T = L.Prefer[0];
		for (const FEventDef& E : HollowData::Events())
			if (E.Terr.Contains(T.T)) { T.EventId = E.Id; break; }
	}
	if (L.Id == TEXT("barrow")) T.EventId = TEXT("barrow");
	if (L.Id == TEXT("road")) T.EventId = TEXT("toll");
	if (L.Id == TEXT("fen")) T.EventId = TEXT("wisps");
	if (L.Id == TEXT("ridge")) T.EventId = TEXT("ridge");
	return Best;
}

void AHollowGameMode::GrantLead(const FString& LeadId)
{
	const FLeadDef* L = HollowData::FindLead(LeadId);
	if (!L) return;
	LeadsGiven.Add(LeadId);
	const FIntPoint Target = FindLeadTarget(*L);
	Hexes[Target].LeadName = L->Name;
	for (const FIntPoint& H : HexLine(FIntPoint(0, 0), Target))
		if (FHollowHex* X = Hexes.Find(H)) if (X->State == EHexState::Hidden) X->State = EHexState::Rumored;
	Log(FString::Printf(TEXT("Lead: %s, to the %s."), *L->Name, *Compass(Target)));
	Toast(FString::Printf(TEXT("New route: %s (%s). Press M to open the map."), *L->Name, *Compass(Target)));
	if (Mode == EHollowMode::Map) BuildBoard();
}

bool AHollowGameMode::CanScout(const FIntPoint& H) const
{
	const FHollowHex* X = Hexes.Find(H);
	if (!X || X->State != EHexState::Rumored) return false;
	for (const FIntPoint& D : Dirs)
		if (const FHollowHex* N = Hexes.Find(H + D)) if (N->IsOpen()) return true;
	return false;
}

bool AHollowGameMode::CanTravel(const FIntPoint& H) const
{
	const FHollowHex* X = Hexes.Find(H);
	return X && X->IsOpen() && HDist(H, Pos) == 1;
}

void AHollowGameMode::ScoutCost(const FIntPoint& H, int32& Gold, int32& Rations) const
{
	const FHollowHex* X = Hexes.Find(H);
	Gold = 3; Rations = 1;
	if (!X) return;
	if (X->T == ETerr::Hills || X->T == ETerr::Swamp || X->T == ETerr::Ruins) Gold += 2;
	if (X->T == ETerr::Keep) Gold = 12;
}

void AHollowGameMode::SelectHex(const FIntPoint& H)
{
	MapSelected = H;
	MarkUI();
}

void AHollowGameMode::MapAction(const FIntPoint& H)
{
	if (Mode != EHollowMode::Map) return;
	FHollowHex* X = Hexes.Find(H);
	if (!X) return;
	if (H == Pos) { ToggleMap(); return; }
	if (CanScout(H)) { if (ScoutHex(H)) { Toast(TEXT("Route scouted. Click it again to travel.")); BuildBoard(); } return; }
	if (TravelTo(H)) return;
	if (X->State == EHexState::Rumored) Toast(TEXT("Scout the hexes nearer to you on this route first."));
	else if (X->State == EHexState::Hidden) Toast(TEXT("Nobody has told you about this place. Talk to people."));
	else Toast(TEXT("Too far. Travel one hex at a time."));
}

bool AHollowGameMode::ScoutHex(const FIntPoint& H)
{
	FHollowHex* X = Hexes.Find(H);
	if (!X || !CanScout(H)) return false;
	int32 G, Ra;
	ScoutCost(H, G, Ra);
	if (PC.Gold < G || PC.Count(TEXT("ration")) < Ra) { Toast(FString::Printf(TEXT("Scouting needs %d silver and %d ration."), G, Ra)); return false; }
	PC.Gold -= G; PC.Inv.Take(TEXT("ration"), Ra);
	X->State = EHexState::Open;
	Log(FString::Printf(TEXT("Scouted a way into %s (%s)."), *TerrInfo(X->T).Name, *Compass(H)));
	MarkUI();
	return true;
}

bool AHollowGameMode::TravelTo(const FIntPoint& H, int32 ExitSide)
{
	if (!CanTravel(H)) return false;
	{
		Day++;
		if (PC.Inv.Has(TEXT("ration"))) PC.Inv.Take(TEXT("ration"));
		else
		{
			const int32 Dm = R(TEXT("1d6"));
			PC.Hp = FMath::Max(0, PC.Hp - Dm);
			Toast(FString::Printf(TEXT("No rations. Hunger costs you %d HP."), Dm));
			if (PC.Hp <= 0) { Die(TEXT("You starved on the road.")); return true; }
		}
		MapSelected.Reset();
		// walking out through a side: arrive on the far side of the next hex, still facing the way you were going
		ArriveSide = ExitSide >= 0 ? (ExitSide + 3) % 6 : -1;
		EnterHex(H, true);
		if (Hero && ExitSide >= 0) Hero->SetViewYaw(60.f * ExitSide);
		if (Hero) NiagaraAt(TEXT("/Game/NiagaraExamples/FX_Player/NS_Player_Teleport_In"), Hero->GetActorLocation() - FVector(0, 0, 85.f), 1.f);
		return true;
	}
}

int32 AHollowGameMode::SideToDir(int32 Side)
{
	// board neighbours lie at yaw 90, 30, -30, -90, -150, 150 (Dirs order); island sides face 60k, 30 degrees round
	static const int32 Map[6] = { 1, 0, 5, 4, 3, 2 };
	return Map[((Side % 6) + 6) % 6];
}

FIntPoint AHollowGameMode::SideHex(int32 Side) const { return Pos + Dirs[SideToDir(Side)]; }

bool AHollowGameMode::SideOpen(int32 Side) const
{
	const FHollowHex* N = Hexes.Find(SideHex(Side));
	return N && N->State >= EHexState::Rumored;
}

void AHollowGameMode::EdgeTravel(int32 Side)
{
	const FIntPoint H = SideHex(Side);
	const FHollowHex* N = Hexes.Find(H);
	if (!N) return;
	if (N->State == EHexState::Rumored)
	{
		if (!CanScout(H)) { Toast(TEXT("You have heard of a way through here, but not enough to follow it.")); return; }
		if (ScoutHex(H)) { Toast(FString::Printf(TEXT("You find the trail %s. Walk on to follow it."), *Compass(H - Pos))); ClearLocal(); BuildLocal(Pos); RefreshAura(); }
		return;
	}
	if (Hero) Hero->StopMove();
	TravelTo(H, Side);
}

void AHollowGameMode::BuildEdges()
{
	UWorld* W = GetWorld();
	const float Ap = LocalRadius * 0.866f;
	for (int32 k = 0; k < 6; ++k)
	{
		if (!SideOpen(k)) continue;
		const FHollowHex& N = Hexes.FindChecked(SideHex(k));
		const float Yaw = 60.f * k;
		const FVector Out(FMath::Cos(FMath::DegreesToRadians(Yaw)), FMath::Sin(FMath::DegreesToRadians(Yaw)), 0.f);
		const FVector Side(Out.Y, -Out.X, 0.f);
		const FVector At = LocalOrigin + Out * Ap * 0.86f;
		const bool bScouted = N.IsOpen();
		Local(HollowArt::Signpost(W, At + Side * 170.f, Yaw));
		if (bScouted) Local(HollowArt::Lantern(W, At - Side * 170.f));
		AActor* M = Local(HollowArt::Marker(W, At, bScouted ? FLinearColor(1.f, 0.85f, 0.45f) : FLinearColor(0.6f, 0.6f, 0.75f)));
		int32 G = 0, Ra = 0;
		ScoutCost(SideHex(k), G, Ra);
		const FString Label = bScouted
			? FString::Printf(TEXT("Trail %s to %s (a day's walk, 1 ration)"), *Compass(SideHex(k) - Pos), *TerrInfo(N.T).Name)
			: FString::Printf(TEXT("Faint path %s: scout it (%d silver, %d ration)"), *Compass(SideHex(k) - Pos), G, Ra);
		AddInteract(M, TEXT("edge"), FString::FromInt(k), Label, k, 320.f);
	}
}

void AHollowGameMode::MakeCamp()
{
	if (Mode != EHollowMode::Map && Mode != EHollowMode::Explore) return;
	const FHollowHex& H = Hexes.FindChecked(Pos);
	if (H.T == ETerr::Town) { Toast(TEXT("You're in town. Tobb has beds for five silver.")); return; }
	if (!PC.Inv.Has(TEXT("ration"))) { Toast(TEXT("You need a ration to make camp.")); return; }
	PC.Inv.Take(TEXT("ration")); Day++; Rest();
	const int32 Heal = FMath::Min(PC.HpMax - PC.Hp, PC.HpMax / 2);
	PC.Hp += Heal;
	Log(FString::Printf(TEXT("Made camp. +%d HP."), Heal));
	Toast(FString::Printf(TEXT("You camp under the stars. +%d HP."), Heal));
	if (FMath::FRand() < 0.25f)
	{
		if (Mode == EHollowMode::Map) ToggleMap();
		ShowEvent(TEXT("ambush_wolf"));
	}
	MarkUI();
}

void AHollowGameMode::BuildBoard()
{
	for (AActor* A : BoardActors) if (IsValid(A)) A->Destroy();
	BoardActors.Reset();
	TileOf.Reset();
	UWorld* W = GetWorld();
	HollowArt::GNoLights = true;
	{
		AActor* Lamp = HollowArt::NewActor(W, BoardOrigin + FVector(-800, 0, 2600));
		HollowArt::GNoLights = false;
		HollowArt::Light(Lamp, FVector::ZeroVector, FLinearColor(0.75f, 0.7f, 1.f), 900.f, 9000.f, true);
		HollowArt::GNoLights = true;
		BoardActors.Add(Lamp);
	}

	auto Label = [&](AActor* A, const FString& Text, float Z, const FColor& Col, float Size)
	{
		UTextRenderComponent* T = NewObject<UTextRenderComponent>(A);
		T->SetupAttachment(A->GetRootComponent());
		T->SetAbsolute(false, true, false);
		T->SetWorldRotation(FRotator(60.f, 180.f, 0.f));
		T->SetRelativeLocation(FVector(0, 0, Z));
		T->SetHorizontalAlignment(EHTA_Center);
		T->SetVerticalAlignment(EVRTA_TextCenter);
		T->SetWorldSize(Size);
		T->SetTextRenderColor(Col);
		T->SetText(FText::FromString(Text));
		T->RegisterComponent();
		A->AddInstanceComponent(T);
	};

	for (auto& P : Hexes)
	{
		const FHollowHex& H = P.Value;
		const FTerrInfo& TI = TerrInfo(H.T);
		const bool bOpen = H.IsOpen();
		const bool bReach = Mode == EHollowMode::Map && (CanTravel(P.Key) || CanScout(P.Key));
		const float Height = H.State == EHexState::Hidden ? 40.f : (H.T == ETerr::Hills ? 110.f : H.T == ETerr::Keep ? 130.f : 70.f);
		FLinearColor Col = TI.Fill;
		if (H.State == EHexState::Hidden) Col = FLinearColor(0.07f, 0.065f, 0.09f);
		else if (H.State == EHexState::Rumored) Col = FMath::Lerp(TI.Fill, FLinearColor(0.35f, 0.33f, 0.38f), 0.6f);
		else if (H.State == EHexState::Open) Col = TI.Fill * 0.8f;

		const FVector At = BoardOrigin + HexWorld(P.Key, BoardHex) + FVector(0, 0, Height);
		AActor* A = HollowArt::NewActor(W, At, 30.f);
		BoardActors.Add(A);
		UProceduralMeshComponent* Tile = HollowArt::HexPrism(A, BoardHex * 0.95f, Height + 60.f, Col, true, FVector::ZeroVector,
			bReach ? HollowArt::GlowMat(Col) : nullptr);
		TileOf.Add(Tile, P.Key);

		if (bOpen || H.T == ETerr::Town)
		{
			auto Mini = [&](AActor* Prop, float Scale) { if (Prop) { Prop->SetActorScale3D(FVector(Scale)); BoardActors.Add(Prop); } };
			FRandomStream Rng(P.Key.X * 31 + P.Key.Y * 131);
			auto Spot = [&](float R) { const float Ang = Rng.FRandRange(0.f, 2 * PI); return At + FVector(FMath::Cos(Ang) * R, FMath::Sin(Ang) * R, 0); };
			switch (H.T)
			{
			case ETerr::Forest: for (int32 i = 0; i < 5; ++i) Mini(HollowArt::PineTree(W, Spot(Rng.FRandRange(80.f, 300.f)), 1.f, FLinearColor(0.2f, 0.45f, 0.2f)), 0.55f); break;
			case ETerr::Hills: Mini(HollowArt::Mound(W, Spot(120.f), 0.5f, FLinearColor(0.55f, 0.48f, 0.3f)), 1.f); Mini(HollowArt::Rock(W, Spot(250.f), 1.2f, 0.f), 1.f); break;
			case ETerr::Swamp: Mini(HollowArt::Pool(W, Spot(100.f), 140.f, FLinearColor(0.15f, 0.25f, 0.22f)), 1.f); Mini(HollowArt::Reeds(W, Spot(250.f)), 1.f); Mini(HollowArt::Reeds(W, Spot(250.f)), 1.f); break;
			case ETerr::Ruins: for (int32 i = 0; i < 3; ++i) Mini(HollowArt::Pillar(W, Spot(Rng.FRandRange(100.f, 280.f)), 200.f, i == 1), 0.6f); break;
			case ETerr::River: { AActor* Rv = HollowArt::NewActor(W, At, Rng.FRandRange(0.f, 180.f)); HollowArt::Part(Rv, HollowArt::EShape::Cube, FVector(0, 0, 2), FVector(BoardHex * 1.7f, 120, 4), FLinearColor(0.25f, 0.5f, 0.85f)); BoardActors.Add(Rv); break; }
			case ETerr::Plains: Mini(HollowArt::Haystack(W, Spot(150.f)), 0.8f); Mini(HollowArt::RoundTree(W, Spot(260.f), 1.f, FLinearColor(0.4f, 0.6f, 0.22f)), 0.6f); break;
			case ETerr::Town: Mini(HollowArt::House(W, At + FVector(120, -110, 0), 180.f, FLinearColor(0.9f, 0.8f, 0.6f), FLinearColor(0.6f, 0.2f, 0.15f), FVector(320, 280, 230)), 0.5f);
				Mini(HollowArt::House(W, At + FVector(100, 140, 0), 180.f, FLinearColor(0.95f, 0.93f, 0.88f), FLinearColor(0.9f, 0.68f, 0.2f), FVector(300, 260, 260)), 0.5f);
				Mini(HollowArt::House(W, At + FVector(-170, 0, 0), 180.f, FLinearColor(0.8f, 0.65f, 0.5f), FLinearColor(0.25f, 0.35f, 0.6f), FVector(300, 260, 220)), 0.45f); break;
			case ETerr::Keep: Mini(HollowArt::Keep(W, At, 180.f), 0.28f); break;
			case ETerr::Shrine: Mini(HollowArt::Shrine(W, At), 0.5f); break;
			}
		}
		if (H.State == EHexState::Rumored)
		{
			Label(A, TEXT("?"), 90.f, FColor(255, 225, 140), 320.f);
			int32 G, Ra; ScoutCost(P.Key, G, Ra);
			Label(A, FString::Printf(TEXT("scout %dg %dr"), G, Ra), -60.f, FColor(255, 245, 225), 95.f);
		}
		if (!H.LeadName.IsEmpty() && H.State != EHexState::Hidden)
			Label(A, H.LeadName, 260.f, FColor(255, 235, 170), 110.f);
		if (H.T == ETerr::Town) Label(A, TEXT("Brindlemark"), 300.f, FColor(255, 235, 170), 120.f);
	}

	// hero token
	const FHollowHex& Here = Hexes.FindChecked(Pos);
	const float HereH = Here.T == ETerr::Hills ? 110.f : Here.T == ETerr::Keep ? 130.f : 70.f;
	AActor* Tok = HollowArt::NewActor(W, BoardOrigin + HexWorld(Pos, BoardHex) + FVector(-150.f, 0, HereH), 0.f);
	HollowArt::Chibi(Tok, nullptr, FVector::ZeroVector, PC.Body, PC.Skin, PC.Hat, PC.HatStyle, 1.4f);
	HollowArt::GlowPart(Tok, HollowArt::EShape::Cylinder, FVector(0, 0, 3), FVector(170, 170, 6), FLinearColor(1.f, 0.8f, 0.3f));
	BoardActors.Add(Tok);
	HollowArt::GNoLights = false;
}
