// Atmosphere, audio, sprite FX, Niagara and the local-LLM NPC brain for AHollowGameMode.
#include "HollowGameMode.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "HAL/PlatformProcess.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Camera/CameraActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Sound/SoundWave.h"
#include "HollowArt.h"
#include "HollowHero.h"

double AHollowGameMode::Now() const { return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0; }

/* ---------- Atmosphere ---------- */

void AHollowGameMode::BuildAtmosphere()
{
	UWorld* W = GetWorld();
	AActor* Sky = HollowArt::NewActor(W, FVector::ZeroVector);
	KeepAlive.Add(Sky);

	UDirectionalLightComponent* Moon = NewObject<UDirectionalLightComponent>(Sky);
	Moon->SetupAttachment(Sky->GetRootComponent());
	Moon->SetWorldRotation(FRotator(-16.f, 135.f, 0.f));
	Moon->SetIntensity(4.f);
	Moon->SetLightColor(FLinearColor(0.55f, 0.5f, 1.f));
	Moon->SetAtmosphereSunLight(true);
	Moon->SetCastShadows(true);
	Moon->SetVolumetricScatteringIntensity(1.5f);
	Moon->RegisterComponent();

	USkyAtmosphereComponent* Atmo = NewObject<USkyAtmosphereComponent>(Sky);
	Atmo->SetupAttachment(Sky->GetRootComponent());
	Atmo->RegisterComponent();
	Atmo->SetRayleighScattering(FLinearColor(0.12f, 0.1f, 0.3f));
	Atmo->SetMieScatteringScale(0.02f);

	USkyLightComponent* Amb = NewObject<USkyLightComponent>(Sky);
	Amb->SetupAttachment(Sky->GetRootComponent());
	Amb->bRealTimeCapture = true;
	Amb->SourceType = SLS_CapturedScene;
	Amb->bLowerHemisphereIsBlack = false;
	Amb->SetLowerHemisphereColor(FLinearColor(0.02f, 0.02f, 0.035f));
	Amb->SetIntensity(1.0f);
	Amb->SetLightColor(FLinearColor(0.55f, 0.6f, 0.9f));
	Amb->RegisterComponent();

	UVolumetricCloudComponent* Cloud = NewObject<UVolumetricCloudComponent>(Sky);
	Cloud->SetupAttachment(Sky->GetRootComponent());
	if (UMaterialInterface* CM = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst")))
		Cloud->SetMaterial(CM);
	Cloud->SetLayerBottomAltitude(3.f);
	Cloud->SetLayerHeight(6.f);
	Cloud->RegisterComponent();

	UExponentialHeightFogComponent* Fog = NewObject<UExponentialHeightFogComponent>(Sky);
	Fog->SetupAttachment(Sky->GetRootComponent());
	Fog->SetRelativeLocation(FVector(0, 0, -200.f));
	Fog->SetFogDensity(0.012f);
	Fog->SetFogHeightFalloff(0.45f);
	Fog->SetFogInscatteringColor(FLinearColor(0.035f, 0.03f, 0.06f));
	Fog->SetVolumetricFog(true);
	Fog->SetVolumetricFogScatteringDistribution(0.55f);
	Fog->SetVolumetricFogExtinctionScale(0.55f);
	Fog->SetVolumetricFogAlbedo(FColor(120, 115, 140));
	Fog->SetVolumetricFogDistance(9000.f);
	Fog->RegisterComponent();

	// The Sleeping Star, seen from every camera that looks up.
	for (const FVector& Base : { LocalOrigin, ArenaOrigin, BoardOrigin })
		KeepAlive.Add(HollowArt::SkyGod(W, Base + FVector(260000.f, 120000.f, 110000.f), 22.f));
}

/* ---------- Audio ---------- */

void AHollowGameMode::PlaySfx(const FString& Name, float Volume)
{
	static const TMap<FString, TArray<FString>> Banks = {
		{ TEXT("click"), { TEXT("click_002"), TEXT("click_003") } },
		{ TEXT("select"), { TEXT("select_003"), TEXT("select_005") } },
		{ TEXT("confirm"), { TEXT("confirmation_002") } },
		{ TEXT("error"), { TEXT("error_004") } },
		{ TEXT("open"), { TEXT("open_002") } },
		{ TEXT("close"), { TEXT("close_002") } },
		{ TEXT("card"), { TEXT("bookFlip1"), TEXT("bookFlip2"), TEXT("bookFlip3") } },
		{ TEXT("draw"), { TEXT("cloth1"), TEXT("cloth2"), TEXT("cloth3") } },
		{ TEXT("hit"), { TEXT("impactPunch_heavy_000"), TEXT("impactPunch_heavy_001"), TEXT("impactPunch_heavy_002"), TEXT("impactGeneric_light_000") } },
		{ TEXT("metal"), { TEXT("impactMetal_heavy_000"), TEXT("impactMetal_heavy_001"), TEXT("impactMetal_heavy_002") } },
		{ TEXT("swing"), { TEXT("drawKnife1"), TEXT("drawKnife2"), TEXT("knifeSlice"), TEXT("knifeSlice2") } },
		{ TEXT("magic"), { TEXT("glass_001"), TEXT("glass_003"), TEXT("glass_005") } },
		{ TEXT("coins"), { TEXT("handleCoins"), TEXT("handleCoins2") } },
		{ TEXT("creak"), { TEXT("creak1"), TEXT("creak2"), TEXT("creak3") } },
		{ TEXT("door"), { TEXT("doorOpen_1"), TEXT("doorOpen_2") } },
		{ TEXT("step"), { TEXT("footstep_grass_000"), TEXT("footstep_grass_001"), TEXT("footstep_grass_002"), TEXT("footstep_grass_003"), TEXT("footstep_grass_004") } },
		{ TEXT("victory"), { TEXT("jingles_PIZZI03") } },
		{ TEXT("levelup"), { TEXT("jingles_PIZZI10") } },
		{ TEXT("defeat"), { TEXT("jingles_HIT05") } },
		{ TEXT("book"), { TEXT("bookOpen") } },
	};
	const TArray<FString>* Bank = Banks.Find(Name);
	const FString Asset = Bank ? (*Bank)[FMath::RandRange(0, Bank->Num() - 1)] : Name;
	static TMap<FString, TWeakObjectPtr<USoundBase>> Cache;
	USoundBase* S = Cache.FindRef(Asset).Get();
	if (!S)
	{
		S = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Hollow/Audio/Sfx/%s.%s"), *Asset, *Asset));
		if (!S) return;
		Cache.Add(Asset, S);
	}
	UGameplayStatics::PlaySound2D(this, S, Volume, FMath::FRandRange(0.93f, 1.07f));
}

void AHollowGameMode::PlayMusic(const FString& Track, float Volume)
{
	if (Track == MusicTrack && Music) { Music->AdjustVolume(1.5f, Volume); return; }
	MusicTrack = Track;
	if (Music) Music->FadeOut(2.f, 0.f);
	USoundWave* Wave = LoadObject<USoundWave>(nullptr, *FString::Printf(TEXT("/Game/Hollow/Audio/Music/%s.%s"), *Track, *Track));
	if (!Wave) { Music = nullptr; return; }
	Wave->bLooping = true;
	Music = UGameplayStatics::CreateSound2D(this, Wave, Volume, 1.f, 0.f, nullptr, true, false);
	if (Music) Music->FadeIn(2.5f, Volume);
}

/* ---------- Sprite FX ---------- */

void AHollowGameMode::Burst(const FVector& At, const FString& Tex, const FLinearColor& Col, int32 Count, float Speed, float Life, float Size, float Up, float Grav, float SizeEnd)
{
	if (!SpriteMat) return;
	if (!FxActor) { FxActor = HollowArt::NewActor(GetWorld(), FVector::ZeroVector); }
	UTexture2D* T = TexCache.FindRef(Tex).Get();
	if (!T)
	{
		T = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Hollow/FX/Sprites/%s.%s"), *Tex, *Tex));
		if (!T) return;
		TexCache.Add(Tex, T);
		KeepAlive.Add(T);
	}
	static TWeakObjectPtr<UStaticMesh> Plane;
	if (!Plane.IsValid()) Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	for (int32 n = 0; n < Count; ++n)
	{
		FSprite* S = Sprites.FindByPredicate([](const FSprite& X) { return !X.bActive && X.C.IsValid(); });
		if (!S)
		{
			if (Sprites.Num() >= 400) return;
			FSprite NewS;
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(FxActor);
			C->SetStaticMesh(Plane.Get());
			C->SetupAttachment(FxActor->GetRootComponent());
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			C->SetUsingAbsoluteLocation(true); C->SetUsingAbsoluteRotation(true); C->SetUsingAbsoluteScale(true);
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(SpriteMat, this);
			FxMats.Add(M);
			C->SetMaterial(0, M);
			C->RegisterComponent();
			NewS.C = C; NewS.M = M;
			S = &Sprites.Add_GetRef(NewS);
		}
		S->bActive = true;
		S->Age = 0.f;
		S->Life = Life * FMath::FRandRange(0.7f, 1.2f);
		S->P = At;
		S->V = FMath::VRand() * Speed * FMath::FRandRange(0.4f, 1.f) + FVector(0, 0, Up);
		S->Size0 = Size * FMath::FRandRange(0.7f, 1.3f);
		S->Size1 = SizeEnd >= 0.f ? SizeEnd : S->Size0 * 0.3f;
		S->Spin = FMath::FRandRange(-180.f, 180.f);
		S->Rot = FMath::FRandRange(0.f, 360.f);
		S->Grav = Grav;
		S->Col = Col;
		S->M->SetTextureParameterValue(TEXT("Tex"), T);
		S->M->SetVectorParameterValue(TEXT("Color"), Col);
		S->C->SetVisibility(true);
	}
}

void AHollowGameMode::TickFx(float Dt)
{
	FVector Cam = FVector::ZeroVector;
	if (APlayerController* P = GetWorld()->GetFirstPlayerController()) if (P->PlayerCameraManager) Cam = P->PlayerCameraManager->GetCameraLocation();
	for (FSprite& S : Sprites)
	{
		if (!S.bActive || !S.C.IsValid()) continue;
		S.Age += Dt;
		if (S.Age >= S.Life) { S.bActive = false; S.C->SetVisibility(false); continue; }
		const float U = S.Age / S.Life;
		S.V.Z -= S.Grav * Dt;
		S.V *= FMath::Max(0.f, 1.f - 1.2f * Dt);
		S.P += S.V * Dt;
		S.Rot += S.Spin * Dt;
		const FVector ToCam = (Cam - S.P).GetSafeNormal();
		const FQuat Q = FQuat(FRotationMatrix::MakeFromZ(ToCam).Rotator()) * FQuat(FVector::UpVector, FMath::DegreesToRadians(S.Rot));
		const float Size = FMath::Lerp(S.Size0, S.Size1, U);
		S.C->SetWorldLocationAndRotation(S.P, Q);
		S.C->SetWorldScale3D(FVector(Size / 100.f));
		S.M->SetScalarParameterValue(TEXT("Opacity"), FMath::Sin(FMath::Min(1.f, U * 4.f) * PI * 0.5f) * (1.f - U));
	}
}

void AHollowGameMode::NiagaraAt(const FString& System, const FVector& At, float Scale)
{
	static TMap<FString, TWeakObjectPtr<UNiagaraSystem>> Cache;
	UNiagaraSystem* S = Cache.FindRef(System).Get();
	if (!S)
	{
		S = System.StartsWith(TEXT("/")) ? LoadObject<UNiagaraSystem>(nullptr, *(System + TEXT(".") + FPaths::GetBaseFilename(System)))
			: LoadObject<UNiagaraSystem>(nullptr, *FString::Printf(TEXT("/Niagara/DefaultAssets/Templates/Systems/%s.%s"), *System, *System));
		if (!S) return;
		Cache.Add(System, S);
		KeepAlive.Add(S);
	}
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), S, At, FRotator::ZeroRotator, FVector(Scale));
}

// Drifting motes around the hero and sparkles over points of interest.
void AHollowGameMode::TickAmbience(float Dt)
{
	MoteClock += Dt;
	if (MoteClock < 0.12f) return;
	MoteClock = 0.f;
	if (Mode == EHollowMode::Explore && Hero)
	{
		const FVector H = Hero->GetActorLocation();
		const FVector P = H + FVector(FMath::FRandRange(-1400.f, 1400.f), FMath::FRandRange(-1400.f, 1400.f), FMath::FRandRange(0.f, 250.f));
		const float R = FMath::FRand();
		const FLinearColor C = R < 0.4f ? FLinearColor(0.55f, 0.3f, 1.f) : R < 0.7f ? FLinearColor(0.35f, 1.f, 0.55f) : FLinearColor(1.f, 0.7f, 0.3f);
		Burst(P, TEXT("light_01"), C * 0.35f, 1, 15.f, 5.f, 30.f, 22.f, 0.f, 18.f);
		for (const FInteractable& I : Interactables)
			if (I.Kind == TEXT("event") && FMath::FRand() < 0.35f)
				Burst(I.Loc + FVector(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), 30.f), TEXT("star_06"), FLinearColor(1.f, 0.75f, 0.35f) * 0.5f, 1, 20.f, 1.6f, 26.f, 90.f);
	}
	else if (Mode == EHollowMode::Combat)
	{
		const FVector P = ArenaOrigin + FVector(FMath::FRandRange(-1200.f, 1200.f), FMath::FRandRange(-900.f, 400.f), FMath::FRandRange(0.f, 300.f));
		Burst(P, TEXT("light_01"), FLinearColor(0.5f, 0.3f, 1.f) * 0.3f, 1, 10.f, 5.f, 34.f, 15.f, 0.f, 20.f);
	}
}

void AHollowGameMode::OnFootstep(const FVector& At)
{
	if (Mode != EHollowMode::Explore) return;
	PlaySfx(TEXT("step"), 0.35f);
	NiagaraAt(TEXT("/Game/NiagaraExamples/FX_Footstep/NS_Footstep_Gravel"), At + FVector(0, 0, 4), 1.f);
	Burst(At + FVector(0, 0, 10), TEXT("smoke_04"), FLinearColor(0.25f, 0.22f, 0.18f) * 0.15f, 1, 40.f, 0.9f, 40.f, 30.f, 0.f, 90.f);
}

void AHollowGameMode::Zoom(float Delta)
{
	if (Mode == EHollowMode::Combat) ZoomArena(Delta);
	else if (Hero) Hero->ZoomBy(Delta);
}

/* ---------- Local LLM brain (Ollama) ---------- */

void AHollowGameMode::ProbeBrain(bool bLaunch)
{
	TWeakObjectPtr<AHollowGameMode> Weak = this;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(TEXT("http://127.0.0.1:11434/api/tags"));
	Req->SetVerb(TEXT("GET"));
	Req->SetTimeout(3.f);
	Req->OnProcessRequestComplete().BindLambda([Weak, bLaunch](FHttpRequestPtr, FHttpResponsePtr Resp, bool bOk)
	{
		AHollowGameMode* G = Weak.Get();
		if (!G) return;
		if (bOk && Resp.IsValid() && Resp->GetResponseCode() == 200)
		{
			const FString Body = Resp->GetContentAsString();
			for (const TCHAR* M : { TEXT("llama3.2"), TEXT("qwen2.5:"), TEXT("qwen2.5"), TEXT("llama3"), TEXT("mistral"), TEXT("phi3") })
				if (Body.Contains(FString::Printf(TEXT("\"name\":\"%s"), M))) { G->BrainModel = FString(M).Replace(TEXT(":"), TEXT("")); break; }
			G->bBrainOnline = !G->BrainModel.IsEmpty();
			G->MarkUI();
			return;
		}
		if (bLaunch)
		{
			// Ollama is installed but not serving: start it quietly and look again.
			const FString Exe = FPlatformMisc::GetEnvironmentVariable(TEXT("LOCALAPPDATA")) / TEXT("Programs/Ollama/ollama.exe");
			if (FPaths::FileExists(Exe))
			{
				FPlatformProcess::CreateProc(*Exe, TEXT("serve"), true, true, true, nullptr, 0, nullptr, nullptr);
				FTimerHandle T;
				G->GetWorldTimerManager().SetTimer(T, FTimerDelegate::CreateLambda([Weak] { if (Weak.IsValid()) Weak->ProbeBrain(false); }), 4.f, false);
			}
		}
	});
	Req->ProcessRequest();
}

namespace
{
	FString JsonEscape(const FString& S)
	{
		FString O;
		O.Reserve(S.Len() + 16);
		for (TCHAR Ch : S)
		{
			switch (Ch)
			{
			case TEXT('"'): O += TEXT("\\\""); break;
			case TEXT('\\'): O += TEXT("\\\\"); break;
			case TEXT('\n'): O += TEXT("\\n"); break;
			case TEXT('\r'): break;
			case TEXT('\t'): O += TEXT(" "); break;
			default: O.AppendChar(Ch); break;
			}
		}
		return O;
	}
}

void AHollowGameMode::AskText(const FString& Text)
{
	if (!Talk.IsSet() || Talk->bThinking) return;
	const FNpcDef* N = HollowData::FindNpc(Talk->NpcId);
	if (!N) return;

	// Keyword pass first: it decides which canned topic (and its mechanics) the question touches.
	const FString Lower = TEXT(" ") + Text.ToLower() + TEXT(" ");
	TArray<FString> Words;
	Text.ToLower().ParseIntoArrayWS(Words, TEXT(",.?!;:'\""));
	FString BestKnown, BestAny;
	int32 BestKnownScore = 0, BestAnyScore = 0;
	for (const FTopicDef& T : HollowData::Topics())
	{
		int32 Score = 0;
		for (const FString& K : T.Keywords)
		{
			if (K.Contains(TEXT(" "))) { if (Lower.Contains(K)) Score += 2; }
			else for (const FString& Wd : Words) if (Wd == K || (K.Len() >= 5 && Wd.StartsWith(K))) { Score++; break; }
		}
		if (Score <= 0) continue;
		if (Score > BestAnyScore) { BestAnyScore = Score; BestAny = T.Id; }
		if (N->Replies.Contains(T.Id) && Score > BestKnownScore) { BestKnownScore = Score; BestKnown = T.Id; }
	}
	Talk->Asked = Text;
	Talk->LeadNote.Reset();
	if (!BestAny.IsEmpty()) KnownTopics.Add(BestAny);

	if (!bBrainOnline)
	{
		if (!BestKnown.IsEmpty()) Reply(*N, BestKnown);
		else { SetTalkLine(N->Unknown); LearnTopics(N->Unknown); }
		MarkUI();
		return;
	}

	// The brain phrases the answer in character, limited to what this NPC knows.
	static FString World;
	if (World.IsEmpty()) FFileHelper::LoadFileToString(World, *(FPaths::ProjectContentDir() / TEXT("Hollow/Lore/World.txt")));
	FString Facts;
	for (const auto& R : N->Replies) Facts += FString::Printf(TEXT("- %s: %s\n"), *R.Key, *StripBrackets(R.Value.Text));
	const FString System = FString::Printf(
		TEXT("You are %s, %s, in Brindlemark. Stay in character. Answer in 1 to 3 short sentences, plain spoken, no stage directions, no lists. ")
		TEXT("Use only facts from WORLD and YOUR KNOWLEDGE. If you don't know, say so in character and point to something you do know. Never mention AI, games or rules.\n")
		TEXT("WORLD:\n%s\nYOUR KNOWLEDGE (topic: fact):\n%s\n")
		TEXT("Respond ONLY with JSON: {\"reply\": \"what you say\", \"topic\": \"the topic id from YOUR KNOWLEDGE the player asked about, or none\"}"),
		*N->Name, *N->Role, *World, *Facts);

	FString Msgs = FString::Printf(TEXT("{\"role\":\"system\",\"content\":\"%s\"}"), *JsonEscape(System));
	for (const auto& H : Talk->History)
		Msgs += FString::Printf(TEXT(",{\"role\":\"user\",\"content\":\"%s\"},{\"role\":\"assistant\",\"content\":\"%s\"}"), *JsonEscape(H.Key), *JsonEscape(H.Value));
	Msgs += FString::Printf(TEXT(",{\"role\":\"user\",\"content\":\"%s\"}"), *JsonEscape(Text));
	const FString Body = FString::Printf(TEXT("{\"model\":\"%s\",\"stream\":false,\"format\":\"json\",\"options\":{\"temperature\":0.7,\"num_predict\":180,\"num_ctx\":3072},\"messages\":[%s]}"), *BrainModel, *Msgs);

	Talk->bThinking = true;
	Talk->Line = FString::Printf(TEXT("%s considers your words..."), *N->Name);
	const int32 Serial = ++Talk->Serial;
	const FString NpcId = N->Id;
	TWeakObjectPtr<AHollowGameMode> Weak = this;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(TEXT("http://127.0.0.1:11434/api/chat"));
	Req->SetVerb(TEXT("POST"));
	Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Req->SetContentAsString(Body);
	Req->SetTimeout(40.f);
	Req->OnProcessRequestComplete().BindLambda([Weak, Serial, NpcId, BestKnown, Text](FHttpRequestPtr, FHttpResponsePtr Resp, bool bOk)
	{
		AHollowGameMode* G = Weak.Get();
		if (!G || !G->Talk.IsSet() || G->Talk->NpcId != NpcId || G->Talk->Serial != Serial) return;
		const FNpcDef* N = HollowData::FindNpc(NpcId);
		G->Talk->bThinking = false;
		FString Line, Topic;
		if (bOk && Resp.IsValid() && Resp->GetResponseCode() == 200)
		{
			TSharedPtr<FJsonObject> Root;
			if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Resp->GetContentAsString()), Root) && Root.IsValid())
			{
				const TSharedPtr<FJsonObject>* Msg;
				if (Root->TryGetObjectField(TEXT("message"), Msg))
				{
					const FString Content = (*Msg)->GetStringField(TEXT("content"));
					TSharedPtr<FJsonObject> Inner;
					if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Content), Inner) && Inner.IsValid())
					{
						Inner->TryGetStringField(TEXT("reply"), Line);
						Inner->TryGetStringField(TEXT("topic"), Topic);
					}
					else Line = Content;
				}
			}
		}
		if (!N) return;
		Topic = Topic.ToLower().TrimStartAndEnd();
		if (!N->Replies.Contains(Topic)) Topic = BestKnown;
		if (Line.TrimStartAndEnd().IsEmpty())
		{
			// brain failed: fall back to the canned library
			G->bBrainOnline = false;
			if (!Topic.IsEmpty()) G->Reply(*N, Topic);
			else G->SetTalkLine(N->Unknown);
		}
		else
		{
			if (!Topic.IsEmpty()) G->Reply(*N, Topic, false);
			G->SetTalkLine(Line.TrimStartAndEnd());
			G->Talk->bFromBrain = true;
		}
		G->Talk->History.Add({ Text, G->Talk->Line });
		if (G->Talk->History.Num() > 4) G->Talk->History.RemoveAt(0);
		G->MarkUI();
	});
	Req->ProcessRequest();
	MarkUI();
}

/* ---------- Fog of war ---------- */

// Low mist covers every wild hex until you walk close; cleared cells stay cleared for that hex.
void AHollowGameMode::BuildFow(FHollowHex& Hex)
{
	Fow.Reset();
	if (Hex.T == ETerr::Town || !SpriteMat) return;
	static TWeakObjectPtr<UStaticMesh> Plane;
	if (!Plane.IsValid()) Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	AActor* A = Local(HollowArt::NewActor(GetWorld(), LocalOrigin));
	const float Ap = LocalRadius * 0.866f, Step = 560.f;
	TArray<FVector> Cells;
	for (float X = -Ap; X <= Ap; X += Step)
		for (float Y = -LocalRadius; Y <= LocalRadius; Y += Step)
		{
			const FVector P(X + (FMath::RoundToInt(Y / Step) % 2) * Step * 0.5f, Y, 0);
			if (FMath::Abs(P.X) <= Ap * 0.98f && FMath::Abs(P.X) * 0.5f + FMath::Abs(P.Y) * 0.866f <= Ap * 0.98f) Cells.Add(P);
		}
	if (Hex.FogCleared.Num() != Cells.Num()) Hex.FogCleared.Init(false, Cells.Num());
	FRandomStream Rng(Hex.Q * 31 + Hex.R * 977);
	for (int32 i = 0; i < Cells.Num(); ++i)
	{
		if (FVector::Dist2D(Cells[i], FVector(-1700.f, 0, 0)) < 900.f) Hex.FogCleared[i] = true;
		if (Hex.FogCleared[i]) continue;
		for (int32 Layer = 0; Layer < 2; ++Layer)
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(A);
			C->SetStaticMesh(Plane.Get());
			C->SetupAttachment(A->GetRootComponent());
			const FVector P = Cells[i] + FVector(Rng.FRandRange(-120.f, 120.f), Rng.FRandRange(-120.f, 120.f), 170.f + Layer * 140.f + Rng.FRandRange(0.f, 60.f));
			C->SetRelativeLocationAndRotation(P, FRotator(0, Rng.FRandRange(0.f, 360.f), 0));
			C->SetRelativeScale3D(FVector(Rng.FRandRange(10.f, 14.f)));
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(SpriteMat, this);
			FxMats.Add(M);
			const FString Tex = FString::Printf(TEXT("smoke_%02d"), Rng.RandRange(1, 10));
			if (UTexture2D* T = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Hollow/FX/Sprites/%s.%s"), *Tex, *Tex))) M->SetTextureParameterValue(TEXT("Tex"), T);
			M->SetVectorParameterValue(TEXT("Color"), Layer ? FLinearColor(0.1f, 0.07f, 0.16f) : FLinearColor(0.09f, 0.09f, 0.12f));
			M->SetScalarParameterValue(TEXT("Opacity"), 1.f);
			C->SetMaterial(0, M);
			C->RegisterComponent();
			FFow F; F.C = C; F.M = M; F.P = Cells[i]; F.Cell = i; F.Phase = Rng.FRandRange(0.f, 6.f); F.Base = Layer ? 0.8f : 1.f;
			Fow.Add(F);
		}
	}
}

void AHollowGameMode::TickFow(float Dt)
{
	if (!Fow.Num()) return;
	FHollowHex* H = Hexes.Find(Pos);
	const FVector Rel = Hero ? Hero->GetActorLocation() - LocalOrigin : FVector(1e9f);
	const float T = float(Now());
	for (FFow& F : Fow)
	{
		if (!F.C.IsValid() || F.Alpha <= 0.f) continue;
		if (!F.bFading && Mode == EHollowMode::Explore && FVector::Dist2D(F.P, Rel) < 760.f)
		{
			F.bFading = true;
			if (H && H->FogCleared.IsValidIndex(F.Cell)) H->FogCleared[F.Cell] = true;
		}
		if (F.bFading)
		{
			F.Alpha -= Dt * 0.9f;
			if (F.Alpha <= 0.f) { F.C->SetVisibility(false); continue; }
		}
		F.M->SetScalarParameterValue(TEXT("Opacity"), F.Alpha * F.Base * (0.82f + 0.18f * FMath::Sin(T * 0.6f + F.Phase)));
	}
}

/* ---------- Title ---------- */

void AHollowGameMode::TickTitle(float Dt)
{
	if (Mode != EHollowMode::Title && Mode != EHollowMode::Create) return;
	if (!ShowCam) return;
	OrbitAngle += Dt * (Mode == EHollowMode::Title ? 2.5f : 1.f);
	const float A = FMath::DegreesToRadians(OrbitAngle);
	const FVector Loc = LocalOrigin + FVector(FMath::Cos(A) * 3300.f, FMath::Sin(A) * 3300.f, 2000.f);
	const FVector Look = LocalOrigin + FVector(0, 0, 150.f);
	ShowCam->SetActorLocationAndRotation(Loc, (Look - Loc).Rotation());
	if (Mode != EHollowMode::Title) return;
	{
		static TWeakObjectPtr<AActor> EmberRig;
		if (!EmberRig.IsValid())
		{
			AActor* Rig = HollowArt::NewActor(GetWorld(), Loc);
			HollowArt::NiagaraPart(Rig, TEXT("/Game/Sparks_Embers/Niagara/NS_Embers01"), FVector::ZeroVector, 3.f);
			HollowArt::NiagaraPart(Rig, TEXT("/Game/Sparks_Embers/Niagara/NS_Embers02"), FVector(0, 400, 0), 3.f);
			EmberRig = Rig;
			KeepAlive.Add(Rig);
		}
		const FRotator CR = ShowCam->GetActorRotation();
		EmberRig->SetActorLocation(Loc + CR.Vector() * 1100.f - FVector(0, 0, 600.f));
	}
	EmberClock += Dt;
	while (EmberClock > 0.05f)
	{
		EmberClock -= 0.05f;
		const FRotator R = ShowCam->GetActorRotation();
		const FVector Fwd = R.Vector(), Right = FRotationMatrix(R).GetUnitAxis(EAxis::Y), Up = FRotationMatrix(R).GetUnitAxis(EAxis::Z);
		const FVector P = Loc + Fwd * FMath::FRandRange(700.f, 1400.f) + Right * FMath::FRandRange(-1100.f, 1100.f) - Up * FMath::FRandRange(300.f, 700.f);
		Burst(P, FMath::FRand() < 0.5f ? TEXT("spark_04") : TEXT("light_01"), FMath::FRand() < 0.85f ? FLinearColor(1.f, 0.42f, 0.1f) : FLinearColor(0.6f, 0.3f, 1.f), 1, 40.f, 4.5f, FMath::FRandRange(8.f, 18.f), 160.f, -15.f, 4.f);
	}
}

void AHollowGameMode::QuitToDesktop()
{
	UKismetSystemLibrary::QuitGame(this, GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}

