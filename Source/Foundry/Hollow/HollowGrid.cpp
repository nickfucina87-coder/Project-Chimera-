// The battle grid: 5-foot hexes on the arena floor (axial coordinates, the same pointy-top layout as the March map).
// Stride and Step move along the shortest open path, melee needs reach, ranged attacks and spells use their range,
// two foes on opposite sides flank you, and boulders between attacker and target give cover.
// The arena camera orbits freely (right-drag or Shift+drag, Q/R in 60 degree steps, wheel to zoom).
#include "HollowGameMode.h"

#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "HollowArt.h"
#include "HollowLibrary.h"
#include "HollowRules.h"
#include "ProceduralMeshComponent.h"

using namespace HollowRules;
using HollowArt::EShape;

namespace
{
	const FIntPoint GDirs[6] = { { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, 0 }, { -1, 1 }, { 0, 1 } };
	constexpr float Cell = 95.f;   // centre to corner, so a hex is ~165 cm across: five feet at our scale
	constexpr int32 GridR = 7;
}

FVector AHollowGameMode::CellWorld(const FIntPoint& H) const
{
	return ArenaOrigin + FVector(-1.5f * H.Y * Cell, FMath::Sqrt(3.f) * (H.X + H.Y * 0.5f) * Cell, 0.f);
}

int32 AHollowGameMode::CellDist(const FIntPoint& A, const FIntPoint& B)
{
	return (FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y) + FMath::Abs(A.X + A.Y - B.X - B.Y)) / 2;
}

bool AHollowGameMode::CellFree(const FIntPoint& H, int32 Ignore) const
{
	if (!Combat.IsSet() || CellDist(H, FIntPoint::ZeroValue) > GridR || Combat->Blocked.Contains(H)) return false;
	for (int32 i = 0; i < Combat->F.Num(); ++i)
		if (i != Ignore && !Combat->F[i].bDead && Combat->F[i].Cell == H) return false;
	return true;
}

TArray<FIntPoint> AHollowGameMode::PathTo(int32 Who, const FIntPoint& Goal, int32 MaxSteps) const
{
	// breadth-first over open hexes; returns the steps after the start, empty if unreachable
	TArray<FIntPoint> Out;
	if (!Combat.IsSet() || !Combat->F.IsValidIndex(Who)) return Out;
	const FIntPoint Start = Combat->F[Who].Cell;
	TMap<FIntPoint, FIntPoint> From;
	TArray<FIntPoint> Q = { Start };
	From.Add(Start, Start);
	for (int32 Head = 0; Head < Q.Num(); ++Head)
	{
		const FIntPoint C = Q[Head];
		if (C == Goal) break;
		if (CellDist(C, Start) >= MaxSteps + 1) continue;
		for (const FIntPoint& D : GDirs)
		{
			const FIntPoint N = C + D;
			if (From.Contains(N) || !CellFree(N, Who)) continue;
			From.Add(N, C);
			Q.Add(N);
		}
	}
	if (!From.Contains(Goal)) return Out;
	for (FIntPoint C = Goal; C != Start; C = From[C]) Out.Insert(C, 0);
	if (Out.Num() > MaxSteps) Out.Reset();
	return Out;
}

TArray<FIntPoint> AHollowGameMode::ReachCells(int32 Who, int32 Steps) const
{
	TArray<FIntPoint> Out;
	if (!Combat.IsSet()) return Out;
	const FIntPoint S = Combat->F[Who].Cell;
	for (int32 q = -GridR; q <= GridR; ++q)
		for (int32 r = -GridR; r <= GridR; ++r)
		{
			const FIntPoint H(q, r);
			if (H == S || CellDist(H, S) > Steps || !CellFree(H, Who)) continue;
			if (PathTo(Who, H, Steps).Num()) Out.Add(H);
		}
	return Out;
}

bool AHollowGameMode::InReach(int32 A, int32 B, int32 Reach) const
{
	return Combat.IsSet() && Combat->F.IsValidIndex(A) && Combat->F.IsValidIndex(B) && CellDist(Combat->F[A].Cell, Combat->F[B].Cell) <= FMath::Max(1, Reach);
}

bool AHollowGameMode::Flanked(int32 Target) const
{
	if (!Combat.IsSet() || !Combat->F.IsValidIndex(Target)) return false;
	const FFighter& T = Combat->F[Target];
	TArray<FIntPoint> Enemies;
	for (const FFighter& F : Combat->F) if (!F.bDead && F.bHero != T.bHero && CellDist(F.Cell, T.Cell) == 1) Enemies.Add(F.Cell);
	for (int32 i = 0; i < Enemies.Num(); ++i)
		for (int32 j = i + 1; j < Enemies.Num(); ++j)
			if (Enemies[i] + Enemies[j] == T.Cell * 2) return true;   // directly opposite each other
	return false;
}

int32 AHollowGameMode::CoverBetween(int32 A, int32 B) const
{
	if (!Combat.IsSet()) return 0;
	const FIntPoint Pa = Combat->F[A].Cell, Pb = Combat->F[B].Cell;
	const int32 N = CellDist(Pa, Pb);
	if (N <= 1) return 0;
	int32 Cover = 0;
	for (int32 i = 1; i < N; ++i)
	{
		// sample the straight line in cube coordinates and round to the nearest hex
		const float T = (float)i / N;
		const float X = FMath::Lerp((float)Pa.X, (float)Pb.X, T), Z = FMath::Lerp((float)Pa.Y, (float)Pb.Y, T), Y = -X - Z;
		int32 Rx = FMath::RoundToInt(X), Rz = FMath::RoundToInt(Z), Ry = FMath::RoundToInt(Y);
		const float Dx = FMath::Abs(Rx - X), Dy = FMath::Abs(Ry - Y), Dz = FMath::Abs(Rz - Z);
		if (Dx > Dy && Dx > Dz) Rx = -Ry - Rz; else if (Dy <= Dz) Rz = -Rx - Ry;
		const FIntPoint H(Rx, Rz);
		if (Combat->Blocked.Contains(H)) Cover = FMath::Max(Cover, 2);
		for (int32 k = 0; k < Combat->F.Num(); ++k)
			if (k != A && k != B && !Combat->F[k].bDead && Combat->F[k].Cell == H) Cover = FMath::Max(Cover, 1);
	}
	return Cover;
}

void AHollowGameMode::BuildBattleGrid()
{
	BattleTiles.Reset();
	UWorld* W = GetWorld();
	FRandomStream Rng(Pos.X * 31 + Pos.Y * 977 + Day);
	// a few boulders to hide behind, away from where everyone starts
	for (int32 i = 0; i < 6; ++i)
	{
		const FIntPoint H(Rng.RandRange(-4, 4), Rng.RandRange(-2, 2));
		bool bStart = false;
		for (const FFighter& F : Combat->F) bStart |= CellDist(F.Cell, H) <= 1;
		if (bStart || CellDist(H, FIntPoint::ZeroValue) > GridR - 1) continue;
		Combat->Blocked.Add(H);
		ArenaActors.Add(HollowArt::Boulders(W, CellWorld(H), 0.55f, Rng.RandRange(0, 99999)));
	}
	AActor* G = HollowArt::NewActor(W, ArenaOrigin);
	ArenaActors.Add(G);
	for (int32 q = -GridR; q <= GridR; ++q)
		for (int32 r = -GridR; r <= GridR; ++r)
		{
			const FIntPoint H(q, r);
			if (CellDist(H, FIntPoint::ZeroValue) > GridR) continue;
			// a faint outline and a clickable top; reachable hexes light up in move mode
			UProceduralMeshComponent* T = HollowArt::HexPrism(G, Cell * 0.93f, 4.f, FLinearColor(0.05f, 0.05f, 0.07f), true, CellWorld(H) - ArenaOrigin + FVector(0, 0, 6.f));
			if (T) { T->SetRenderCustomDepth(false); BattleTiles.Add(T, H); }
		}
	RefreshGridHighlight();
}

void AHollowGameMode::RefreshGridHighlight()
{
	for (AActor* A : GridMarks) if (IsValid(A)) A->Destroy();
	GridMarks.Reset();
	if (!Combat.IsSet()) return;
	UWorld* W = GetWorld();
	auto Mark = [&](const FIntPoint& H, const FLinearColor& Col)
	{
		AActor* A = HollowArt::NewActor(W, CellWorld(H) + FVector(0, 0, 12.f));
		HollowArt::GlowPart(A, EShape::Cylinder, FVector::ZeroVector, FVector(Cell * 1.5f, Cell * 1.5f, 1.f), Col);
		GridMarks.Add(A);
	};
	if (Combat->bMoveMode)
		for (const FIntPoint& H : Combat->Reachable) Mark(H, FLinearColor(0.25f, 0.5f, 0.9f) * 0.25f);
	// your hex, and the target's hex
	Mark(Combat->F[0].Cell, FLinearColor(0.9f, 0.75f, 0.3f) * 0.2f);
	if (Combat->F.IsValidIndex(Combat->Target) && !Combat->F[Combat->Target].bDead) Mark(Combat->F[Combat->Target].Cell, FLinearColor(0.9f, 0.25f, 0.2f) * 0.2f);
}

void AHollowGameMode::MoveFighter(int32 Who, const TArray<FIntPoint>& Path)
{
	if (!Combat.IsSet() || !Path.Num()) return;
	FFighter& F = Combat->F[Who];
	AActor* A = F.Puppet.Get();
	const FVector From = A ? A->GetActorLocation() : F.Home;
	F.Cell = Path.Last();
	F.Home = CellWorld(F.Cell);
	if (A)
	{
		FAnim M; M.A = A; M.From = From; M.To = F.Home; M.Dur = 0.18f * Path.Num(); M.Kind = 4;
		Anims.Add(M);
		// face the way you're going
		A->SetActorRotation(FRotator(0, (F.Home - From).Rotation().Yaw, 0));
	}
	PlaySfx(TEXT("step"), 0.4f);
}

void AHollowGameMode::CombatMove(bool bStep)
{
	if (!IsHeroTurn()) return;
	Combat->bMoveMode = !(Combat->bMoveMode && Combat->bStepMode == bStep);
	Combat->bStepMode = bStep;
	Combat->Reachable = Combat->bMoveMode ? ReachCells(0, bStep ? 1 : FMath::Max(1, PC.Speed / 5)) : TArray<FIntPoint>();
	RefreshGridHighlight();
	MarkUI();
}

void AHollowGameMode::ClickCell(const FIntPoint& H)
{
	if (!Combat.IsSet() || !IsHeroTurn() || !Combat->bMoveMode) return;
	if (!Combat->Reachable.Contains(H)) { Toast(TEXT("You can't get there this action.")); return; }
	const TArray<FIntPoint> Path = PathTo(0, H, Combat->bStepMode ? 1 : FMath::Max(1, PC.Speed / 5));
	if (!Path.Num() || !Spend(1)) return;
	MoveFighter(0, Path);
	CLog(FString::Printf(TEXT("You %s %d feet."), Combat->bStepMode ? TEXT("Step") : TEXT("Stride"), Path.Num() * 5));
	Combat->bMoveMode = false;
	Combat->Reachable.Reset();
	RefreshGridHighlight();
	if (Combat->Actions == 0) Beat(0.6f, [this] { CombatEndTurn(); });
	MarkUI();
}

// Before an attack: is the target close enough? Ranged weapons and spells use range increments in hexes.
bool AHollowGameMode::CanReach(int32 Target, int32 RangeFeet)
{
	if (!Combat.IsSet() || !Combat->F.IsValidIndex(Target)) return false;
	const int32 RangeHexes = RangeFeet > 0 ? FMath::Max(1, RangeFeet / 5) : 1;
	if (InReach(0, Target, RangeHexes)) return true;
	Toast(FString::Printf(TEXT("%s is %d feet away. %s"), *Combat->F[Target].Name, CellDist(Combat->F[0].Cell, Combat->F[Target].Cell) * 5,
		RangeFeet > 0 ? TEXT("Out of range.") : TEXT("Stride closer first.")));
	return false;
}

int32 AHollowGameMode::HeroWeaponRange() const
{
	const FLibItem* W = HollowLib::Item(PC.WeaponName);
	return W ? W->Range : 0;
}

/* ---------- Arena camera ---------- */

void AHollowGameMode::OrbitArena(float Deg) { ArenaYawTarget += Deg; }
void AHollowGameMode::ZoomArena(float Delta) { ArenaDist = FMath::Clamp(ArenaDist + Delta, 900.f, 3600.f); }

void AHollowGameMode::TickArenaCam(float Dt)
{
	if (Mode != EHollowMode::Combat || !ArenaCam) return;
	ArenaYaw = FMath::FInterpTo(ArenaYaw, ArenaYawTarget, Dt, 6.f);
	ArenaDistNow = FMath::FInterpTo(ArenaDistNow, ArenaDist, Dt, 6.f);
	const float Yaw = FMath::DegreesToRadians(ArenaYaw), Pitch = FMath::DegreesToRadians(FMath::GetMappedRangeValueClamped(FVector2D(900.f, 3600.f), FVector2D(24.f, 55.f), ArenaDistNow));
	const FVector Focus = ArenaOrigin + FVector(0, 0, 60.f);
	const FVector At = Focus + FVector(FMath::Cos(Yaw) * FMath::Cos(Pitch), FMath::Sin(Yaw) * FMath::Cos(Pitch), FMath::Sin(Pitch)) * ArenaDistNow;
	ArenaCam->SetActorLocationAndRotation(At, (Focus - At).Rotation());
}
