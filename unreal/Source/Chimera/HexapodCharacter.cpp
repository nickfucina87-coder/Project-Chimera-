#include "HexapodCharacter.h"
#include "HexapodGravity.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* HexDir = TEXT("/Game/Characters/HexapodV2");
	constexpr float CapsuleSize = 48.f;      // radius == half height: a sphere, so re-orienting never penetrates
	constexpr float FeetBelowOrigin = 30.f;  // mesh feet sit 30 cm below the mesh origin

	template <typename T>
	T* LoadDefault(const FString& Path)
	{
		ConstructorHelpers::FObjectFinder<T> Finder(*Path);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}
	FString Anim(const TCHAR* Name) { return FString::Printf(TEXT("%s/Anims/A_Hex_%s.A_Hex_%s"), HexDir, Name, Name); }
}

AHexapodCharacter::AHexapodCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(CapsuleSize, CapsuleSize);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = false;
	Move->bUseControllerDesiredRotation = true;          // body faces the camera, so strafe/backup clips get used
	Move->RotationRate = FRotator(720.f, 300.f, 720.f);
	Move->MaxWalkSpeed = RunSpeed;
	Move->JumpZVelocity = 520.f;
	Move->AirControl = 0.35f;
	Move->MaxStepHeight = 30.f;
	Move->BrakingDecelerationWalking = 900.f;
	Move->SetWalkableFloorAngle(60.f);

	USkeletalMeshComponent* SK = GetMesh();
	SK->SetSkeletalMeshAsset(LoadDefault<USkeletalMesh>(FString::Printf(
		TEXT("%s/Ironclad_Hexapod_v2_Animated/SkeletalMeshes/Ironclad_Hexapod_v2_Animated.Ironclad_Hexapod_v2_Animated"), HexDir)));
	SK->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleSize + FeetBelowOrigin));
	SK->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));   // mesh faces +Y, actor forward is +X
	SK->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SK->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	SK->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	MeshBaseRotation = SK->GetRelativeRotation().Quaternion();

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 420.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 70.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;
	CameraBoom->ProbeSize = 18.f;                       // fatter probe: pulls in before the lens reaches a wall
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeChannel = ECC_Camera;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	AnimIdle = LoadDefault<UAnimSequence>(Anim(TEXT("Idle")));
	AnimWalk = LoadDefault<UAnimSequence>(Anim(TEXT("Walk_Fwd")));
	AnimRun = LoadDefault<UAnimSequence>(Anim(TEXT("Run_Fwd")));
	AnimSprint = LoadDefault<UAnimSequence>(Anim(TEXT("Sprint_Fwd")));
	AnimGallop = LoadDefault<UAnimSequence>(Anim(TEXT("Gallop_Fwd")));
	AnimStrafeL = LoadDefault<UAnimSequence>(Anim(TEXT("Strafe_L")));
	AnimStrafeR = LoadDefault<UAnimSequence>(Anim(TEXT("Strafe_R")));
	AnimBackup = LoadDefault<UAnimSequence>(Anim(TEXT("Walk_Backup")));
	AnimTurnL = LoadDefault<UAnimSequence>(Anim(TEXT("Turn_L")));
	AnimTurnR = LoadDefault<UAnimSequence>(Anim(TEXT("Turn_R")));
	AnimCeilingIdle = LoadDefault<UAnimSequence>(Anim(TEXT("Idle_Ceiling_Hang")));
	AnimStealth = LoadDefault<UAnimSequence>(Anim(TEXT("Stealth_Crawl")));
	AnimJumpStart = LoadDefault<UAnimSequence>(Anim(TEXT("Jump_Start")));
	AnimFall = LoadDefault<UAnimSequence>(Anim(TEXT("Jump_Fall")));
	AnimLand = LoadDefault<UAnimSequence>(Anim(TEXT("Jump_Land")));
	AnimGrabReach = LoadDefault<UAnimSequence>(Anim(TEXT("Grab_Reach")));
	AnimGrabHold = LoadDefault<UAnimSequence>(Anim(TEXT("Grab_Hold")));
	AnimGrabRelease = LoadDefault<UAnimSequence>(Anim(TEXT("Grab_Release")));
	AnimAttack = LoadDefault<UAnimSequence>(Anim(TEXT("Attack_Stab")));
	AnimUprightStart = LoadDefault<UAnimSequence>(Anim(TEXT("Stand_Upright_Start")));
	AnimUprightLoop = LoadDefault<UAnimSequence>(Anim(TEXT("Stand_Upright_Loop")));
	AnimUprightEnd = LoadDefault<UAnimSequence>(Anim(TEXT("Stand_Upright_End")));
	AnimGrabWalkFwd = LoadDefault<UAnimSequence>(Anim(TEXT("GrabWalk_Fwd")));
	AnimGrabWalkBack = LoadDefault<UAnimSequence>(Anim(TEXT("GrabWalk_Back")));
	AnimGrabStrafeL = LoadDefault<UAnimSequence>(Anim(TEXT("GrabStrafe_L")));
	AnimGrabStrafeR = LoadDefault<UAnimSequence>(Anim(TEXT("GrabStrafe_R")));
	AnimUprightWalkFwd = LoadDefault<UAnimSequence>(Anim(TEXT("Upright_Walk_Fwd")));
	AnimUprightWalkBack = LoadDefault<UAnimSequence>(Anim(TEXT("Upright_Walk_Back")));
	AnimPounce = LoadDefault<UAnimSequence>(Anim(TEXT("Upright_Pounce")));
	AnimBuck = LoadDefault<UAnimSequence>(Anim(TEXT("Upright_Buck")));

	DefaultContext = LoadDefault<UInputMappingContext>(TEXT("/Game/Input/IMC_Default.IMC_Default"));
	MouseLookContext = LoadDefault<UInputMappingContext>(TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"));
	HexContext = LoadDefault<UInputMappingContext>(FString::Printf(TEXT("%s/Input/IMC_Hex.IMC_Hex"), HexDir));
	MoveAction = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	LookAction = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_Look.IA_Look"));
	MouseLookAction = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook"));
	JumpAction = LoadDefault<UInputAction>(TEXT("/Game/Input/Actions/IA_Jump.IA_Jump"));
	auto HexIA = [](const TCHAR* N) { return LoadDefault<UInputAction>(FString::Printf(TEXT("%s/Input/%s.%s"), HexDir, N, N)); };
	SprintAction = HexIA(TEXT("IA_Hex_Sprint"));
	StealthAction = HexIA(TEXT("IA_Hex_Stealth"));
	WalkAction = HexIA(TEXT("IA_Hex_Walk"));
	GrabAction = HexIA(TEXT("IA_Hex_Grab"));
	AttackAction = HexIA(TEXT("IA_Hex_Attack"));
	RearAction = HexIA(TEXT("IA_Hex_Rear"));
	ZoomAction = HexIA(TEXT("IA_Hex_Zoom"));
	BuckAction = HexIA(TEXT("IA_Hex_Buck"));
}

void AHexapodCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultContext) Subsystem->AddMappingContext(DefaultContext, 0);
			if (MouseLookContext) Subsystem->AddMappingContext(MouseLookContext, 0);
			if (HexContext) Subsystem->AddMappingContext(HexContext, 1);
		}
	}
	PlayClip(AnimIdle, true);
}

void AHexapodCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC) return;
	if (MoveAction)
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHexapodCharacter::Move);
		EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &AHexapodCharacter::Move);
	}
	if (LookAction) EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHexapodCharacter::Look);
	if (MouseLookAction) EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AHexapodCharacter::Look);
	if (JumpAction)
	{
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &AHexapodCharacter::JumpPressed);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (SprintAction)
	{
		EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &AHexapodCharacter::SprintOn);
		EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &AHexapodCharacter::SprintOff);
	}
	if (StealthAction) EIC->BindAction(StealthAction, ETriggerEvent::Started, this, &AHexapodCharacter::StealthToggle);
	if (WalkAction)
	{
		EIC->BindAction(WalkAction, ETriggerEvent::Started, this, &AHexapodCharacter::WalkOn);
		EIC->BindAction(WalkAction, ETriggerEvent::Completed, this, &AHexapodCharacter::WalkOff);
	}
	if (GrabAction)
	{
		EIC->BindAction(GrabAction, ETriggerEvent::Started, this, &AHexapodCharacter::GrabPressed);
		EIC->BindAction(GrabAction, ETriggerEvent::Completed, this, &AHexapodCharacter::GrabReleased);
	}
	if (AttackAction) EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AHexapodCharacter::AttackPressed);
	if (RearAction)
	{
		EIC->BindAction(RearAction, ETriggerEvent::Started, this, &AHexapodCharacter::RearPressed);
		EIC->BindAction(RearAction, ETriggerEvent::Completed, this, &AHexapodCharacter::RearReleased);
	}
	if (ZoomAction) EIC->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AHexapodCharacter::Zoom);
	if (BuckAction) EIC->BindAction(BuckAction, ETriggerEvent::Started, this, &AHexapodCharacter::BuckPressed);
}

// ------------------------------------------------------------------ input

void AHexapodCharacter::Move(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
	if (!Controller || MoveInput.IsNearlyZero()) return;

	const FVector Gravity = GetCharacterMovement()->GetGravityDirection();
	const FRotator Relative = HexGravity::ToGravityRelative(Controller->GetControlRotation(), Gravity);
	const FRotator YawOnly(0.f, Relative.Yaw, 0.f);
	const FRotationMatrix Basis(HexGravity::ToGravityWorld(YawOnly, Gravity));
	AddMovementInput(Basis.GetUnitAxis(EAxis::X), MoveInput.Y);
	AddMovementInput(Basis.GetUnitAxis(EAxis::Y), MoveInput.X);
}

void AHexapodCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AHexapodCharacter::JumpPressed()
{
	if (Action != EHexAction::None) return;
	if (!SurfaceUp.Equals(FVector::UpVector, 0.05f))
	{
		// Let go of the wall/ceiling: push off the surface and return to world gravity.
		const FVector PushOff = SurfaceUp * 420.f + FVector::UpVector * 250.f;
		AttachToSurface(FVector::UpVector);
		SurfaceCooldown = 0.5f;
		LaunchCharacter(PushOff, true, true);
		return;
	}
	if (!GetCharacterMovement()->IsMovingOnGround() || PendingJump > 0.f) return;
	// anticipation: crouch and push (Jump_Start frames 0-9), then leave the ground on the push
	if (AnimJumpStart) { PlayClip(AnimJumpStart, false, 1.3f); bJumpStartPlaying = true; PendingJump = (9.f / 24.f) / 1.3f; }
	else Jump();
}

void AHexapodCharacter::GrabPressed()
{
	if (Action != EHexAction::None || !GetCharacterMovement()->IsMovingOnGround() || !AnimGrabReach) return;
	Action = EHexAction::Grabbing;
	bActionHeld = true;
	PlayClip(AnimGrabReach, false);
}

void AHexapodCharacter::GrabReleased() { bActionHeld = false; }

void AHexapodCharacter::AttackPressed()
{
	if (Action == EHexAction::Upright && IsUprightIdleOrWalking() && AnimPounce)
	{
		// big pounce out of the reared-up stance: sink back, then launch on the push frame (13 @ 24 fps)
		Action = EHexAction::OneShot;
		PlayClip(AnimPounce, false);
		PendingPounce = 13.f / 24.f;
		return;
	}
	if (Action != EHexAction::None || !AnimAttack) return;
	Action = EHexAction::OneShot;
	PlayClip(AnimAttack, false);
}

void AHexapodCharacter::BuckPressed()
{
	if (Action == EHexAction::Upright && IsUprightIdleOrWalking() && AnimBuck) PlayClip(AnimBuck, false);
}

void AHexapodCharacter::Zoom(const FInputActionValue& Value)
{
	CameraBoom->TargetArmLength = FMath::Clamp(CameraBoom->TargetArmLength - Value.Get<float>() * ZoomStep, ZoomMin, ZoomMax);
}

bool AHexapodCharacter::IsUprightIdleOrWalking() const
{
	return CurrentClip && (CurrentClip == AnimUprightLoop || CurrentClip == AnimUprightWalkFwd || CurrentClip == AnimUprightWalkBack);
}

bool AHexapodCharacter::IsHoldingIdleOrWalking() const
{
	return CurrentClip && (CurrentClip == AnimGrabHold || CurrentClip == AnimGrabWalkFwd || CurrentClip == AnimGrabWalkBack ||
		CurrentClip == AnimGrabStrafeL || CurrentClip == AnimGrabStrafeR);
}

void AHexapodCharacter::RearPressed()
{
	if (Action != EHexAction::None || !GetCharacterMovement()->IsMovingOnGround() || !AnimUprightStart) return;
	Action = EHexAction::Upright;
	bActionHeld = true;
	PlayClip(AnimUprightStart, false);
}

void AHexapodCharacter::RearReleased() { bActionHeld = false; }

// ------------------------------------------------------------------ tick

void AHexapodCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (Action == EHexAction::Grabbing) Move->MaxWalkSpeed = IsHoldingIdleOrWalking() ? HoldingSpeed : 0.f;
	else if (Action == EHexAction::Upright) Move->MaxWalkSpeed = IsUprightIdleOrWalking() ? UprightSpeed : 0.f;
	else if (Action == EHexAction::OneShot) Move->MaxWalkSpeed = 0.f;
	else Move->MaxWalkSpeed = bStealth ? StealthSpeed : bWalk ? WalkSpeed : bSprint ? SprintSpeed : RunSpeed;

	if (PendingPounce > 0.f)
	{
		PendingPounce -= DeltaSeconds;
		if (PendingPounce <= 0.f)
		{
			PendingPounce = -1.f;
			LaunchCharacter(GetActorForwardVector() * PounceForward + SurfaceUp * PounceUp, true, true);
		}
	}
	if (PendingJump > 0.f)
	{
		PendingJump -= DeltaSeconds;
		if (PendingJump <= 0.f) { PendingJump = -1.f; Jump(); }
	}
	if (bInCorner) UpdateCorner(DeltaSeconds);
	else UpdateSurface(DeltaSeconds);
	UpdateAnimation(DeltaSeconds);

	// Short settle for the instant re-attach cases (jumping onto a wall, letting go).
	const float Alpha = 1.f - FMath::Exp(-DeltaSeconds * 12.f);
	VisualOffset = FQuat::Slerp(VisualOffset, FQuat::Identity, Alpha);
	GetMesh()->SetRelativeRotation(VisualOffset * MeshBaseRotation);
}

// ------------------------------------------------------------------ climbing

bool AHexapodCharacter::Probe(const FVector& From, const FVector& To, FHitResult& OutHit) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HexProbe), false, this);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	return GetWorld()->LineTraceSingleByObjectType(OutHit, From, To, Objects, Params);
}

bool AHexapodCharacter::IsClimbable(const FHitResult& Hit) const
{
	const UPrimitiveComponent* Comp = Hit.GetComponent();
	const AActor* Other = Hit.GetActor();
	if (!Comp || Comp->Mobility == EComponentMobility::Movable) return false;
	if (Other && (Other->ActorHasTag(TEXT("NoClimb")) || Other->IsA<APawn>())) return false;
	return true;
}

void AHexapodCharacter::AttachToSurface(const FVector& NewUp, const FVector* NewLocation)
{
	const FVector OldUp = SurfaceUp;
	const FQuat OldMeshWorld = GetMesh()->GetComponentQuat();

	SurfaceUp = NewUp.GetSafeNormal();
	GetCharacterMovement()->SetGravityDirection(-SurfaceUp);

	// Keep heading: project the current forward onto the new plane. If we were facing straight into the new
	// surface (walked into a wall), the new forward is "up the wall", i.e. the old up.
	FVector Forward = GetActorForwardVector();
	Forward -= SurfaceUp * (Forward | SurfaceUp);
	if (Forward.SizeSquared() < 0.05f)
	{
		Forward = OldUp - SurfaceUp * (OldUp | SurfaceUp);
	}
	Forward = Forward.GetSafeNormal(UE_SMALL_NUMBER, FVector::CrossProduct(SurfaceUp, FVector::RightVector));
	const FQuat NewRotation = FRotationMatrix::MakeFromXZ(Forward, SurfaceUp).ToQuat();

	if (NewLocation)
	{
		SetActorLocationAndRotation(*NewLocation, NewRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		SetActorRotation(NewRotation, ETeleportType::TeleportPhysics);
	}

	// The mesh keeps its old world orientation for a moment and blends into the new one (see Tick).
	VisualOffset = GetActorQuat().Inverse() * OldMeshWorld * MeshBaseRotation.Inverse();
	SurfaceCooldown = 0.35f;
}

void AHexapodCharacter::UpdateSurface(float DeltaSeconds)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	SurfaceCooldown -= DeltaSeconds;

	if (!bCanClimb)
	{
		if (!SurfaceUp.Equals(FVector::UpVector, 0.01f)) AttachToSurface(FVector::UpVector);
		return;
	}

	const float R = GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector Loc = GetActorLocation();
	FVector Planar = GetVelocity() - SurfaceUp * (GetVelocity() | SurfaceUp);

	if (Move->IsMovingOnGround())
	{
		AirTime = 0.f;
		const bool bMoving = Planar.SizeSquared() > 20.f * 20.f && !MoveInput.IsNearlyZero();
		if (!bMoving || SurfaceCooldown > 0.f) return;
		const FVector Forward = Planar.GetSafeNormal();
		FHitResult Hit;

		// Inside corner: something steep is right in front of us -> walk up onto it.
		if (Probe(Loc, Loc + Forward * (R + ProbeAhead), Hit) && (Hit.ImpactNormal | SurfaceUp) < NewSurfaceDot && IsClimbable(Hit))
		{
			// corner line: where the wall meets the plane we stand on
			const FVector FloorPoint = Loc - SurfaceUp * R;
			const FVector Pivot = Hit.ImpactPoint - SurfaceUp * ((Hit.ImpactPoint - FloorPoint) | SurfaceUp);
			BeginCorner(Pivot, Hit.ImpactNormal, Pivot + SurfaceUp * (R + 2.f) + Hit.ImpactNormal * (R + 2.f));
			return;
		}

		// Outside corner: the surface ends just ahead -> look back under the lip for the face and wrap onto it.
		const FVector Ahead = Loc + Forward * (R + 15.f);
		if (!Probe(Ahead, Ahead - SurfaceUp * (R + 60.f), Hit))
		{
			const FVector Below = Ahead - SurfaceUp * (R + 35.f);
			FHitResult Face;
			if (Probe(Below, Below - Forward * (R + 90.f), Face) && FMath::Abs(Face.ImpactNormal | SurfaceUp) < 0.5f && IsClimbable(Face))
			{
				const FVector FloorPoint = Loc - SurfaceUp * R;
				const FVector Pivot = Face.ImpactPoint - SurfaceUp * ((Face.ImpactPoint - FloorPoint) | SurfaceUp);
				BeginCorner(Pivot, Face.ImpactNormal, Pivot + Face.ImpactNormal * (R + 2.f) - SurfaceUp * (R + 10.f));
			}
		}
	}
	else if (Move->IsFalling())
	{
		AirTime += DeltaSeconds;
		// Jumped or fell into a wall: stick to it.
		if (AirTime > 0.08f && SurfaceCooldown <= 0.f && GetVelocity().SizeSquared() > 50.f * 50.f)
		{
			const FVector Dir = GetVelocity().GetSafeNormal();
			FHitResult Hit;
			if (Probe(Loc, Loc + Dir * (R + 25.f), Hit) && IsClimbable(Hit) && (Hit.ImpactNormal | FVector::UpVector) < 0.7f)
			{
				AttachToSurface(Hit.ImpactNormal);
				return;
			}
		}
		// Lost the surface (walked off something with nothing to wrap onto): fall back to world gravity.
		if (AirTime > 0.2f && !SurfaceUp.Equals(FVector::UpVector, 0.01f))
		{
			AttachToSurface(FVector::UpVector);
		}
	}
}

// ------------------------------------------------------------------ animation

void AHexapodCharacter::PlayClip(UAnimationAsset* Asset, bool bLoop, float Rate)
{
	if (!Asset) return;
	GetMesh()->PlayAnimation(Asset, bLoop);
	if (UAnimSingleNodeInstance* Node = GetMesh()->GetSingleNodeInstance())
	{
		Node->SetPlayRate(Rate);
	}
	CurrentClip = Asset;
	bCurrentLoops = bLoop;
}

bool AHexapodCharacter::ClipFinished() const
{
	UAnimSingleNodeInstance* Node = GetMesh()->GetSingleNodeInstance();
	if (!Node) return true;
	return !Node->IsPlaying() || Node->GetCurrentTime() >= Node->GetLength() - 0.02f;
}

void AHexapodCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	bJumpStartPlaying = false;
	if (Action == EHexAction::None && AirTime > 0.45f && AnimLand)
	{
		Action = EHexAction::OneShot;
		PlayClip(AnimLand, false, 1.4f);
	}
}

void AHexapodCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();
}

void AHexapodCharacter::UpdateAnimation(float DeltaSeconds)
{
	const UCharacterMovementComponent* Move = GetCharacterMovement();

	switch (Action)
	{
	case EHexAction::Grabbing:
	{
		if (CurrentClip == AnimGrabReach && ClipFinished()) PlayClip(AnimGrabHold, true);
		if (!IsHoldingIdleOrWalking()) return;
		if (!bActionHeld) { Action = EHexAction::OneShot; PlayClip(AnimGrabRelease, false); return; }
		// front legs keep holding; the other four walk in whatever direction we move
		const FVector V = GetVelocity();
		const float Speed = V.Size();
		UAnimSequence* Want = AnimGrabHold;
		if (Speed > 6.f)
		{
			const float Dir = FMath::RadiansToDegrees(FMath::Atan2(V | GetActorRightVector(), V | GetActorForwardVector()));
			const float A = FMath::Abs(Dir);
			Want = A < 45.f ? AnimGrabWalkFwd : A > 135.f ? AnimGrabWalkBack : Dir > 0.f ? AnimGrabStrafeR : AnimGrabStrafeL;
		}
		if (!Want) Want = AnimGrabHold;
		const float Rate = Want == AnimGrabHold ? 1.f : FMath::Clamp(Speed / 25.f, 0.5f, 2.f);
		if (CurrentClip != Want) PlayClip(Want, true, Rate);
		else if (UAnimSingleNodeInstance* Node = GetMesh()->GetSingleNodeInstance()) Node->SetPlayRate(Rate);
		return;
	}
	case EHexAction::Upright:
	{
		if (CurrentClip == AnimUprightStart && ClipFinished()) PlayClip(AnimUprightLoop, true);
		if (CurrentClip == AnimBuck && ClipFinished()) PlayClip(AnimUprightLoop, true);
		if (!IsUprightIdleOrWalking()) return;
		if (!bActionHeld) { Action = EHexAction::OneShot; PlayClip(AnimUprightEnd, false); return; }
		// shuffle forward/back on the rear four legs while reared up
		const FVector V = GetVelocity();
		const float Speed = V.Size();
		UAnimSequence* Want = AnimUprightLoop;
		if (Speed > 5.f) Want = (V | GetActorForwardVector()) >= 0.f ? AnimUprightWalkFwd : AnimUprightWalkBack;
		if (!Want) Want = AnimUprightLoop;
		const float Rate = Want == AnimUprightLoop ? 1.f : FMath::Clamp(Speed / 14.f, 0.5f, 2.f);
		if (CurrentClip != Want) PlayClip(Want, true, Rate);
		else if (UAnimSingleNodeInstance* Node = GetMesh()->GetSingleNodeInstance()) Node->SetPlayRate(Rate);
		return;
	}
	case EHexAction::OneShot:
		if (!ClipFinished()) return;
		Action = EHexAction::None;
		break;
	default:
		break;
	}

	if (bInCorner)
	{
		if (CurrentClip != AnimWalk) PlayClip(AnimWalk, true, 1.2f);
		return;
	}

	if (bJumpStartPlaying && !ClipFinished()) return;
	bJumpStartPlaying = false;
	if (Move->IsFalling())
	{
		if (CurrentClip != AnimFall) PlayClip(AnimFall, true);
		return;
	}

	// signed turn rate about the surface normal, for turn-in-place
	const FVector Forward = GetActorForwardVector();
	const float TurnRate = DeltaSeconds > 0.f && !LastForward.IsZero()
		? FMath::RadiansToDegrees(FMath::Atan2((LastForward ^ Forward) | SurfaceUp, LastForward | Forward)) / DeltaSeconds : 0.f;
	LastForward = Forward;

	const FVector V = GetVelocity();
	const float Speed = V.Size();
	UAnimSequence* Want = nullptr;
	float Rate = 1.f;
	if (bStealth)
	{
		Want = Speed > 5.f ? AnimStealth : AnimIdle;
		Rate = Speed > 5.f ? FMath::Clamp(Speed / StealthSpeed, 0.5f, 1.5f) : 1.f;
	}
	else if (Speed < 8.f)
	{
		if (FMath::Abs(TurnRate) > 35.f && AnimTurnL && AnimTurnR)
		{
			Want = TurnRate > 0.f ? AnimTurnL : AnimTurnR;
			Rate = FMath::Clamp(FMath::Abs(TurnRate) / 44.f, 0.6f, 2.5f);
		}
		else
		{
			Want = (SurfaceUp.Z < -0.6f && AnimCeilingIdle) ? AnimCeilingIdle : AnimIdle;
		}
	}
	else
	{
		const float Direction = FMath::RadiansToDegrees(FMath::Atan2(V | GetActorRightVector(), V | Forward));
		Want = PickGait(Speed, Direction, Rate);
	}
	if (!Want) return;
	if (CurrentClip != Want) PlayClip(Want, true, Rate);
	else if (UAnimSingleNodeInstance* Node = GetMesh()->GetSingleNodeInstance()) Node->SetPlayRate(Rate);
}

UAnimSequence* AHexapodCharacter::PickGait(float Speed, float Direction, float& OutRate) const
{
	const float A = FMath::Abs(Direction);
	if (A > 135.f) { OutRate = FMath::Clamp(Speed / 30.f, 0.6f, 3.f); return AnimBackup; }
	if (A > 45.f) { OutRate = FMath::Clamp(Speed / 34.f, 0.6f, 3.f); return Direction > 0.f ? AnimStrafeR : AnimStrafeL; }

	// forward gaits: native ground speed of each clip (cm/s) and the speed band it covers (bands overlap = hysteresis)
	struct FGait { UAnimSequence* Clip; float Native, Lo, Hi; };
	const FGait Gaits[] = { { AnimWalk, 38.f, 0.f, 95.f }, { AnimRun, 133.f, 80.f, 185.f }, { AnimSprint, 208.f, 170.f, 262.f }, { AnimGallop, 257.f, 245.f, 1.e6f } };
	for (const FGait& G : Gaits)
	{
		if (G.Clip && G.Clip == CurrentClip && Speed >= G.Lo && Speed <= G.Hi) { OutRate = FMath::Clamp(Speed / G.Native, 0.5f, 1.8f); return G.Clip; }
	}
	for (const FGait& G : Gaits)
	{
		if (G.Clip && Speed <= (G.Hi < 1.e5f ? G.Hi - 10.f : G.Hi)) { OutRate = FMath::Clamp(Speed / G.Native, 0.5f, 1.8f); return G.Clip; }
	}
	OutRate = 1.f;
	return AnimGallop;
}

// ------------------------------------------------------------------ corner crawl

void AHexapodCharacter::BeginCorner(const FVector& Pivot, const FVector& NewUp, const FVector& EndLocation)
{
	const FVector Start = GetActorLocation();
	CornerPivot = Pivot;
	CornerUp0 = SurfaceUp;
	CornerUp1 = NewUp.GetSafeNormal();
	const FVector D0 = Start - Pivot, D1 = EndLocation - Pivot;
	CornerLen0 = D0.Size(); CornerLen1 = D1.Size();
	CornerDir0 = D0.GetSafeNormal(); CornerDir1 = D1.GetSafeNormal();

	// Outside corners (the new face points the way we walk) must arc over the lip, never through the block:
	// rotate about the edge in the same sense as the body pitches. Inside corners take the short way.
	const FVector OldForward = GetActorForwardVector();
	const bool bConvex = (CornerUp1 | OldForward) > 0.f;
	FVector Axis = CornerUp0 ^ CornerUp1;
	CornerArcAngle = 0.f;
	if (Axis.Normalize())
	{
		CornerArcAxis = Axis;
		float Angle = FMath::Atan2((CornerDir0 ^ CornerDir1) | Axis, CornerDir0 | CornerDir1);
		if (bConvex && Angle < 0.f) Angle += 2.f * PI;
		CornerArcAngle = Angle;
	}
	else
	{
		CornerArcAxis = FVector::ZeroVector;
	}

	CornerRot0 = GetActorQuat();
	FVector Forward = OldForward - CornerUp1 * (OldForward | CornerUp1);
	if (Forward.SizeSquared() < 0.05f)
	{
		Forward = bConvex ? -CornerUp0 : CornerUp0;   // over an edge: head down the face; into a wall: head up it
	}
	CornerRot1 = FRotationMatrix::MakeFromXZ(Forward.GetSafeNormal(), CornerUp1).ToQuat();

	const float Turn = FMath::Acos(FMath::Clamp(CornerUp0 | CornerUp1, -1.f, 1.f));
	CornerDuration = CornerTime * FMath::Max(Turn / HALF_PI, 0.3f);
	CornerAlpha = 0.f;
	bInCorner = true;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->StopMovementImmediately();
	Move->DisableMovement();
}

void AHexapodCharacter::UpdateCorner(float DeltaSeconds)
{
	CornerAlpha = FMath::Min(1.f, CornerAlpha + DeltaSeconds / FMath::Max(CornerDuration, 0.05f));
	const float S = FMath::SmoothStep(0.f, 1.f, CornerAlpha);

	const FVector Dir = CornerArcAxis.IsZero()
		? FMath::Lerp(CornerDir0, CornerDir1, S).GetSafeNormal()
		: FQuat(CornerArcAxis, CornerArcAngle * S).RotateVector(CornerDir0);
	const FVector Location = CornerPivot + Dir * FMath::Lerp(CornerLen0, CornerLen1, S);
	const FQuat Rotation = FQuat::Slerp(CornerRot0, CornerRot1, S);
	SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);

	// gravity (and with it the camera) turns gradually with the body
	const FVector Up = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(CornerUp0, CornerUp1), S).RotateVector(CornerUp0);
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->SetGravityDirection(-Up);

	if (CornerAlpha >= 1.f)
	{
		bInCorner = false;
		SurfaceUp = CornerUp1;
		Move->SetGravityDirection(-SurfaceUp);
		Move->SetMovementMode(MOVE_Walking);
		SurfaceCooldown = 0.3f;
	}
}