#include "HollowArt.h"

#include "Animation/AnimSequence.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

namespace HollowArt
{

bool GNoLights = false;

namespace
{
	TWeakObjectPtr<UMaterialInterface> GBase;
	TWeakObjectPtr<UMaterialInterface> GGrim;
	TMap<uint32, TWeakObjectPtr<UMaterialInstanceDynamic>> GCache;
	TMap<FString, TWeakObjectPtr<UMaterialInstanceDynamic>> GGrimCache;
	TMap<FString, TWeakObjectPtr<UStaticMesh>> GMeshCache;
	TSet<FString> GMissing;

	UStaticMesh* Mesh(EShape S)
	{
		static TWeakObjectPtr<UStaticMesh> Cache[4];
		static const TCHAR* Paths[4] = {
			TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("/Engine/BasicShapes/Sphere.Sphere"),
			TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), TEXT("/Engine/BasicShapes/Cone.Cone") };
		const int32 I = (int32)S;
		if (!Cache[I].IsValid()) Cache[I] = LoadObject<UStaticMesh>(nullptr, Paths[I]);
		return Cache[I].Get();
	}

	const FLinearColor Bark(0.13f, 0.1f, 0.085f);
	const FLinearColor Wood(0.2f, 0.12f, 0.07f);
	const FLinearColor DarkWood(0.1f, 0.065f, 0.04f);
	const FLinearColor Stone(0.28f, 0.27f, 0.29f);
	const FLinearColor DarkStone(0.14f, 0.135f, 0.15f);
	const FLinearColor Moss(0.16f, 0.19f, 0.07f);
	const FLinearColor Lamp(1.0f, 0.62f, 0.28f);
	const FLinearColor Eldritch(0.55f, 0.25f, 1.0f);
	const FLinearColor Sickly(0.35f, 1.0f, 0.55f);
	const FLinearColor Black(0.01f, 0.01f, 0.015f);

	UMaterialInstanceDynamic* GrimFor(const FString& Kit)
	{
		if (TWeakObjectPtr<UMaterialInstanceDynamic>* F = GGrimCache.Find(Kit)) if (F->IsValid()) return F->Get();
		if (!GGrim.IsValid()) return nullptr;
		UTexture* Tex = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/Hollow/Kenney/%s/colormap.colormap"), *Kit));
		if (!Tex) return nullptr;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(GGrim.Get(), GetTransientPackage());
		M->SetTextureParameterValue(TEXT("Tex"), Tex);
		if (Kit == TEXT("Graveyard")) { M->SetScalarParameterValue(TEXT("Desat"), 0.4f); M->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.62f, 0.6f, 0.66f)); }
		else if (Kit == TEXT("Castle")) { M->SetScalarParameterValue(TEXT("Desat"), 0.6f); M->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.42f, 0.4f, 0.47f)); }
		else { M->SetScalarParameterValue(TEXT("Desat"), 0.62f); M->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.55f, 0.5f, 0.5f)); }
		GGrimCache.Add(Kit, M);
		return M;
	}
}

void Init(UMaterialInterface* Base, UMaterialInterface* Grim)
{
	GBase = Base;
	GGrim = Grim;
	GCache.Reset();
	GGrimCache.Reset();
	GMissing.Reset();
}

UMaterialInstanceDynamic* Mat(const FLinearColor& C)
{
	const uint32 Key = C.ToFColor(false).DWColor();
	if (TWeakObjectPtr<UMaterialInstanceDynamic>* Found = GCache.Find(Key))
		if (Found->IsValid()) return Found->Get();
	if (!GBase.IsValid()) return nullptr;
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(GBase.Get(), GetTransientPackage());
	M->SetVectorParameterValue(TEXT("Color"), C);
	GCache.Add(Key, M);
	return M;
}

UMaterialInstanceDynamic* GlowMat(const FLinearColor& C)
{
	if (!GBase.IsValid()) return nullptr;
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(GBase.Get(), GetTransientPackage());
	M->SetVectorParameterValue(TEXT("Color"), C);
	M->SetScalarParameterValue(TEXT("Glow"), 1.f);
	return M;
}

UMaterialInstanceDynamic* EmitMat(const FLinearColor& C, float Emit)
{
	if (!GBase.IsValid()) return nullptr;
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(GBase.Get(), GetTransientPackage());
	M->SetVectorParameterValue(TEXT("Color"), C);
	M->SetScalarParameterValue(TEXT("Emit"), Emit);
	return M;
}

AActor* NewActor(UWorld* W, const FVector& Loc, float Yaw)
{
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* A = W->SpawnActor<AActor>(AActor::StaticClass(), FTransform(FRotator(0, Yaw, 0), Loc), P);
	USceneComponent* Root = NewObject<USceneComponent>(A, TEXT("Root"));
	A->SetRootComponent(Root);
	Root->RegisterComponent();
	A->SetActorLocationAndRotation(Loc, FRotator(0, Yaw, 0));
	return A;
}

static UStaticMeshComponent* MakePart(AActor* A, UStaticMesh* SM, const FVector& Loc, const FVector& Scale, UMaterialInterface* M,
	const FRotator& Rot, bool bCollide, USceneComponent* Parent)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(A);
	C->SetStaticMesh(SM);
	C->SetupAttachment(Parent ? Parent : A->GetRootComponent());
	C->SetRelativeLocationAndRotation(Loc, Rot);
	C->SetRelativeScale3D(Scale);
	if (M) for (int32 i = 0; i < FMath::Max(1, C->GetNumMaterials()); ++i) C->SetMaterial(i, M);
	C->SetCastShadow(true);
	C->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
	C->RegisterComponent();
	A->AddInstanceComponent(C);
	return C;
}

UStaticMeshComponent* Part(AActor* A, EShape S, const FVector& Loc, const FVector& Size, const FLinearColor& C,
	const FRotator& Rot, bool bCollide, USceneComponent* Parent)
{
	return MakePart(A, Mesh(S), Loc, Size / 100.f, Mat(C), Rot, bCollide, Parent);
}

UStaticMeshComponent* GlowPart(AActor* A, EShape S, const FVector& Loc, const FVector& Size, const FLinearColor& C,
	const FRotator& Rot, USceneComponent* Parent)
{
	UStaticMeshComponent* P = MakePart(A, Mesh(S), Loc, Size / 100.f, GlowMat(C), Rot, false, Parent);
	P->SetCastShadow(false);
	return P;
}

UStaticMeshComponent* Collider(AActor* A, const FVector& Loc, const FVector& Size, float Yaw, bool bRound)
{
	UStaticMeshComponent* C = MakePart(A, Mesh(bRound ? EShape::Cylinder : EShape::Cube), Loc, Size / 100.f, nullptr, FRotator(0, Yaw, 0), true, nullptr);
	C->SetVisibility(false);
	C->SetCastShadow(false);
	return C;
}

UPointLightComponent* Light(AActor* A, const FVector& Loc, const FLinearColor& C, float Candela, float Radius, bool bShadows, USceneComponent* Parent)
{
	if (GNoLights) return nullptr;
	UPointLightComponent* L = NewObject<UPointLightComponent>(A);
	L->SetupAttachment(Parent ? Parent : A->GetRootComponent());
	L->SetRelativeLocation(Loc);
	L->SetIntensityUnits(ELightUnits::Candelas);
	L->SetIntensity(Candela);
	L->SetAttenuationRadius(Radius);
	L->SetLightColor(C);
	L->SetSourceRadius(6.f);
	L->SetCastShadows(bShadows);
	L->SetVolumetricScatteringIntensity(0.6f);
	L->RegisterComponent();
	A->AddInstanceComponent(L);
	return L;
}

UProceduralMeshComponent* HexPrism(AActor* A, float Radius, float Height, const FLinearColor& C, bool bCollide, const FVector& Loc,
	UMaterialInstanceDynamic* MatOverride)
{
	UProceduralMeshComponent* P = NewObject<UProceduralMeshComponent>(A);
	P->SetupAttachment(A->GetRootComponent());
	P->SetRelativeLocation(Loc);
	P->bUseComplexAsSimpleCollision = true;

	TArray<FVector> V; TArray<int32> T; TArray<FVector> N; TArray<FVector2D> UV;
	TArray<FColor> Col; TArray<FProcMeshTangent> Tan;
	auto Corner = [&](int32 i, float Z)
	{
		const float Ang = FMath::DegreesToRadians(60.f * i + 30.f);
		return FVector(FMath::Cos(Ang) * Radius, FMath::Sin(Ang) * Radius, Z);
	};
	V.Add(FVector(0, 0, 0)); N.Add(FVector::UpVector);
	for (int32 i = 0; i < 6; ++i) { V.Add(Corner(i, 0)); N.Add(FVector::UpVector); }
	for (int32 i = 0; i < 6; ++i) { T.Add(0); T.Add(1 + (i + 1) % 6); T.Add(1 + i); }
	for (int32 i = 0; i < 6; ++i)
	{
		const FVector A0 = Corner(i, 0), A1 = Corner((i + 1) % 6, 0);
		const FVector B0 = Corner(i, -Height), B1 = Corner((i + 1) % 6, -Height);
		const FVector Nn = ((A0 + A1) * 0.5f).GetSafeNormal2D();
		const int32 Base = V.Num();
		V.Append({ A0, A1, B1, B0 });
		N.Append({ Nn, Nn, Nn, Nn });
		T.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
	}
	UV.SetNumZeroed(V.Num());
	P->CreateMeshSection(0, V, T, N, UV, Col, Tan, bCollide);
	P->SetMaterial(0, MatOverride ? MatOverride : Mat(C));
	P->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
	P->RegisterComponent();
	A->AddInstanceComponent(P);
	return P;
}

/* ---------- Kenney meshes ---------- */

UStaticMesh* KMesh(const FString& Path)
{
	if (GMissing.Contains(Path)) return nullptr;
	if (TWeakObjectPtr<UStaticMesh>* F = GMeshCache.Find(Path)) if (F->IsValid()) return F->Get();
	FString Kit, Name;
	Path.Split(TEXT("/"), &Kit, &Name);
	UStaticMesh* SM = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Hollow/Kenney/%s/%s.%s"), *Kit, *Name, *Name));
	if (!SM) { GMissing.Add(Path); return nullptr; }
	GMeshCache.Add(Path, SM);
	return SM;
}

UStaticMeshComponent* KPart(AActor* A, const FString& Path, const FVector& Loc, float Yaw, float Scale, USceneComponent* Parent, const FVector& NonUniform)
{
	UStaticMesh* SM = KMesh(Path);
	if (!SM) return nullptr;
	FString Kit, Name;
	Path.Split(TEXT("/"), &Kit, &Name);
	UMaterialInterface* M = Kit == TEXT("Nature") ? nullptr : GrimFor(Kit);
	return MakePart(A, SM, Loc, NonUniform * Scale, M, FRotator(0, Yaw, 0), false, Parent);
}

USkeletalMeshComponent* KChar(AActor* A, USceneComponent* Parent, const FString& Name, const FVector& Loc, float Yaw, float Height)
{
	const FString Base = FString::Printf(TEXT("/Game/Hollow/Kenney/Graveyard/character-%s"), *Name);
	USkeletalMesh* SK = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("%s.character-%s"), *Base, *Name));
	if (!SK) return nullptr;
	USkeletalMeshComponent* C = NewObject<USkeletalMeshComponent>(A);
	C->SetSkeletalMesh(SK);
	C->SetupAttachment(Parent ? Parent : A->GetRootComponent());
	const FBoxSphereBounds B = SK->GetBounds();
	const float H = FMath::Max(1.f, B.BoxExtent.Z * 2.f);
	const float S = Height / H;
	const float Bottom = (B.Origin.Z - B.BoxExtent.Z) * S;
	C->SetRelativeLocationAndRotation(Loc - FVector(0, 0, Bottom), FRotator(0, Yaw - 90.f, 0));
	C->SetRelativeScale3D(FVector(S));
	C->SetCollisionProfileName(TEXT("NoCollision"));
	C->RegisterComponent();
	A->AddInstanceComponent(C);
	if (UAnimSequence* Anim = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("%s_Anim.character-%s_Anim"), *Base, *Name)))
		C->PlayAnimation(Anim, true);
	return C;
}

/* ---------- Scenery ---------- */

AActor* PineTree(UWorld* W, const FVector& L, float S, const FLinearColor& Leaf)
{
	AActor* A = NewActor(W, L, FMath::FRandRange(0.f, 360.f));
	static const TCHAR* Kinds[] = { TEXT("Graveyard/pine-crooked"), TEXT("Graveyard/pine"), TEXT("Graveyard/pine-fall-crooked"), TEXT("Graveyard/pine-crooked") };
	if (!KPart(A, Kinds[FMath::RandRange(0, 3)], FVector::ZeroVector, 0.f, 2.4f * S))
	{
		Part(A, EShape::Cylinder, FVector(0, 0, 40 * S), FVector(30, 30, 80) * S, Bark);
		Part(A, EShape::Cone, FVector(0, 0, 150 * S), FVector(180, 180, 200) * S, Leaf);
	}
	Collider(A, FVector(0, 0, 100), FVector(50, 50, 200) * S, 0.f, true);
	return A;
}

AActor* RoundTree(UWorld* W, const FVector& L, float S, const FLinearColor& Leaf)
{
	AActor* A = NewActor(W, L, FMath::FRandRange(0.f, 360.f));
	static const TCHAR* Kinds[] = { TEXT("Nature/tree_default_dark"), TEXT("Nature/tree_oak_dark"), TEXT("Nature/tree_detailed_dark"), TEXT("Nature/tree_simple_dark") };
	if (!KPart(A, Kinds[FMath::RandRange(0, 3)], FVector::ZeroVector, 0.f, 3.0f * S))
	{
		Part(A, EShape::Cylinder, FVector(0, 0, 60 * S), FVector(34, 34, 120) * S, Bark);
		Part(A, EShape::Sphere, FVector(0, 0, 175 * S), FVector(170, 170, 150) * S, Leaf);
	}
	Collider(A, FVector(0, 0, 100), FVector(50, 50, 200) * S, 0.f, true);
	return A;
}

// Gnarled, leafless tree grown from recursive cylinders, like the concept art's dead oaks.
AActor* DeadTree(UWorld* W, const FVector& L, float S, int32 Seed)
{
	AActor* A = NewActor(W, L, 0.f);
	FRandomStream R(Seed);
	TFunction<void(const FVector&, const FVector&, float, float, int32)> Grow = [&](const FVector& From, const FVector& Dir, float Len, float Rad, int32 Depth)
	{
		const FVector To = From + Dir * Len;
		const FRotator Rot = FRotationMatrix::MakeFromZ(Dir).Rotator();
		Part(A, EShape::Cylinder, (From + To) * 0.5f, FVector(Rad * 2.f, Rad * 2.f, Len * 1.04f), Bark * R.FRandRange(0.85f, 1.15f), Rot);
		if (Depth <= 0) return;
		const int32 Kids = R.RandRange(2, 3);
		for (int32 k = 0; k < Kids; ++k)
		{
			FVector D = Dir.RotateAngleAxis(R.FRandRange(25.f, 50.f), FVector::CrossProduct(Dir, FVector(R.FRandRange(-1.f, 1.f), R.FRandRange(-1.f, 1.f), 0.f)).GetSafeNormal());
			D = (D + FVector(0, 0, 0.15f)).GetSafeNormal();
			Grow(To, D, Len * R.FRandRange(0.6f, 0.78f), Rad * 0.62f, Depth - 1);
		}
	};
	Grow(FVector::ZeroVector, FVector(R.FRandRange(-0.15f, 0.15f), R.FRandRange(-0.15f, 0.15f), 1.f).GetSafeNormal(), 190.f * S, 22.f * S, 4);
	for (int32 i = 0; i < 5; ++i)
	{
		const float Ang = i * 72.f + R.FRandRange(-20.f, 20.f);
		const FVector D = FVector(FMath::Cos(FMath::DegreesToRadians(Ang)), FMath::Sin(FMath::DegreesToRadians(Ang)), -0.35f).GetSafeNormal();
		Part(A, EShape::Cylinder, D * 55.f * S + FVector(0, 0, 18 * S), FVector(14, 14, 130) * S, Bark * 0.9f, FRotationMatrix::MakeFromZ(D).Rotator());
	}
	Collider(A, FVector(0, 0, 100), FVector(60, 60, 200) * S, 0.f, true);
	return A;
}

AActor* AutumnTree(UWorld* W, const FVector& L, float S)
{
	AActor* A = NewActor(W, L, FMath::FRandRange(0.f, 360.f));
	static const TCHAR* Kinds[] = { TEXT("Nature/tree_default_fall"), TEXT("Nature/tree_oak_fall"), TEXT("Nature/tree_detailed_fall"), TEXT("Nature/tree_cone_fall"), TEXT("Nature/tree_tall_fall") };
	if (!KPart(A, Kinds[FMath::RandRange(0, 4)], FVector::ZeroVector, 0.f, 3.0f * S))
	{
		Part(A, EShape::Cylinder, FVector(0, 0, 60 * S), FVector(30, 30, 120) * S, Bark);
		Part(A, EShape::Sphere, FVector(0, 0, 170 * S), FVector(160, 160, 150) * S, FLinearColor(0.4f, 0.14f, 0.04f));
	}
	Collider(A, FVector(0, 0, 100), FVector(50, 50, 200) * S, 0.f, true);
	return A;
}

// A raised rocky ridge for the far edge of a hex, stacked boulders and cliff blocks topped with pines.
AActor* Ridge(UWorld* W, const FVector& L, float Yaw, float S, int32 Seed)
{
	AActor* A = NewActor(W, L, Yaw);
	FRandomStream R(Seed);
	for (int32 i = 0; i < 7; ++i)
	{
		const FVector P(R.FRandRange(-200.f, 200.f) * S, (i - 3) * 190.f * S, 0);
		const float H = R.FRandRange(1.f, 1.8f);
		if (UStaticMeshComponent* Cl = KPart(A, R.FRand() < 0.5f ? TEXT("Nature/cliff_block_rock") : TEXT("Nature/cliff_large_rock"), P, R.FRandRange(0.f, 360.f), 3.2f * S, nullptr, FVector(1.f, 1.f, H)))
			for (int32 m = 0; m < Cl->GetNumMaterials(); ++m) Cl->SetMaterial(m, Mat(Stone * R.FRandRange(0.5f, 0.8f)));
		else
			Part(A, EShape::Cube, P + FVector(0, 0, 160 * H * S), FVector(300, 220, 320 * H) * S, Stone, FRotator(0, R.FRandRange(0.f, 90.f), 0));
		Part(A, EShape::Sphere, P + FVector(0, 0, 330.f * H * S), FVector(240, 200, 60) * S, Moss * R.FRandRange(0.8f, 1.2f));
	}
	Collider(A, FVector(0, 0, 250 * S), FVector(500, 1400, 500) * S);
	return A;
}

AActor* Waterfall(UWorld* W, const FVector& Top, const FVector& Bottom, float Width)
{
	AActor* A = NewActor(W, Bottom);
	const FVector D = Top - Bottom;
	const FLinearColor Foam(0.55f, 0.62f, 0.7f);
	UStaticMeshComponent* Fall = Part(A, EShape::Cube, D * 0.5f, FVector(30, Width, D.Size()), Foam, FRotationMatrix::MakeFromZ(D.GetSafeNormal()).Rotator());
	Fall->SetMaterial(0, GlowMat(Foam * 0.45f));
	for (int32 i = 0; i < 6; ++i)
		GlowPart(A, EShape::Sphere, FVector(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-Width * 0.6f, Width * 0.6f), 20), FVector(110, 90, 50), Foam * 0.6f);
	Light(A, FVector(80, 0, 120), FLinearColor(0.5f, 0.6f, 0.75f), 8.f, 600.f);
	return A;
}

// A mossy boulder cluster for the hex rim.
AActor* Boulders(UWorld* W, const FVector& L, float S, int32 Seed)
{
	AActor* A = NewActor(W, L, 0.f);
	FRandomStream R(Seed);
	static const TCHAR* Rocks[] = { TEXT("Nature/rock_largeA"), TEXT("Nature/rock_largeB"), TEXT("Nature/rock_largeC"), TEXT("Nature/rock_largeD"),
		TEXT("Nature/rock_tallA"), TEXT("Nature/rock_tallB"), TEXT("Nature/rock_tallE"), TEXT("Graveyard/rocks-tall"), TEXT("Graveyard/rocks") };
	const int32 N = R.RandRange(3, 6);
	for (int32 i = 0; i < N; ++i)
	{
		const FVector P(R.FRandRange(-120.f, 120.f) * S, R.FRandRange(-120.f, 120.f) * S, 0);
		const float Sc = R.FRandRange(2.2f, 4.2f) * S;
		UStaticMeshComponent* C = KPart(A, Rocks[R.RandRange(0, 8)], P, R.FRandRange(0.f, 360.f), Sc);
		if (!C) C = Part(A, EShape::Sphere, P + FVector(0, 0, 40 * Sc), FVector(110, 90, 80) * Sc, Stone);
		else for (int32 m = 0; m < C->GetNumMaterials(); ++m) C->SetMaterial(m, Mat(Stone * R.FRandRange(0.55f, 0.85f)));
		if (R.FRand() < 0.5f)
			Part(A, EShape::Sphere, P + FVector(R.FRandRange(-20.f, 20.f), R.FRandRange(-20.f, 20.f), 38.f * Sc), FVector(55, 45, 12) * Sc, Moss * R.FRandRange(0.5f, 0.8f));
	}
	Collider(A, FVector(0, 0, 80), FVector(260, 260, 160) * S, 0.f, true);
	return A;
}

AActor* Rock(UWorld* W, const FVector& L, float S, float Yaw)
{
	AActor* A = NewActor(W, L, Yaw);
	static const TCHAR* Rocks[] = { TEXT("Nature/rock_largeA"), TEXT("Nature/rock_largeE"), TEXT("Nature/rock_largeF"), TEXT("Graveyard/rocks") };
	if (UStaticMeshComponent* C = KPart(A, Rocks[FMath::RandRange(0, 3)], FVector::ZeroVector, 0.f, 2.6f * S))
	{
		for (int32 m = 0; m < C->GetNumMaterials(); ++m) C->SetMaterial(m, Mat(Stone * FMath::FRandRange(0.55f, 0.85f)));
	}
	else Part(A, EShape::Sphere, FVector(0, 0, 20 * S), FVector(120, 90, 70) * S, Stone, FRotator(8, 0, 12));
	Collider(A, FVector(0, 0, 40), FVector(150, 150, 80) * S, 0.f, true);
	return A;
}

AActor* Tuft(UWorld* W, const FVector& L, const FLinearColor& C)
{
	AActor* A = NewActor(W, L, FMath::FRandRange(0.f, 360.f));
	for (int32 i = 0; i < 4; ++i)
		Part(A, EShape::Cone, FVector(FMath::FRandRange(-14.f, 14.f), FMath::FRandRange(-14.f, 14.f), 18), FVector(8, 8, 44), C * FMath::FRandRange(0.8f, 1.2f),
			FRotator(FMath::FRandRange(-22.f, 22.f), 0, FMath::FRandRange(-22.f, 22.f)));
	return A;
}

AActor* Shrub(UWorld* W, const FVector& L, float S)
{
	AActor* A = NewActor(W, L, FMath::FRandRange(0.f, 360.f));
	const FLinearColor Rust = FMath::FRand() < 0.6f ? FLinearColor(0.32f, 0.08f, 0.03f) : FLinearColor(0.34f, 0.22f, 0.07f);
	for (int32 i = 0; i < 7; ++i)
		Part(A, EShape::Cone, FVector(FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-30.f, 30.f), 30) * S, FVector(18, 18, 70) * S, Rust * FMath::FRandRange(0.8f, 1.3f),
			FRotator(FMath::FRandRange(-30.f, 30.f), 0, FMath::FRandRange(-30.f, 30.f)));
	return A;
}

AActor* House(UWorld* W, const FVector& L, float Yaw, const FLinearColor& Wall, const FLinearColor& Roof, const FVector& Size, bool bSign, bool bStone)
{
	AActor* A = NewActor(W, L, Yaw);
	const float S = 1.9f, Cell = 100.f * S;
	const int32 D = FMath::Max(2, FMath::RoundToInt(Size.X / Cell)), Wd = FMath::Max(2, FMath::RoundToInt(Size.Y / Cell));
	const int32 Floors = Size.Z > 300.f ? 2 : 1;
	const float X = D * Cell, Y = Wd * Cell, Z = Floors * Cell;
	bool bKenney = KMesh(TEXT("Town/wall-wood")) != nullptr;
	if (bKenney)
	{
		const FString Plain = bStone ? TEXT("Town/wall") : TEXT("Town/wall-wood");
		const FString Win = bStone ? TEXT("Town/wall-window-stone") : TEXT("Town/wall-wood-window-shutters");
		const FString Small = bStone ? TEXT("Town/wall-window-stone") : TEXT("Town/wall-wood-window-small");
		const FString Door = bStone ? TEXT("Town/wall-door") : TEXT("Town/wall-wood-door");
		for (int32 f = 0; f < Floors; ++f)
			for (int32 j = 0; j < D; ++j)
				for (int32 i = 0; i < Wd; ++i)
				{
					const FVector C((j - (D - 1) * 0.5f) * Cell, (i - (Wd - 1) * 0.5f) * Cell, f * Cell);
					auto Piece = [&](float SideYaw, bool bFront)
					{
						FString P = ((i + j + f) % 2) ? Win : Plain;
						if (f > 0) P = ((i + j) % 2) ? Small : Plain;
						if (bFront && f == 0 && i == Wd / 2) P = Door;
						KPart(A, P, C, SideYaw, S);
					};
					if (j == D - 1) Piece(0.f, true);
					if (j == 0) Piece(180.f, false);
					if (i == Wd - 1) Piece(90.f, false);
					if (i == 0) Piece(-90.f, false);
				}
		// floor + ceiling so the interior light stays inside
		Part(A, EShape::Cube, FVector(0, 0, Z - 6), FVector(X, Y, 12), DarkWood);
	}
	else Part(A, EShape::Cube, FVector(0, 0, Z * 0.5f), FVector(X, Y, Z), Wall);
	const float R = Y * 0.74f;
	Part(A, EShape::Cube, FVector(0, 0, Z), FVector(X * 1.12f, R, R), Roof, FRotator(0, 0, 45));
	if (!KPart(A, TEXT("Town/chimney"), FVector(-X * 0.25f, Y * 0.2f, Z + R * 0.2f), 0.f, S))
		Part(A, EShape::Cube, FVector(-X * 0.25f, Y * 0.25f, Z + R * 0.45f), FVector(34, 34, 90), Stone * 0.8f);
	Light(A, FVector(0, 0, 150), Lamp, 18.f, Cell * 2.4f);
	Light(A, FVector(X * 0.5f + 60, 0, 230), Lamp, 10.f, 520.f);
	GlowPart(A, EShape::Sphere, FVector(X * 0.5f + 40, Cell * 0.45f, 240), FVector(22, 22, 22), Lamp);
	if (bSign)
	{
		Part(A, EShape::Cylinder, FVector(X * 0.5f + 30, Y * 0.5f + 10, Z * 0.75f), FVector(8, 8, 60), DarkWood, FRotator(0, 0, 90));
		Part(A, EShape::Cube, FVector(X * 0.5f + 30, Y * 0.5f + 40, Z * 0.62f), FVector(8, 50, 34), Lamp * 0.5f);
	}
	Collider(A, FVector(0, 0, Z * 0.5f), FVector(X, Y, Z));
	return A;
}

AActor* Well(UWorld* W, const FVector& L)
{
	AActor* A = NewActor(W, L);
	if (KPart(A, TEXT("Town/fountain-round-detail"), FVector::ZeroVector, 0.f, 1.8f))
		Part(A, EShape::Cylinder, FVector(0, 0, 40), FVector(300, 300, 4), FLinearColor(0.02f, 0.04f, 0.05f));
	else
	{
		Part(A, EShape::Cylinder, FVector(0, 0, 35), FVector(140, 140, 70), Stone);
		Part(A, EShape::Cylinder, FVector(0, 0, 71), FVector(110, 110, 4), FLinearColor(0.02f, 0.05f, 0.08f));
	}
	Collider(A, FVector(0, 0, 50), FVector(330, 330, 100), 0.f, true);
	return A;
}

AActor* Lantern(UWorld* W, const FVector& L)
{
	AActor* A = NewActor(W, L, FMath::FRandRange(0.f, 360.f));
	const float S = 2.2f;
	if (KPart(A, TEXT("Graveyard/lightpost-single"), FVector::ZeroVector, 0.f, S))
	{
		GlowPart(A, EShape::Sphere, FVector(0, 26 * S, 100 * S), FVector(18, 18, 22), Lamp);
		Light(A, FVector(0, 26 * S, 98 * S), Lamp, 14.f, 750.f);
	}
	else
	{
		Part(A, EShape::Cylinder, FVector(0, 0, 90), FVector(10, 10, 180), DarkWood);
		GlowPart(A, EShape::Sphere, FVector(0, 0, 190), FVector(30, 30, 30), Lamp);
		Light(A, FVector(0, 0, 190), Lamp, 14.f, 750.f);
	}
	Collider(A, FVector(0, 0, 100), FVector(30, 30, 200), 0.f, true);
	return A;
}

AActor* Fence(UWorld* W, const FVector& From, const FVector& To)
{
	const FVector Dlt = To - From;
	const float Len = Dlt.Size2D();
	AActor* A = NewActor(W, (From + To) * 0.5f, Dlt.Rotation().Yaw);
	const float S = 2.f, Piece = 100.f * S;
	const int32 N = FMath::Max(1, FMath::RoundToInt(Len / Piece));
	if (KMesh(TEXT("Graveyard/iron-fence")))
	{
		for (int32 i = 0; i < N; ++i)
			KPart(A, FMath::FRand() < 0.2f ? TEXT("Graveyard/iron-fence-damaged") : TEXT("Graveyard/iron-fence"),
				FVector(-Len * 0.5f + (i + 0.5f) * Len / N, 65.f, 0), 0.f, S, nullptr, FVector(Len / N / Piece, 1.f, 1.f));
	}
	else
	{
		Part(A, EShape::Cube, FVector(0, 0, 50), FVector(Len, 8, 10), Wood);
		Part(A, EShape::Cube, FVector(0, 0, 25), FVector(Len, 8, 10), Wood);
	}
	Collider(A, FVector(0, 0, 60), FVector(Len, 20, 120));
	return A;
}

AActor* Stall(UWorld* W, const FVector& L, float Yaw, const FLinearColor& Awning)
{
	AActor* A = NewActor(W, L, Yaw);
	if (!KPart(A, Awning.R > Awning.B ? TEXT("Town/stall-red") : TEXT("Town/stall-green"), FVector::ZeroVector, 90.f, 1.9f))
	{
		Part(A, EShape::Cube, FVector(0, 0, 40), FVector(80, 160, 80), Wood);
		Part(A, EShape::Cube, FVector(10, 0, 200), FVector(110, 190, 12), Awning, FRotator(-12, 0, 0));
	}
	GlowPart(A, EShape::Sphere, FVector(40, 60, 150), FVector(14, 14, 18), Lamp);
	Light(A, FVector(60, 0, 160), Lamp, 6.f, 420.f);
	Collider(A, FVector(0, 0, 60), FVector(190, 190, 120));
	return A;
}

AActor* Haystack(UWorld* W, const FVector& L)
{
	AActor* A = NewActor(W, L, FMath::FRandRange(0.f, 360.f));
	if (!KPart(A, FMath::FRand() < 0.5f ? TEXT("Graveyard/hay-bale") : TEXT("Graveyard/hay-bale-bundled"), FVector::ZeroVector, 0.f, 2.4f))
		Part(A, EShape::Sphere, FVector(0, 0, 40), FVector(130, 130, 110), FLinearColor(0.35f, 0.28f, 0.1f));
	Collider(A, FVector(0, 0, 50), FVector(150, 150, 100), 0.f, true);
	return A;
}

AActor* Reeds(UWorld* W, const FVector& L)
{
	AActor* A = NewActor(W, L, FMath::FRandRange(0.f, 360.f));
	for (int32 i = 0; i < 6; ++i)
	{
		const FVector P(FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(-25.f, 25.f), 0);
		const float H = FMath::FRandRange(70.f, 130.f);
		Part(A, EShape::Cylinder, P + FVector(0, 0, H * 0.5f), FVector(4, 4, H), FLinearColor(0.16f, 0.2f, 0.08f));
		Part(A, EShape::Sphere, P + FVector(0, 0, H), FVector(8, 8, 20), FLinearColor(0.15f, 0.08f, 0.04f));
	}
	return A;
}

AActor* Pool(UWorld* W, const FVector& L, float Radius, const FLinearColor& C)
{
	AActor* A = NewActor(W, L);
	UStaticMeshComponent* P = Part(A, EShape::Cylinder, FVector(0, 0, 1), FVector(Radius * 2, Radius * 2 * 0.8f, 4), C);
	P->SetMaterial(0, EmitMat(C, 0.08f));
	return A;
}

AActor* Pillar(UWorld* W, const FVector& L, float H, bool bBroken)
{
	AActor* A = NewActor(W, L, FMath::FRandRange(0.f, 360.f));
	const float S = H / 113.f;
	if (KPart(A, TEXT("Graveyard/column-large"), FVector::ZeroVector, 0.f, 2.2f, nullptr, FVector(1.f, 1.f, (bBroken ? 0.55f : 1.f) * S / 2.2f)))
	{
		if (bBroken) for (int32 i = 0; i < 3; ++i)
			Part(A, EShape::Cube, FVector(FMath::FRandRange(-90.f, 90.f), FMath::FRandRange(-90.f, 90.f), 12), FVector(40, 30, 24), Stone, FRotator(FMath::FRandRange(0.f, 40.f), FMath::FRandRange(0.f, 90.f), 0));
	}
	else
	{
		Part(A, EShape::Cube, FVector(0, 0, 10), FVector(70, 70, 20), Stone * 0.9f);
		Part(A, EShape::Cylinder, FVector(0, 0, H * 0.5f + 20), FVector(44, 44, bBroken ? H * 0.55f : H), Stone);
	}
	Collider(A, FVector(0, 0, H * 0.5f), FVector(80, 80, H), 0.f, true);
	return A;
}

AActor* BrokenWall(UWorld* W, const FVector& L, float Yaw, float Len)
{
	AActor* A = NewActor(W, L, Yaw);
	const float S = 2.1f, Piece = 100.f * S;
	const int32 N = FMath::Max(1, FMath::RoundToInt(Len / Piece));
	for (int32 i = 0; i < N; ++i)
	{
		const FVector P(-Len * 0.5f + (i + 0.5f) * Len / N, 0, 0);
		if (!KPart(A, FMath::FRand() < 0.5f ? TEXT("Graveyard/stone-wall-damaged") : TEXT("Graveyard/stone-wall"), P, 0.f, S))
			Part(A, EShape::Cube, P + FVector(0, 0, 60), FVector(Piece - 4, 40, FMath::FRandRange(40.f, 160.f)), Stone);
	}
	Collider(A, FVector(0, 0, 70), FVector(Len, 50, 140));
	return A;
}

AActor* RuinArch(UWorld* W, const FVector& L, float Yaw, bool bFire)
{
	AActor* A = NewActor(W, L, Yaw);
	for (int32 s : { -1, 1 })
	{
		Part(A, EShape::Cube, FVector(0, s * 120.f, 160), FVector(70, 70, 320), Stone * 0.9f);
		for (int32 k = 0; k < 3; ++k)
			Part(A, EShape::Cube, FVector(0, s * (200.f + k * 70.f), 70 + (k % 2) * 40.f), FVector(60, 66, 140 - k * 30.f), Stone * FMath::FRandRange(0.75f, 1.f));
	}
	for (int32 k = 0; k < 7; ++k)
	{
		const float Ang = FMath::DegreesToRadians(180.f * k / 6.f);
		Part(A, EShape::Cube, FVector(0, FMath::Cos(Ang) * 120.f, 320 + FMath::Sin(Ang) * 90.f), FVector(70, 46, 40), Stone, FRotator(0, 0, -FMath::RadiansToDegrees(Ang) + 90.f));
	}
	Part(A, EShape::Cube, FVector(0, 0, 5), FVector(260, 360, 10), DarkStone);
	if (bFire)
	{
		if (!KPart(A, TEXT("Graveyard/fire-basket"), FVector(-60, 0, 0), 0.f, 2.f))
			Part(A, EShape::Cylinder, FVector(-60, 0, 20), FVector(60, 60, 40), DarkStone);
		GlowPart(A, EShape::Cone, FVector(-60, 0, 70), FVector(40, 40, 70), FLinearColor(1.f, 0.4f, 0.08f));
		GlowPart(A, EShape::Cone, FVector(-60, 0, 78), FVector(22, 22, 50), FLinearColor(1.f, 0.8f, 0.3f));
		Light(A, FVector(-60, 0, 110), FLinearColor(1.f, 0.45f, 0.15f), 40.f, 900.f, true);
	}
	Collider(A, FVector(0, 120, 160), FVector(70, 70, 320));
	Collider(A, FVector(0, -120, 160), FVector(70, 70, 320));
	return A;
}

AActor* RitualCircle(UWorld* W, const FVector& L, float Radius, const FLinearColor& Glow)
{
	AActor* A = NewActor(W, L);
	Part(A, EShape::Cylinder, FVector(0, 0, 3), FVector(Radius * 2.f, Radius * 2.f, 6), DarkStone * 1.2f);
	for (float Rr : { 0.85f, 0.6f, 0.3f })
	{
		UStaticMeshComponent* Ring = Part(A, EShape::Cylinder, FVector(0, 0, 6.5f), FVector(Radius * 2.f * Rr, Radius * 2.f * Rr, 1), Glow);
		Ring->SetMaterial(0, EmitMat(Glow * 0.4f, 0.6f));
		Part(A, EShape::Cylinder, FVector(0, 0, 7.f), FVector(Radius * 2.f * Rr - 16.f, Radius * 2.f * Rr - 16.f, 1), DarkStone * 1.2f);
	}
	for (int32 i = 0; i < 8; ++i)
	{
		const float Ang = FMath::DegreesToRadians(i * 45.f);
		const FVector P(FMath::Cos(Ang) * Radius * 1.05f, FMath::Sin(Ang) * Radius * 1.05f, 0);
		if (!KPart(A, i % 3 == 0 ? TEXT("Graveyard/pillar-obelisk") : TEXT("Graveyard/pillar-square"), P, i * 45.f, 2.2f))
			Part(A, EShape::Cube, P + FVector(0, 0, 100), FVector(50, 50, 200), Stone);
		Collider(A, P + FVector(0, 0, 100), FVector(60, 60, 200));
	}
	GlowPart(A, EShape::Sphere, FVector(0, 0, 160), FVector(40, 40, 40), Glow);
	Light(A, FVector(0, 0, 160), Glow, 25.f, Radius * 2.5f);
	return A;
}

AActor* Wagon(UWorld* W, const FVector& L, float Yaw)
{
	AActor* A = NewActor(W, L, Yaw);
	if (UStaticMeshComponent* C = KPart(A, TEXT("Town/cart"), FVector::ZeroVector, 0.f, 2.6f))
		C->SetRelativeRotation(FRotator(0, 0, 9));
	else Part(A, EShape::Cube, FVector(0, 0, 50), FVector(120, 220, 60), Wood, FRotator(0, 0, 9));
	if (!KPart(A, TEXT("Town/wheel"), FVector(90, 160, 30), 30.f, 2.4f))
		Part(A, EShape::Cylinder, FVector(90, 160, 10), FVector(90, 90, 12), DarkWood);
	Collider(A, FVector(0, 0, 60), FVector(170, 340, 120));
	return A;
}

AActor* Bones(UWorld* W, const FVector& L, float Yaw)
{
	AActor* A = NewActor(W, L, Yaw);
	const FLinearColor Bone(0.55f, 0.52f, 0.45f);
	Part(A, EShape::Cylinder, FVector(0, 0, 8), FVector(10, 10, 220), Bone, FRotator(90, 0, 0));
	for (int32 i = 0; i < 7; ++i)
		for (int32 s : { -1, 1 })
			Part(A, EShape::Cylinder, FVector(-70 + i * 22.f, s * 26.f, 14), FVector(6, 6, 60), Bone, FRotator(0, 0, s * 60.f));
	Part(A, EShape::Sphere, FVector(-110, 0, 18), FVector(40, 34, 30), Bone);
	return A;
}

AActor* PathStrip(UWorld* W, const TArray<FVector>& Pts, float Width, const FLinearColor& C)
{
	AActor* A = NewActor(W, Pts.Num() ? Pts[0] : FVector::ZeroVector);
	for (int32 i = 0; i + 1 < Pts.Num(); ++i)
	{
		const FVector D = Pts[i + 1] - Pts[i];
		const float Len = D.Size2D();
		const FVector Mid = (Pts[i] + Pts[i + 1]) * 0.5f - A->GetActorLocation();
		Part(A, EShape::Cube, Mid + FVector(0, 0, 2 + (i % 2) * 0.5f), FVector(Len + Width * 0.6f, Width * FMath::FRandRange(0.85f, 1.1f), 4), C * FMath::FRandRange(0.9f, 1.1f), FRotator(0, D.Rotation().Yaw, 0));
		for (int32 k = 0; k < 3; ++k)
			Part(A, EShape::Sphere, Mid + FVector(FMath::FRandRange(-Len * 0.5f, Len * 0.5f), FMath::FRandRange(-Width * 0.6f, Width * 0.6f), 4), FVector(14, 12, 8), Stone * 1.2f);
	}
	return A;
}

AActor* Stream(UWorld* W, const TArray<FVector>& Pts, float Width)
{
	AActor* A = NewActor(W, Pts.Num() ? Pts[0] : FVector::ZeroVector);
	const FLinearColor Water(0.04f, 0.07f, 0.1f);
	for (int32 i = 0; i + 1 < Pts.Num(); ++i)
	{
		const FVector D = Pts[i + 1] - Pts[i];
		const FVector Mid = (Pts[i] + Pts[i + 1]) * 0.5f - A->GetActorLocation();
		UStaticMeshComponent* P = Part(A, EShape::Cube, Mid + FVector(0, 0, 3), FVector(D.Size2D() + Width * 0.5f, Width, 6), Water, FRotator(0, D.Rotation().Yaw, 0));
		P->SetMaterial(0, EmitMat(Water, 0.15f));
		for (int32 k = 0; k < 4; ++k)
			GlowPart(A, EShape::Sphere, Mid + FVector(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-Width * 0.4f, Width * 0.4f), 8), FVector(30, 18, 6), FLinearColor(0.35f, 0.4f, 0.45f));
		Part(A, EShape::Sphere, Mid + FVector(0, Width * 0.55f, 10), FVector(80, 60, 40), Stone);
		Part(A, EShape::Sphere, Mid + FVector(0, -Width * 0.55f, 10), FVector(70, 60, 36), Stone);
	}
	return A;
}

AActor* FogBank(UWorld* W, const FVector& L, float Radius, float Density, const FLinearColor& Albedo)
{
	AActor* A = NewActor(W, L);
	ULocalFogVolumeComponent* F = NewObject<ULocalFogVolumeComponent>(A);
	F->SetupAttachment(A->GetRootComponent());
	F->SetRelativeScale3D(FVector(Radius / ULocalFogVolumeComponent::GetBaseVolumeSize()));
	F->SetRadialFogExtinction(Density);
	F->SetHeightFogExtinction(Density * 1.5f);
	F->SetHeightFogFalloff(350.f);
	F->SetHeightFogOffset(-0.6f);
	F->SetFogAlbedo(Albedo);
	F->SetFogPhaseG(0.3f);
	F->RegisterComponent();
	A->AddInstanceComponent(F);
	return A;
}

AActor* Mound(UWorld* W, const FVector& L, float S, const FLinearColor& C)
{
	AActor* A = NewActor(W, L, FMath::FRandRange(0.f, 360.f));
	Part(A, EShape::Sphere, FVector(0, 0, -30 * S), FVector(420, 340, 220) * S, C, FRotator::ZeroRotator, true);
	return A;
}

AActor* Bridge(UWorld* W, const FVector& L, float Yaw, float Len)
{
	AActor* A = NewActor(W, L, Yaw);
	for (int32 k = 0; k < 7; ++k)
	{
		const float T = k / 6.f;
		const float Zc = FMath::Sin(T * PI) * 50.f;
		Part(A, EShape::Cube, FVector(-Len * 0.5f + T * Len, 0, 14 + Zc), FVector(Len / 6.f + 10.f, 190, 26), Stone * FMath::FRandRange(0.85f, 1.05f));
		Part(A, EShape::Cube, FVector(-Len * 0.5f + T * Len, 95, 50 + Zc), FVector(Len / 6.f + 6.f, 22, 50), Stone * 0.8f);
		Part(A, EShape::Cube, FVector(-Len * 0.5f + T * Len, -95, 50 + Zc), FVector(Len / 6.f + 6.f, 22, 50), Stone * 0.8f);
	}
	return A;
}

AActor* Campfire(UWorld* W, const FVector& L)
{
	AActor* A = NewActor(W, L);
	if (!KPart(A, TEXT("Nature/campfire_logs"), FVector::ZeroVector, 0.f, 3.f))
		for (int32 i = 0; i < 3; ++i)
			Part(A, EShape::Cylinder, FVector(0, 0, 8), FVector(14, 14, 80), DarkWood, FRotator(90, i * 60.f, 0));
	GlowPart(A, EShape::Cone, FVector(0, 0, 35), FVector(40, 40, 60), FLinearColor(1.f, 0.38f, 0.08f));
	GlowPart(A, EShape::Cone, FVector(0, 0, 42), FVector(22, 22, 50), FLinearColor(1.f, 0.78f, 0.3f));
	for (int32 i = 0; i < 8; ++i)
		Part(A, EShape::Sphere, FVector(FMath::Cos(i * PI / 4) * 50, FMath::Sin(i * PI / 4) * 50, 6), FVector(22, 22, 16), Stone);
	Light(A, FVector(0, 0, 80), FLinearColor(1.f, 0.45f, 0.15f), 45.f, 1000.f, true);
	return A;
}

AActor* Tent(UWorld* W, const FVector& L, float Yaw, const FLinearColor& C)
{
	AActor* A = NewActor(W, L, Yaw);
	Part(A, EShape::Cube, FVector(0, 0, 40), FVector(200, 110, 110), C * 0.5f, FRotator(0, 0, 45));
	Part(A, EShape::Cube, FVector(101, 0, 30), FVector(4, 40, 60), Black);
	Collider(A, FVector(0, 0, 60), FVector(200, 160, 120));
	return A;
}

AActor* Grave(UWorld* W, const FVector& L, float Yaw)
{
	AActor* A = NewActor(W, L, Yaw + FMath::FRandRange(-12.f, 12.f));
	static const TCHAR* Stones[] = { TEXT("Graveyard/gravestone-cross"), TEXT("Graveyard/gravestone-round"), TEXT("Graveyard/gravestone-broken"),
		TEXT("Graveyard/gravestone-bevel"), TEXT("Graveyard/gravestone-decorative"), TEXT("Graveyard/gravestone-wide"), TEXT("Graveyard/cross-wood") };
	if (KPart(A, Stones[FMath::RandRange(0, 6)], FVector::ZeroVector, 90.f, 2.f))
	{
		if (FMath::FRand() < 0.4f) KPart(A, TEXT("Graveyard/grave-border"), FVector(-60, 0, 0), 90.f, 2.f);
	}
	else
	{
		Part(A, EShape::Cube, FVector(0, 0, 35), FVector(16, 50, 70), Stone);
		Part(A, EShape::Cube, FVector(-60, 0, 4), FVector(110, 60, 10), FLinearColor(0.12f, 0.09f, 0.06f));
	}
	Collider(A, FVector(0, 0, 40), FVector(40, 70, 80));
	return A;
}

AActor* Signpost(UWorld* W, const FVector& L, float Yaw)
{
	AActor* A = NewActor(W, L, Yaw);
	Part(A, EShape::Cylinder, FVector(0, 0, 80), FVector(12, 12, 160), Wood, FRotator::ZeroRotator, true);
	Part(A, EShape::Cube, FVector(0, 30, 140), FVector(8, 80, 26), FLinearColor(0.3f, 0.2f, 0.1f));
	Part(A, EShape::Cube, FVector(0, -25, 110), FVector(8, 70, 22), FLinearColor(0.28f, 0.18f, 0.09f));
	GlowPart(A, EShape::Sphere, FVector(0, 0, 175), FVector(16, 16, 16), FLinearColor(0.5f, 0.75f, 1.f));
	Light(A, FVector(0, 0, 175), FLinearColor(0.5f, 0.7f, 1.f), 6.f, 450.f);
	return A;
}

AActor* Chest(UWorld* W, const FVector& L, float Yaw)
{
	AActor* A = NewActor(W, L, Yaw);
	Part(A, EShape::Cube, FVector(0, 0, 22), FVector(60, 80, 44), FLinearColor(0.15f, 0.08f, 0.04f), FRotator::ZeroRotator, true);
	GlowPart(A, EShape::Cylinder, FVector(0, 0, 44), FVector(60, 80, 30), FLinearColor(0.95f, 0.65f, 0.2f), FRotator(0, 0, 90));
	Light(A, FVector(0, 0, 70), FLinearColor(1.f, 0.7f, 0.3f), 4.f, 300.f);
	return A;
}

// The town's heart from the concept: a coiling stone tentacle cradling a violet orb over glowing water and rune rings.
AActor* EldritchFountain(UWorld* W, const FVector& L)
{
	AActor* A = NewActor(W, L);
	if (!KPart(A, TEXT("Town/fountain-round-detail"), FVector::ZeroVector, 0.f, 2.f))
		Part(A, EShape::Cylinder, FVector(0, 0, 40), FVector(400, 400, 80), Stone);
	UStaticMeshComponent* Water = Part(A, EShape::Cylinder, FVector(0, 0, 46), FVector(330, 330, 4), Eldritch);
	Water->SetMaterial(0, EmitMat(Eldritch * 0.25f, 0.5f));
	for (float Rr : { 560.f, 760.f })
	{
		UStaticMeshComponent* Ring = Part(A, EShape::Cylinder, FVector(0, 0, 4), FVector(Rr * 2.f, Rr * 2.f, 1.f), Eldritch);
		Ring->SetMaterial(0, EmitMat(Eldritch * 0.3f, 0.7f));
		Part(A, EShape::Cylinder, FVector(0, 0, 4.5f), FVector(Rr * 2.f - 22.f, Rr * 2.f - 22.f, 1.f), FLinearColor(0.13f, 0.125f, 0.13f));
	}
	FVector P(0, 0, 40);
	FVector Dir(0, 0, 1);
	float Rad = 34.f;
	for (int32 i = 0; i < 11; ++i)
	{
		const float Len = 46.f;
		Dir = Dir.RotateAngleAxis(i < 7 ? 9.f : 28.f, FVector(0.4f, 1.f, 0.f).GetSafeNormal()).RotateAngleAxis(14.f, FVector::UpVector);
		const FVector Next = P + Dir * Len;
		Part(A, EShape::Cylinder, (P + Next) * 0.5f, FVector(Rad * 2.f, Rad * 2.f, Len * 1.15f), DarkStone * 1.3f, FRotationMatrix::MakeFromZ(Dir).Rotator());
		if (i % 2 == 0) Part(A, EShape::Sphere, Next + FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal() * Rad * 0.8f, FVector(Rad * 0.5f), DarkStone * 1.6f);
		P = Next;
		Rad *= 0.86f;
	}
	GlowPart(A, EShape::Sphere, FVector(0, 0, 470), FVector(90, 90, 90), Eldritch);
	Light(A, FVector(0, 0, 470), Eldritch, 40.f, 1400.f, true);
	Light(A, FVector(0, 0, 80), Eldritch, 10.f, 700.f);
	return A;
}

AActor* RuneObelisk(UWorld* W, const FVector& L, float S)
{
	AActor* A = NewActor(W, L, FMath::FRandRange(0.f, 360.f));
	if (!KPart(A, TEXT("Graveyard/pillar-obelisk"), FVector::ZeroVector, 0.f, 3.6f * S))
		Part(A, EShape::Cube, FVector(0, 0, 180) * S, FVector(80, 80, 360) * S, Black);
	for (int32 i = 0; i < 3; ++i)
		for (int32 f = 0; f < 4; ++f)
		{
			const FRotator R(0, f * 90.f, 0);
			GlowPart(A, EShape::Cube, R.RotateVector(FVector(34 * S, 0, (90 + i * 70.f) * S)), FVector(3, 26 - i * 6.f, 26) * S, Eldritch, R);
		}
	Light(A, FVector(0, 0, 250) * S, Eldritch, 45.f, 1100.f * S);
	Collider(A, FVector(0, 0, 180) * S, FVector(90, 90, 360) * S);
	return A;
}

AActor* Banner(UWorld* W, const FVector& L, float Yaw)
{
	AActor* A = NewActor(W, L, Yaw);
	Part(A, EShape::Cylinder, FVector(0, 0, 160), FVector(10, 10, 320), DarkWood);
	Part(A, EShape::Cylinder, FVector(0, 30, 300), FVector(6, 6, 80), DarkWood, FRotator(0, 0, 90));
	Part(A, EShape::Cube, FVector(0, 60, 220), FVector(4, 60, 150), FLinearColor(0.12f, 0.04f, 0.16f));
	GlowPart(A, EShape::Cylinder, FVector(3, 60, 235), FVector(30, 30, 2), Eldritch, FRotator(0, 0, 90));
	return A;
}

// The Starfall Altar: obelisks around a slab where pilgrims pray to the sleeping star.
AActor* Shrine(UWorld* W, const FVector& L)
{
	AActor* A = NewActor(W, L);
	Part(A, EShape::Cylinder, FVector(0, 0, 15), FVector(520, 520, 30), DarkStone, FRotator::ZeroRotator, true);
	Part(A, EShape::Cylinder, FVector(0, 0, 40), FVector(360, 360, 30), DarkStone * 1.2f, FRotator::ZeroRotator, true);
	if (!KPart(A, TEXT("Graveyard/altar-stone"), FVector(0, 0, 55), 0.f, 2.6f))
		Part(A, EShape::Cube, FVector(0, 0, 100), FVector(90, 90, 100), Stone);
	GlowPart(A, EShape::Sphere, FVector(0, 0, 260), FVector(80, 80, 80), Eldritch);
	Light(A, FVector(0, 0, 260), Eldritch, 60.f, 1400.f, true);
	for (int32 i = 0; i < 6; ++i)
	{
		const float Ang = i * PI / 3;
		const FVector P(FMath::Cos(Ang) * 240, FMath::Sin(Ang) * 240, 55);
		if (!KPart(A, TEXT("Graveyard/pillar-obelisk"), P, i * 60.f, 2.6f))
			Part(A, EShape::Cube, P + FVector(0, 0, 130), FVector(50, 50, 260), Stone);
		GlowPart(A, EShape::Sphere, P + FVector(0, 0, 300), FVector(22, 22, 22), Sickly);
		Collider(A, P + FVector(0, 0, 130), FVector(60, 60, 260));
	}
	return A;
}

AActor* Monolith(UWorld* W, const FVector& L, float S)
{
	AActor* A = NewActor(W, L, FMath::FRandRange(0.f, 360.f));
	Part(A, EShape::Cube, FVector(0, 0, 210) * S, FVector(110, 60, 420) * S, Black, FRotator(0, 0, 3));
	for (int32 i = 0; i < 6; ++i)
	{
		UStaticMeshComponent* R = Part(A, EShape::Cube, FVector(31, 0, 70 + i * 55.f) * S, FVector(2, 60 - (i % 3) * 14.f, 8) * S, Eldritch);
		R->SetMaterial(0, GlowMat(Eldritch));
	}
	for (int32 i = 0; i < 4; ++i)
		GlowPart(A, EShape::Cone, FVector(FMath::Cos(i * PI / 2) * 120, FMath::Sin(i * PI / 2) * 120, 480 + (i % 2) * 40.f) * S, FVector(24, 24, 60) * S, Sickly, FRotator(180, 0, 0));
	Light(A, FVector(0, 0, 300) * S, Eldritch, 30.f, 1100.f * S);
	Collider(A, FVector(0, 0, 210) * S, FVector(120, 70, 420) * S);
	return A;
}

AActor* Keep(UWorld* W, const FVector& L, float Yaw)
{
	AActor* A = NewActor(W, L, Yaw);
	const FLinearColor Wall(0.12f, 0.11f, 0.14f), Roof(0.22f, 0.04f, 0.07f);
	const float S = 3.2f, Lv = 101.f * S;
	if (KMesh(TEXT("Castle/tower-square-base")))
	{
		for (int32 sx : { -1, 1 }) for (int32 sy : { -1, 1 })
		{
			const FVector P(sx * 360.f, sy * 460.f, 0);
			KPart(A, TEXT("Castle/tower-square-base"), P, 0.f, S);
			KPart(A, TEXT("Castle/tower-square-mid-windows"), P + FVector(0, 0, Lv), 0.f, S);
			KPart(A, TEXT("Castle/tower-square-mid"), P + FVector(0, 0, Lv * 2), 0.f, S);
			KPart(A, TEXT("Castle/tower-square-top-roof-high"), P + FVector(0, 0, Lv * 3), 0.f, S);
			Light(A, P + FVector(sx * 180.f, 0, Lv * 1.5f), FLinearColor(0.9f, 0.15f, 0.12f), 30.f, 900.f);
		}
		for (int32 k = -1; k <= 1; ++k)
		{
			if (k != 0) KPart(A, TEXT("Castle/wall"), FVector(360, k * 300.f, 0), 90.f, S * 0.95f);
			KPart(A, TEXT("Castle/wall"), FVector(-360, k * 300.f, 0), 90.f, S * 0.95f);
		}
		KPart(A, TEXT("Castle/wall"), FVector(0, 460, 0), 0.f, S * 0.95f);
		KPart(A, TEXT("Castle/wall"), FVector(0, -460, 0), 0.f, S * 0.95f);
		KPart(A, TEXT("Castle/tower-hexagon-base"), FVector(0, 0, 0), 0.f, 5.f);
		KPart(A, TEXT("Castle/tower-hexagon-mid"), FVector(0, 0, 131.f * 5.f), 0.f, 5.f);
		KPart(A, TEXT("Castle/tower-hexagon-roof"), FVector(0, 0, 131.f * 5.f + 101.f * 5.f), 0.f, 5.f);
	}
	else
	{
		Part(A, EShape::Cube, FVector(0, 0, 250), FVector(700, 900, 500), Wall);
		for (int32 sx : { -1, 1 }) for (int32 sy : { -1, 1 })
		{
			const FVector P(sx * 360.f, sy * 460.f, 0);
			Part(A, EShape::Cylinder, P + FVector(0, 0, 350), FVector(220, 220, 700), Wall * 1.1f);
			Part(A, EShape::Cone, P + FVector(0, 0, 790), FVector(270, 270, 200), Roof);
		}
	}
	// gate glow on the +X face
	Part(A, EShape::Cube, FVector(372, 0, 120), FVector(10, 200, 240), Black);
	GlowPart(A, EShape::Cube, FVector(378, 0, 60), FVector(4, 180, 10), FLinearColor(0.9f, 0.2f, 0.15f));
	Light(A, FVector(450, 0, 160), FLinearColor(0.9f, 0.18f, 0.12f), 40.f, 1000.f, true);
	GlowPart(A, EShape::Sphere, FVector(0, 0, 1500), FVector(120, 120, 120), Eldritch);
	Light(A, FVector(0, 0, 1500), Eldritch, 120.f, 3000.f);
	Collider(A, FVector(0, 0, 400), FVector(900, 1100, 800));
	return A;
}

AActor* BarrowDoor(UWorld* W, const FVector& L, float Yaw)
{
	AActor* A = NewActor(W, L, Yaw);
	Part(A, EShape::Sphere, FVector(-200, 0, -40), FVector(560, 560, 340), FLinearColor(0.12f, 0.14f, 0.07f), FRotator::ZeroRotator, true);
	if (!KPart(A, TEXT("Graveyard/crypt-large"), FVector(40, 0, 0), 90.f, 2.4f))
		Part(A, EShape::Cube, FVector(90, 0, 80), FVector(30, 200, 180), Stone);
	for (int32 i = 0; i < 6; ++i)
		GlowPart(A, EShape::Sphere, FVector(170, FMath::Cos(i * PI / 3) * 80, 120 + FMath::Sin(i * PI / 3) * 80), FVector(14, 14, 14), FLinearColor(0.3f, 0.8f, 1.f));
	Light(A, FVector(240, 0, 150), FLinearColor(0.3f, 0.75f, 1.f), 25.f, 800.f);
	Collider(A, FVector(40, 0, 120), FVector(300, 300, 240));
	return A;
}

AActor* Marker(UWorld* W, const FVector& L, const FLinearColor& C)
{
	AActor* A = NewActor(W, L);
	GlowPart(A, EShape::Cone, FVector(0, 0, 260), FVector(40, 40, 80), C, FRotator(180, 0, 0));
	GlowPart(A, EShape::Cylinder, FVector(0, 0, 2), FVector(180, 180, 2), C * 0.35f);
	Light(A, FVector(0, 0, 200), C, 20.f, 650.f);
	return A;
}

// The sleeping star-god hanging in the sky: a dark core, a sickly corona, and tendrils reaching for the world.
AActor* SkyGod(UWorld* W, const FVector& L, float S)
{
	AActor* A = NewActor(W, L);
	UStaticMeshComponent* Core = Part(A, EShape::Sphere, FVector::ZeroVector, FVector(1000) * S, FLinearColor(0.05f, 0.01f, 0.08f));
	Core->SetMaterial(0, EmitMat(FLinearColor(0.15f, 0.04f, 0.25f), 0.5f));
	Core->SetCastShadow(false);
	UStaticMeshComponent* Corona = Part(A, EShape::Cylinder, FVector::ZeroVector, FVector(1700, 1700, 20) * S, Sickly);
	Corona->SetMaterial(0, EmitMat(Sickly * 0.5f, 1.2f));
	Corona->SetRelativeRotation(FRotator(70, 0, 0));
	Corona->SetCastShadow(false);
	for (int32 i = 0; i < 9; ++i)
	{
		const FVector D = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 0.4f)).GetSafeNormal();
		UStaticMeshComponent* T = Part(A, EShape::Cone, D * 900.f * S, FVector(120, 120, 1400) * S, Eldritch, FRotationMatrix::MakeFromZ(-D).Rotator());
		T->SetMaterial(0, EmitMat(Eldritch * 0.6f, 0.9f));
		T->SetCastShadow(false);
	}
	return A;
}

/* ---------- Characters ---------- */

void Chibi(AActor* A, USceneComponent* Parent, const FVector& B, const FLinearColor& Body, const FLinearColor& Skin,
	const FLinearColor& Hat, int32 HatStyle, float S, const FString& Weapon)
{
	auto P = [&](EShape Sh, FVector L, FVector Sz, const FLinearColor& C, FRotator R = FRotator::ZeroRotator)
	{
		return Part(A, Sh, B + L * S, Sz * S, C, R, false, Parent);
	};
	P(EShape::Cylinder, FVector(0, 14, 14), FVector(20, 20, 28), Body * 0.5f);
	P(EShape::Cylinder, FVector(0, -14, 14), FVector(20, 20, 28), Body * 0.5f);
	P(EShape::Cone, FVector(0, 0, 50), FVector(80, 76, 90), Body);
	P(EShape::Sphere, FVector(0, 34, 66), FVector(20, 20, 20), Skin);
	P(EShape::Sphere, FVector(0, -34, 66), FVector(20, 20, 20), Skin);
	P(EShape::Sphere, FVector(0, 0, 122), FVector(70, 66, 66), Skin);
	Part(A, EShape::Sphere, B + FVector(31, 13, 126) * S, FVector(9, 9, 12) * S, FLinearColor(0.02f, 0.02f, 0.03f), FRotator::ZeroRotator, false, Parent);
	Part(A, EShape::Sphere, B + FVector(31, -13, 126) * S, FVector(9, 9, 12) * S, FLinearColor(0.02f, 0.02f, 0.03f), FRotator::ZeroRotator, false, Parent);
	switch (HatStyle)
	{
	case 1: P(EShape::Sphere, FVector(-2, 0, 136), FVector(80, 76, 54), Hat); P(EShape::Cube, FVector(36, 0, 132), FVector(6, 8, 30), Hat * 0.8f); break;
	case 2: P(EShape::Cylinder, FVector(0, 0, 150), FVector(110, 110, 6), Hat); P(EShape::Cone, FVector(-6, 0, 200), FVector(70, 70, 100), Hat, FRotator(-10, 0, 0)); break;
	case 3: GlowPart(A, EShape::Cylinder, B + FVector(0, 0, 178) * S, FVector(60, 60, 3) * S, Hat, FRotator::ZeroRotator, Parent); P(EShape::Sphere, FVector(-6, 0, 136), FVector(80, 78, 56), Body * 0.9f); break;
	case 4: P(EShape::Cone, FVector(-8, 0, 150), FVector(90, 86, 110), Hat); P(EShape::Cube, FVector(30, 0, 110), FVector(8, 56, 18), Hat * 0.7f); break;
	default: P(EShape::Sphere, FVector(-6, 0, 146), FVector(66, 64, 32), Hat); break;
	}
	const FLinearColor Steel(0.45f, 0.46f, 0.5f);
	if (Weapon == TEXT("sword")) { P(EShape::Cube, FVector(16, -42, 90), FVector(8, 10, 90), Steel); P(EShape::Cube, FVector(16, -42, 48), FVector(10, 30, 6), FLinearColor(0.4f, 0.3f, 0.1f)); }
	else if (Weapon == TEXT("staff"))
	{
		P(EShape::Cylinder, FVector(10, -42, 85), FVector(8, 8, 170), DarkWood);
		GlowPart(A, EShape::Sphere, B + FVector(10, -42, 175) * S, FVector(18, 18, 18) * S, FLinearColor(0.4f, 0.6f, 1.f), FRotator::ZeroRotator, Parent);
	}
	else if (Weapon == TEXT("dagger")) P(EShape::Cube, FVector(20, -40, 60), FVector(6, 8, 40), Steel, FRotator(-30, 0, 0));
	else if (Weapon == TEXT("mace")) { P(EShape::Cylinder, FVector(14, -42, 70), FVector(8, 8, 70), DarkWood); P(EShape::Sphere, FVector(14, -42, 110), FVector(26, 26, 26), Steel); }
}

AActor* Npc(UWorld* W, const FVector& L, float Yaw, const FLinearColor& Body, const FLinearColor& Skin, const FLinearColor& Hat, int32 HatStyle)
{
	AActor* A = NewActor(W, L, Yaw);
	Collider(A, FVector(0, 0, 80), FVector(80, 80, 160), 0.f, true);
	Chibi(A, nullptr, FVector::ZeroVector, Body, Skin, Hat, HatStyle);
	return A;
}

void Foe(AActor* A, USceneComponent* Parent, const FString& Id, const FVector& B)
{
	auto P = [&](EShape Sh, FVector L, FVector Sz, const FLinearColor& C, FRotator R = FRotator::ZeroRotator)
	{
		return Part(A, Sh, B + L, Sz, C, R, false, Parent);
	};
	if (Id == TEXT("wolf"))
	{
		const FLinearColor Fur(0.16f, 0.15f, 0.17f);
		P(EShape::Sphere, FVector(0, 0, 60), FVector(130, 70, 70), Fur);
		P(EShape::Sphere, FVector(70, 0, 85), FVector(60, 55, 55), Fur * 1.1f);
		P(EShape::Cube, FVector(105, 0, 75), FVector(40, 26, 24), Fur * 1.2f);
		P(EShape::Cone, FVector(65, 15, 120), FVector(20, 16, 30), Fur);
		P(EShape::Cone, FVector(65, -15, 120), FVector(20, 16, 30), Fur);
		GlowPart(A, EShape::Sphere, B + FVector(96, 13, 94), FVector(9, 9, 9), FLinearColor(1.f, 0.15f, 0.05f), FRotator::ZeroRotator, Parent);
		GlowPart(A, EShape::Sphere, B + FVector(96, -13, 94), FVector(9, 9, 9), FLinearColor(1.f, 0.15f, 0.05f), FRotator::ZeroRotator, Parent);
		for (int32 sx : { -1, 1 }) for (int32 sy : { -1, 1 })
			P(EShape::Cylinder, FVector(sx * 38.f, sy * 20.f, 20), FVector(16, 16, 40), Fur * 0.8f);
		P(EShape::Cone, FVector(-75, 0, 75), FVector(24, 24, 60), Fur, FRotator(60, 0, 0));
		return;
	}
	const TMap<FString, TPair<FString, float>> Kenney = {
		{ TEXT("skeleton"), { TEXT("skeleton"), 190.f } }, { TEXT("ghoul"), { TEXT("zombie"), 180.f } },
		{ TEXT("drowned"), { TEXT("ghost"), 220.f } }, { TEXT("boss"), { TEXT("vampire"), 320.f } } };
	if (const TPair<FString, float>* K = Kenney.Find(Id))
		if (KChar(A, Parent, K->Key, B, 0.f, K->Value))
		{
			if (Id == TEXT("boss"))
				for (int32 i = 0; i < 5; ++i)
					GlowPart(A, EShape::Cone, B + FVector(FMath::Cos(i * 2 * PI / 5) * 34, FMath::Sin(i * 2 * PI / 5) * 34, 340), FVector(18, 18, 40), Eldritch, FRotator::ZeroRotator, Parent);
			if (Id == TEXT("drowned")) Light(A, B + FVector(0, 0, 150), FLinearColor(0.3f, 0.8f, 0.7f), 12.f, 400.f, false, Parent);
			return;
		}
	if (Id == TEXT("skeleton")) Chibi(A, Parent, B, FLinearColor(0.5f, 0.48f, 0.42f), FLinearColor(0.6f, 0.58f, 0.5f), FLinearColor(0.2f, 0.16f, 0.12f), 4, 1.f, TEXT("sword"));
	else if (Id == TEXT("bandit")) Chibi(A, Parent, B, FLinearColor(0.25f, 0.05f, 0.04f), FLinearColor(0.6f, 0.45f, 0.35f), FLinearColor(0.06f, 0.05f, 0.05f), 4, 1.f, TEXT("dagger"));
	else if (Id == TEXT("ghoul")) Chibi(A, Parent, B, FLinearColor(0.15f, 0.2f, 0.13f), FLinearColor(0.3f, 0.38f, 0.28f), FLinearColor(0.08f, 0.1f, 0.07f), 0, 0.95f);
	else if (Id == TEXT("drowned")) Chibi(A, Parent, B, FLinearColor(0.08f, 0.2f, 0.22f), FLinearColor(0.25f, 0.35f, 0.33f), FLinearColor(0.12f, 0.2f, 0.18f), 1, 1.15f, TEXT("sword"));
	else Chibi(A, Parent, B, FLinearColor(0.1f, 0.03f, 0.12f), FLinearColor(0.45f, 0.45f, 0.52f), FLinearColor(0.05f, 0.02f, 0.06f), 0, 1.6f, TEXT("sword"));
}

}
