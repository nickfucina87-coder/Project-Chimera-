// Project Zomboid tiles as stand-in building art (local proof of concept only; the art belongs to The Indie Stone and
// is read from ExternalAssets/PZ, which Tools/ExtractPZ.py fills from the user's own install and git ignores).
//
// PZ draws everything isometric in a 128x256 frame. We rebuild it in 3D so it survives a rotating camera:
//   floor  - the diamond at the frame's foot (64,192 / 128,224 / 64,256 / 0,224) is mapped back onto a flat square
//   wall N - the face from (64,192)-(128,224) up 192 px is stood up on the square's north edge
//   wall W - the face from (64,192)-(0,224) up 192 px is stood up on the square's west edge
//   corner - a WallNW tile holds both faces; doors and windows are drawn in the wall plane the same way
//   object - furniture keeps its iso drawing on a camera-facing card, feet on the square's centre
// Walls sit only on N and W edges, as in PZ, so every wall belongs to one square.
#include "HollowGameMode.h"

#include "Components/MaterialBillboardComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HollowArt.h"
#include "HollowRules.h"
#include "HollowHero.h"

namespace
{
	struct FPZSprite { FString Page; int32 X = 0, Y = 0, W = 0, H = 0, Ox = 0, Oy = 0; };
	struct FPZ
	{
		bool bTried = false, bOk = false;
		TMap<FString, FPZSprite> Atlas;
		TMap<FString, TSharedPtr<FJsonObject>> Props;
		TMap<FString, FString> Palette;
		TMap<FString, FImage> Pages;
		TMap<FString, UTexture2D*> Frames;
	};
	FPZ& PZ() { static FPZ X; return X; }
	FString Dir() { return FPaths::Combine(FPaths::ProjectDir(), TEXT("ExternalAssets/PZ")); }

	bool LoadPZ()
	{
		FPZ& X = PZ();
		if (X.bTried) return X.bOk;
		X.bTried = true;
		auto Json = [](const FString& File) -> TSharedPtr<FJsonObject>
		{
			FString S;
			TSharedPtr<FJsonObject> O;
			if (FFileHelper::LoadFileToString(S, *FPaths::Combine(Dir(), File))) FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S), O);
			return O;
		};
		const TSharedPtr<FJsonObject> A = Json(TEXT("atlas.json")), T = Json(TEXT("tiles.json")), P = Json(TEXT("palette.json"));
		if (!A || !T || !P) { UE_LOG(LogTemp, Warning, TEXT("PZ stand-in tiles not found (run Tools/ExtractPZ.py)")); return false; }
		for (const auto& Kv : A->Values)
		{
			const TSharedPtr<FJsonObject> E = Kv.Value->AsObject();
			FPZSprite S;
			S.Page = E->GetStringField(TEXT("page"));
			S.X = E->GetIntegerField(TEXT("x")); S.Y = E->GetIntegerField(TEXT("y")); S.W = E->GetIntegerField(TEXT("w")); S.H = E->GetIntegerField(TEXT("h"));
			S.Ox = E->GetIntegerField(TEXT("ox")); S.Oy = E->GetIntegerField(TEXT("oy"));
			X.Atlas.Add(FString(Kv.Key), S);
		}
		for (const auto& Kv : T->Values) X.Props.Add(FString(Kv.Key), Kv.Value->AsObject());
		for (const auto& Kv : P->Values) { FString V; if (Kv.Value->TryGetString(V)) X.Palette.Add(FString(Kv.Key), V); }
		X.bOk = true;
		return true;
	}

	// The sprite cut out of its atlas page into its own 128x256 frame texture (transparent outside the drawing).
	UTexture2D* Frame(const FString& Name)
	{
		FPZ& X = PZ();
		if (UTexture2D** Found = X.Frames.Find(Name)) return *Found;
		UTexture2D* Tex = nullptr;
		const FPZSprite* S = X.Atlas.Find(Name);
		if (S)
		{
			FImage* Page = X.Pages.Find(S->Page);
			if (!Page)
			{
				FImage Img;
				if (FImageUtils::LoadImage(*FPaths::Combine(Dir(), TEXT("pages"), S->Page), Img)) { Img.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB); Page = &X.Pages.Add(S->Page, MoveTemp(Img)); }
			}
			if (Page)
			{
				Tex = UTexture2D::CreateTransient(128, 256, PF_B8G8R8A8);
				Tex->Filter = TF_Bilinear;
				Tex->SRGB = true;
				uint8* Dst = (uint8*)Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memzero(Dst, 128 * 256 * 4);
				const TArrayView64<FColor> Src = Page->AsBGRA8();
				for (int32 y = 0; y < S->H; ++y)
					for (int32 x = 0; x < S->W; ++x)
					{
						const int32 Fx = S->Ox + x, Fy = S->Oy + y, Px = S->X + x, Py = S->Y + y;
						if (Fx < 0 || Fx >= 128 || Fy < 0 || Fy >= 256 || Px >= Page->SizeX || Py >= Page->SizeY) continue;
						FMemory::Memcpy(Dst + (Fy * 128 + Fx) * 4, &Src[(int64)Py * Page->SizeX + Px], 4);
					}
				Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
				Tex->UpdateResource();
				Tex->AddToRoot();
			}
		}
		X.Frames.Add(Name, Tex);
		return Tex;
	}

	bool Has(const FString& Name, const TCHAR* Prop)
	{
		const TSharedPtr<FJsonObject>* P = PZ().Props.Find(Name);
		return P && (*P)->HasField(Prop);
	}
	FString Prop(const FString& Name, const TCHAR* Key)
	{
		const TSharedPtr<FJsonObject>* P = PZ().Props.Find(Name);
		FString V;
		if (P) (*P)->TryGetStringField(Key, V);
		return V;
	}
}

FString AHollowGameMode::PZPalette(const FString& Key) const { return LoadPZ() ? PZ().Palette.FindRef(Key) : FString(); }

// One quad of a PZ frame mapped onto 4 world points (UVs in frame pixels: 128 wide, 256 tall).
void AHollowGameMode::PZQuad(AActor* A, const FString& Sprite, const FVector P[4], const FVector2D Uv[4])
{
	UTexture2D* T = Frame(Sprite);
	if (!A || !T || !SpriteMat) return;
	UProceduralMeshComponent* M = NewObject<UProceduralMeshComponent>(A);
	M->SetupAttachment(A->GetRootComponent());
	M->RegisterComponent();
	A->AddInstanceComponent(M);
	const FVector O = A->GetActorLocation();
	TArray<FVector> V = { P[0] - O, P[1] - O, P[2] - O, P[3] - O };
	TArray<FVector2D> U = { Uv[0] / FVector2D(128, 256), Uv[1] / FVector2D(128, 256), Uv[2] / FVector2D(128, 256), Uv[3] / FVector2D(128, 256) };
	TArray<int32> Tri = { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 };   // both faces
	M->CreateMeshSection(0, V, Tri, {}, U, {}, {}, false);
	UMaterialInstanceDynamic* Mi = UMaterialInstanceDynamic::Create(SpriteMat, A);
	Mi->SetTextureParameterValue(TEXT("Tex"), T);
	Mi->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.53f, 0.6f, 1.f));
	M->SetMaterial(0, Mi);
}

// A small PZ-style cottage stamped onto the island: squares of Sq cm, walls on N/W edges, door to the south.
void AHollowGameMode::BuildPZCottage(const FVector& Center, int32 Seed)
{
	if (!LoadPZ() || !SpriteMat) return;
	UWorld* W = GetWorld();
	constexpr float Sq = 150.f, WallH = 300.f;
	const int32 Nx = 6, Ny = 5;   // squares along PZ x (east) and y (south)
	const FVector O = Center + FVector(Ny * Sq * 0.5f, -Nx * Sq * 0.5f, 4.f);   // world position of PZ corner (0,0)
	auto Corner = [&](float cx, float cy, float z = 0.f) { return O + FVector(-cy * Sq, cx * Sq, z); };
	AActor* A = Local(HollowArt::NewActor(W, Center));
	const FString Floor = PZPalette(TEXT("floorWood"));
	const FVector2D FUv[4] = { { 64, 192 }, { 128, 224 }, { 64, 256 }, { 0, 224 } };
	const FVector2D NUv[4] = { { 64, 192 }, { 128, 224 }, { 128, 32 }, { 64, 0 } };
	const FVector2D WUv[4] = { { 64, 192 }, { 0, 224 }, { 0, 32 }, { 64, 0 } };
	auto FloorAt = [&](int32 x, int32 y, const FString& S) { const FVector P[4] = { Corner(x, y), Corner(x + 1, y), Corner(x + 1, y + 1), Corner(x, y + 1) }; PZQuad(A, S, P, FUv); };
	auto WallN = [&](int32 x, int32 y, const FString& S, bool bSolid)
	{
		const FVector P[4] = { Corner(x, y), Corner(x + 1, y), Corner(x + 1, y, WallH), Corner(x, y, WallH) };
		PZQuad(A, S, P, NUv);
		if (bSolid) HollowArt::Collider(A, (Corner(x, y) + Corner(x + 1, y)) * 0.5f - Center + FVector(0, 0, WallH * 0.5f), FVector(12, Sq, WallH));
	};
	auto WallW = [&](int32 x, int32 y, const FString& S, bool bSolid)
	{
		const FVector P[4] = { Corner(x, y), Corner(x, y + 1), Corner(x, y + 1, WallH), Corner(x, y, WallH) };
		PZQuad(A, S, P, WUv);
		if (bSolid) HollowArt::Collider(A, (Corner(x, y) + Corner(x, y + 1)) * 0.5f - Center + FVector(0, 0, WallH * 0.5f), FVector(Sq, 12, WallH));
	};

	for (int32 y = 0; y < Ny; ++y) for (int32 x = 0; x < Nx; ++x) FloorAt(x, y, Floor);
	const FString Wn = PZPalette(TEXT("wallN")), Ww = PZPalette(TEXT("wallW"));
	const int32 DoorX = Nx / 2;
	for (int32 x = 0; x < Nx; ++x)
	{
		WallN(x, 0, x == 2 ? PZPalette(TEXT("windowN")) : Wn, true);                       // north wall, a window
		if (x == DoorX) { WallN(x, Ny, PZPalette(TEXT("doorframeN")), false); }          // south wall: the door square
		else WallN(x, Ny, Wn, true);
	}
	for (int32 y = 0; y < Ny; ++y)
	{
		WallW(0, y, y == 2 ? PZPalette(TEXT("windowW")) : Ww, true);                      // west wall, a window
		WallW(Nx, y, Ww, true);                                                            // east wall
	}
	// the door itself: drawn in the wall plane, opens (hides) when used
	AActor* Door = Local(HollowArt::NewActor(W, (Corner(DoorX, Ny) + Corner(DoorX + 1, Ny)) * 0.5f));
	{
		const FVector P[4] = { Corner(DoorX, Ny, 1.f), Corner(DoorX + 1, Ny, 1.f), Corner(DoorX + 1, Ny, WallH), Corner(DoorX, Ny, WallH) };
		PZQuad(Door, PZPalette(TEXT("doorN")), P, NUv);
		HollowArt::Collider(Door, FVector(0, 0, WallH * 0.5f), FVector(12, Sq, WallH));
		AddInteract(Door, TEXT("door"), TEXT(""), TEXT("Open the door"), 0, 260.f);
	}

	// furniture keeps its iso drawing on a camera-facing card; containers can be searched
	auto Object = [&](const FString& Sprite, int32 x, int32 y)
	{
		UTexture2D* T = Frame(Sprite);
		if (!T) return;
		const FVector At = Corner(x + 0.5f, y + 0.5f);
		AActor* Ob = Local(HollowArt::NewActor(W, At));
		UMaterialInstanceDynamic* Mi = UMaterialInstanceDynamic::Create(SpriteMat, Ob);
		Mi->SetTextureParameterValue(TEXT("Tex"), T);
		Mi->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.6f, 0.58f, 0.64f, 1.f));
		UMaterialBillboardComponent* B = NewObject<UMaterialBillboardComponent>(Ob);
		B->SetupAttachment(Ob->GetRootComponent());
		B->AddElement(Mi, nullptr, false, Sq * 0.75f, Sq * 1.5f, nullptr);
		B->SetRelativeLocation(FVector(0, 0, Sq * 1.5f - Sq * 0.25f));   // the frame's floor point sits on the square
		B->RegisterComponent();
		Ob->AddInstanceComponent(B);
		if (Has(Sprite, TEXT("solid")) || Has(Sprite, TEXT("solidtrans"))) HollowArt::Collider(Ob, FVector(0, 0, 60), FVector(Sq * 0.7f, Sq * 0.7f, 120), 0.f);
		const FString Cont = Prop(Sprite, TEXT("container"));
		const FString Nm = Prop(Sprite, TEXT("CustomName"));
		if (!Cont.IsEmpty()) AddInteract(Ob, TEXT("search"), Cont, FString::Printf(TEXT("Search the %s"), *(Nm.IsEmpty() ? Cont : Nm.ToLower())), 0, 220.f);
	};
	Object(PZPalette(TEXT("bed")), 1, 1);
	Object(PZPalette(TEXT("table")), 4, 2);
	Object(PZPalette(TEXT("chair")), 3, 2);
	Object(PZPalette(TEXT("stove")), 5, 0);
	Object(TEXT("furniture_storage_01_16"), 0, 3);   // an oak wardrobe (container)
	(void)Seed;
}

void AHollowGameMode::SearchContainer(const FInteractable& I)
{
	FString Msg = FString::Printf(TEXT("You search the %s."), *I.Id);
	int32 Silver = HollowRules::D(6);
	PC.Gold += Silver;
	Msg += FString::Printf(TEXT(" %d silver"), Silver);
	if (FMath::FRand() < 0.6f)
	{
		const FString Lib = RollLoot(HexLevel(Pos), FMath::Rand());
		if (const FItemDef* D = HollowItems::Find(Lib)) { Give(Lib, 1); Msg += TEXT(" and ") + D->Name; }
	}
	Toast(Msg + TEXT("."));
	Log(Msg + TEXT("."));
	PlaySfx(TEXT("coins"), 0.6f);
	Interactables.RemoveAll([&](const FInteractable& X) { return X.Actor == I.Actor && X.Kind == TEXT("search"); });
	NearIdx = INDEX_NONE;
	MarkUI();
}

/* ---------- Whole buildings lifted from PZ map cells (Tools/PZLots.py) ---------- */

namespace
{
	TArray<FString> LotFiles()
	{
		TArray<FString> F;
		IFileManager::Get().FindFiles(F, *FPaths::Combine(Dir(), TEXT("lots"), TEXT("*.json")), true, false);
		F.Sort();
		return F;
	}
	bool AnyOf(const FString& T, std::initializer_list<const TCHAR*> Keys) { for (const TCHAR* K : Keys) if (Has(T, K)) return true; return false; }
}

// Stamp a lot (squares with PZ tile stacks, named rooms) onto the island, clipped to the hex. Floors are merged into
// one mesh; every wall face is its own piece so the cutaway can hide the ones between you and the camera.
void AHollowGameMode::BuildPZLot(const FVector& Center, int32 Seed)
{
	if (!LoadPZ() || !SpriteMat) return;
	const TArray<FString> Files = LotFiles();
	if (!Files.Num()) { BuildPZCottage(Center, Seed); return; }
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *FPaths::Combine(Dir(), TEXT("lots"), Files[FMath::Abs(Seed) % Files.Num()]))) return;
	TSharedPtr<FJsonObject> Lot;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Lot) || !Lot) return;

	UWorld* W = GetWorld();
	const float Sq = PZSquare, WallH = PZSquare * 2.6f;
	const int32 Lw = Lot->GetIntegerField(TEXT("w")), Lh = Lot->GetIntegerField(TEXT("h"));
	const FVector O = Center + FVector(Lh * Sq * 0.5f, -Lw * Sq * 0.5f, 3.f);
	auto Corner = [&](float cx, float cy, float z = 0.f) { return O + FVector(-cy * Sq, cx * Sq, z); };
	const float Keep = LocalRadius * 0.8f;
	AActor* Ground = Local(HollowArt::NewActor(W, Center));
	AActor* Walls = Local(HollowArt::NewActor(W, Center));
	LotActors.Add(Ground); LotActors.Add(Walls);
	LotBounds.Add(FBox2D(FVector2D(Corner(0, Lh)), FVector2D(Corner(Lw, 0))));

	// shared materials, one per sprite
	TMap<FString, UMaterialInstanceDynamic*> Mats;
	auto Mat = [&](const FString& S, float Shade) -> UMaterialInstanceDynamic*
	{
		if (UMaterialInstanceDynamic** M = Mats.Find(S)) return *M;
		UTexture2D* T = Frame(S);
		if (!T) return nullptr;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(SpriteMat, Ground);
		M->SetTextureParameterValue(TEXT("Tex"), T);
		M->SetVectorParameterValue(TEXT("Color"), FLinearColor(Shade, Shade * 0.97f, Shade * 1.05f, 1.f));
		Mats.Add(S, M);
		return M;
	};
	struct FSec { TArray<FVector> V; TArray<FVector2D> U; TArray<int32> T; };
	TMap<FString, FSec> Floors;
	const FVector2D FUv[4] = { { 64, 192 }, { 128, 224 }, { 64, 256 }, { 0, 224 } };
	const FVector2D NUv[4] = { { 64, 192 }, { 128, 224 }, { 128, 32 }, { 64, 0 } };
	const FVector2D WUv[4] = { { 64, 192 }, { 0, 224 }, { 0, 32 }, { 64, 0 } };
	auto Face = [&](const FString& S, const FVector P[4], const FVector2D Uv[4], bool bSolid, bool bNorth, float Lift)
	{
		UMaterialInstanceDynamic* M = Mat(S, 0.55f);
		if (!M) return;
		UProceduralMeshComponent* Pm = NewObject<UProceduralMeshComponent>(Walls);
		Pm->SetupAttachment(Walls->GetRootComponent());
		Pm->RegisterComponent();
		Walls->AddInstanceComponent(Pm);
		const FVector Off = bNorth ? FVector(-Lift, 0, 0) : FVector(0, Lift, 0);   // doors and windows sit a hair inside the wall
		TArray<FVector> V = { P[0] - Center + Off, P[1] - Center + Off, P[2] - Center + Off, P[3] - Center + Off };
		TArray<FVector2D> U = { Uv[0] / FVector2D(128, 256), Uv[1] / FVector2D(128, 256), Uv[2] / FVector2D(128, 256), Uv[3] / FVector2D(128, 256) };
		Pm->CreateMeshSection(0, V, { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 }, {}, U, {}, {}, false);
		Pm->SetMaterial(0, M);
		Pm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CutWalls.Add({ Pm, (P[0] + P[1]) * 0.5f });
		if (bSolid)
		{
			const FVector Mid = (P[0] + P[1]) * 0.5f - Center + FVector(0, 0, WallH * 0.5f);
			HollowArt::Collider(Walls, Mid, bNorth ? FVector(10, Sq, WallH) : FVector(Sq, 10, WallH));
		}
	};

	int32 Objects = 0;
	const TArray<TSharedPtr<FJsonValue>>* Squares = nullptr;
	if (!Lot->TryGetArrayField(TEXT("squares"), Squares)) return;
	for (const TSharedPtr<FJsonValue>& Sv : *Squares)
	{
		const TSharedPtr<FJsonObject> S = Sv->AsObject();
		const int32 x = S->GetIntegerField(TEXT("x")), y = S->GetIntegerField(TEXT("y")), z = S->GetIntegerField(TEXT("z"));
		if (z != 0) continue;   // ground floor only for now; upper floors and roofs come with level switching
		const FVector Mid = Corner(x + 0.5f, y + 0.5f);
		if (FVector::Dist2D(Mid, LocalOrigin) > Keep) continue;   // stay inside the hex
		const TArray<TSharedPtr<FJsonValue>>* Tiles = nullptr;
		if (!S->TryGetArrayField(TEXT("tiles"), Tiles)) continue;
		for (const TSharedPtr<FJsonValue>& Tv : *Tiles)
		{
			const FString T = Tv->AsString();
			if (T.StartsWith(TEXT("overlay_grime")) || T.StartsWith(TEXT("d_")) || T.StartsWith(TEXT("roofs_")) || T.Contains(TEXT("snow")) || T.StartsWith(TEXT("location_trailer"))) continue;
			const FPZSprite* Sp = PZ().Atlas.Find(T);
			if (!Sp) continue;
			const FVector Pn[4] = { Corner(x, y), Corner(x + 1, y), Corner(x + 1, y, WallH), Corner(x, y, WallH) };
			const FVector Pw[4] = { Corner(x, y), Corner(x, y + 1), Corner(x, y + 1, WallH), Corner(x, y, WallH) };
			if (Sp->Page.StartsWith(TEXT("f_")) || T.StartsWith(TEXT("floors_")) || T.StartsWith(TEXT("blends_")))
			{
				FSec& F = Floors.FindOrAdd(T);
				const int32 B = F.V.Num();
				const FVector P[4] = { Corner(x, y), Corner(x + 1, y), Corner(x + 1, y + 1), Corner(x, y + 1) };
				for (int32 k = 0; k < 4; ++k) { F.V.Add(P[k] - Center); F.U.Add(FUv[k] / FVector2D(128, 256)); }
				F.T.Append({ B, B + 2, B + 1, B, B + 3, B + 2 });
				continue;
			}
			const bool bN = AnyOf(T, { TEXT("WallN"), TEXT("WallNTrans"), TEXT("DoorWallN"), TEXT("DoorWallNTrans"), TEXT("doorFrN"), TEXT("WindowN"), TEXT("HoppableN"), TEXT("TallHoppableN") });
			const bool bW = AnyOf(T, { TEXT("WallW"), TEXT("WallWTrans"), TEXT("DoorWallW"), TEXT("DoorWallWTrans"), TEXT("doorFrW"), TEXT("WindowW"), TEXT("HoppableW"), TEXT("TallHoppableW") });
			const bool bNW = Has(T, TEXT("WallNW"));
			if (bNW || bN || bW)
			{
				const bool bFrame = AnyOf(T, { TEXT("DoorWallN"), TEXT("DoorWallW"), TEXT("doorFrN"), TEXT("doorFrW") });
				if (bNW || bN) Face(T, Pn, NUv, !bFrame, true, 0.f);
				if (bNW || bW) Face(T, Pw, WUv, !bFrame, false, 0.f);
				continue;
			}
			// doors, windows and wall-hung things are drawn in their wall plane
			const FString Facing = Prop(T, TEXT("Facing"));
			const bool bWindow = Prop(T, TEXT("MoveType")) == TEXT("Window");
			if (Has(T, TEXT("doorN")) || Has(T, TEXT("attachedN")) || (bWindow && Facing == TEXT("N"))) { Face(T, Pn, NUv, false, true, 2.f); continue; }
			if (Has(T, TEXT("doorW")) || Has(T, TEXT("attachedW")) || (bWindow && Facing == TEXT("W"))) { Face(T, Pw, WUv, false, false, 2.f); continue; }
			// everything else stands on its square as a paper card
			UMaterialInstanceDynamic* M = Mat(T, 0.6f);
			if (!M || Objects > 450) continue;
			AActor* Ob = Local(HollowArt::NewActor(W, Mid));
			LotActors.Add(Ob);
			UMaterialBillboardComponent* Bb = NewObject<UMaterialBillboardComponent>(Ob);
			Bb->SetupAttachment(Ob->GetRootComponent());
			Bb->AddElement(M, nullptr, false, Sq * 0.75f, Sq * 1.5f, nullptr);
			Bb->SetRelativeLocation(FVector(0, 0, Sq * 1.25f));
			Bb->RegisterComponent();
			Ob->AddInstanceComponent(Bb);
			Objects++;
			if (Has(T, TEXT("solid")) || Has(T, TEXT("solidtrans"))) HollowArt::Collider(Ob, FVector(0, 0, 50), FVector(Sq * 0.7f, Sq * 0.7f, 100), 0.f);
			const FString Cont = Prop(T, TEXT("container"));
			if (!Cont.IsEmpty())
			{
				const FString Nm = Prop(T, TEXT("CustomName"));
				AddInteract(Ob, TEXT("search"), Cont, FString::Printf(TEXT("Search the %s"), *(Nm.IsEmpty() ? Cont : Nm.ToLower())), 0, 170.f);
			}
		}
	}
	UProceduralMeshComponent* Gm = NewObject<UProceduralMeshComponent>(Ground);
	Gm->SetupAttachment(Ground->GetRootComponent());
	Gm->RegisterComponent();
	Ground->AddInstanceComponent(Gm);
	Gm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	int32 Sec = 0;
	for (auto& F : Floors)
	{
		Gm->CreateMeshSection(Sec, F.Value.V, F.Value.T, {}, F.Value.U, {}, {}, false);
		Gm->SetMaterial(Sec++, Mat(F.Key, 0.5f));
	}
	FString Src;
	Lot->TryGetStringField(TEXT("source"), Src);
	UE_LOG(LogTemp, Display, TEXT("PZ lot %s: %d floor kinds, %d wall faces, %d objects"), *Src, Floors.Num(), CutWalls.Num(), Objects);
}

// Cutaway, as in Zomboid: walls between you and the camera go see-through so you can always see yourself indoors.
void AHollowGameMode::TickCutaway()
{
	if (!CutWalls.Num() || !Hero || Mode != EHollowMode::Explore) return;
	APlayerController* P = GetWorld()->GetFirstPlayerController();
	if (!P || !P->PlayerCameraManager) return;
	const FVector2D Me(Hero->GetActorLocation()), Cam(P->PlayerCameraManager->GetCameraLocation());
	const FVector2D Fwd = (Me - Cam).GetSafeNormal();
	bool bInside = false;
	for (const FBox2D& B : LotBounds) bInside |= B.IsInside(Me);
	for (FLotWall& Wl : CutWalls)
	{
		UPrimitiveComponent* C = Wl.C.Get();
		if (!C) continue;
		const FVector2D Wp(Wl.P);
		const float Ahead = FVector2D::DotProduct(Wp - Me, Fwd);          // below zero: on the camera side of you
		const float Side = FMath::Abs(FVector2D::CrossProduct(Fwd, Wp - Me));
		const bool bHide = Ahead < -30.f && (bInside ? FVector2D::Distance(Wp, Me) < 1300.f : Side < 260.f && -Ahead < 1200.f);
		if (C->IsVisible() == bHide) C->SetVisibility(!bHide);
	}
}
