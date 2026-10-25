// Runtime prop kit for the dark-fantasy look. Props prefer imported CC0 Kenney meshes (graded grim by M_KenneyGrim),
// fall back to primitives with the lit M_HollowLit material, and carry their own point lights.
// Every builder is the single place to swap in a Tripo/Fab mesh later.
#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UPointLightComponent;
class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

namespace HollowArt
{
	enum class EShape : uint8 { Cube, Sphere, Cylinder, Cone };

	void Init(UMaterialInterface* Base, UMaterialInterface* Grim);
	extern bool GNoLights;   // set while building the miniature board

	UMaterialInstanceDynamic* Mat(const FLinearColor& C);
	UMaterialInstanceDynamic* GlowMat(const FLinearColor& C);
	UMaterialInstanceDynamic* EmitMat(const FLinearColor& C, float Emit);

	AActor* NewActor(UWorld* W, const FVector& Loc, float Yaw = 0.f);
	UStaticMeshComponent* Part(AActor* A, EShape S, const FVector& Loc, const FVector& Size, const FLinearColor& C,
		const FRotator& Rot = FRotator::ZeroRotator, bool bCollide = false, USceneComponent* Parent = nullptr);
	UStaticMeshComponent* GlowPart(AActor* A, EShape S, const FVector& Loc, const FVector& Size, const FLinearColor& C,
		const FRotator& Rot = FRotator::ZeroRotator, USceneComponent* Parent = nullptr);
	UStaticMeshComponent* Collider(AActor* A, const FVector& Loc, const FVector& Size, float Yaw = 0.f, bool bRound = false);
	UPointLightComponent* Light(AActor* A, const FVector& Loc, const FLinearColor& C, float Candela, float Radius, bool bShadows = false, USceneComponent* Parent = nullptr);
	UProceduralMeshComponent* HexPrism(AActor* A, float Radius, float Height, const FLinearColor& C, bool bCollide,
		const FVector& Loc = FVector::ZeroVector, UMaterialInstanceDynamic* MatOverride = nullptr);

	// Kenney meshes: Path is "Kit/name" under /Game/Hollow/Kenney. Returns null if the asset is missing.
	UStaticMesh* KMesh(const FString& Path);
	UStaticMeshComponent* KPart(AActor* A, const FString& Path, const FVector& Loc, float Yaw, float Scale,
		USceneComponent* Parent = nullptr, const FVector& NonUniform = FVector::OneVector);
	USkeletalMeshComponent* KChar(AActor* A, USceneComponent* Parent, const FString& Name, const FVector& Loc, float Yaw, float Height);

	// Scenery
	AActor* PineTree(UWorld* W, const FVector& L, float Scale, const FLinearColor& Leaf);
	AActor* RoundTree(UWorld* W, const FVector& L, float Scale, const FLinearColor& Leaf);
	AActor* DeadTree(UWorld* W, const FVector& L, float Scale, int32 Seed);
	AActor* AutumnTree(UWorld* W, const FVector& L, float Scale);
	AActor* Ridge(UWorld* W, const FVector& L, float Yaw, float Scale, int32 Seed);
	AActor* Waterfall(UWorld* W, const FVector& Top, const FVector& Bottom, float Width);
	AActor* Boulders(UWorld* W, const FVector& L, float Scale, int32 Seed);
	AActor* Rock(UWorld* W, const FVector& L, float Scale, float Yaw);
	AActor* Tuft(UWorld* W, const FVector& L, const FLinearColor& C);
	AActor* Shrub(UWorld* W, const FVector& L, float Scale);
	AActor* House(UWorld* W, const FVector& L, float Yaw, const FLinearColor& Wall, const FLinearColor& Roof, const FVector& Size, bool bSign = false, bool bStone = false);
	AActor* Well(UWorld* W, const FVector& L);
	AActor* Lantern(UWorld* W, const FVector& L);
	AActor* Fence(UWorld* W, const FVector& A, const FVector& B);
	AActor* Stall(UWorld* W, const FVector& L, float Yaw, const FLinearColor& Awning);
	AActor* Haystack(UWorld* W, const FVector& L);
	AActor* Reeds(UWorld* W, const FVector& L);
	AActor* Pool(UWorld* W, const FVector& L, float Radius, const FLinearColor& C);
	AActor* Pillar(UWorld* W, const FVector& L, float Height, bool bBroken);
	AActor* BrokenWall(UWorld* W, const FVector& L, float Yaw, float Len);
	AActor* RuinArch(UWorld* W, const FVector& L, float Yaw, bool bFire);
	AActor* RitualCircle(UWorld* W, const FVector& L, float Radius, const FLinearColor& Glow);
	AActor* Wagon(UWorld* W, const FVector& L, float Yaw);
	AActor* Bones(UWorld* W, const FVector& L, float Yaw);
	AActor* Stream(UWorld* W, const TArray<FVector>& Points, float Width);
	AActor* PathStrip(UWorld* W, const TArray<FVector>& Points, float Width, const FLinearColor& C);
	AActor* FogBank(UWorld* W, const FVector& L, float Radius, float Density, const FLinearColor& Albedo);
	AActor* Mound(UWorld* W, const FVector& L, float Scale, const FLinearColor& C);
	AActor* Bridge(UWorld* W, const FVector& L, float Yaw, float Len);
	AActor* Campfire(UWorld* W, const FVector& L);
	AActor* Tent(UWorld* W, const FVector& L, float Yaw, const FLinearColor& C);
	AActor* Grave(UWorld* W, const FVector& L, float Yaw);
	AActor* Signpost(UWorld* W, const FVector& L, float Yaw);
	AActor* Chest(UWorld* W, const FVector& L, float Yaw);
	AActor* Shrine(UWorld* W, const FVector& L);
	AActor* Monolith(UWorld* W, const FVector& L, float Scale);
	AActor* EldritchFountain(UWorld* W, const FVector& L);
	AActor* RuneObelisk(UWorld* W, const FVector& L, float Scale);
	AActor* Banner(UWorld* W, const FVector& L, float Yaw);
	AActor* Keep(UWorld* W, const FVector& L, float Yaw);
	AActor* BarrowDoor(UWorld* W, const FVector& L, float Yaw);
	AActor* Marker(UWorld* W, const FVector& L, const FLinearColor& C);
	AActor* SkyGod(UWorld* W, const FVector& L, float Scale);

	// Characters. Front faces +X; feet at Base.
	void Chibi(AActor* A, USceneComponent* Parent, const FVector& Base, const FLinearColor& Body, const FLinearColor& Skin,
		const FLinearColor& Hat, int32 HatStyle, float Scale = 1.f, const FString& Weapon = FString());
	AActor* Npc(UWorld* W, const FVector& L, float Yaw, const FLinearColor& Body, const FLinearColor& Skin, const FLinearColor& Hat, int32 HatStyle);
	void Foe(AActor* A, USceneComponent* Parent, const FString& FoeId, const FVector& Base);
}
