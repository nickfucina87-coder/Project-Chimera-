#include "HollowHero.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "HollowGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "HollowArt.h"
#include "HollowTypes.h"

AHollowHero::AHollowHero()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(40.f, 85.f);

	Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Visual"));
	Visual->SetupAttachment(GetCapsuleComponent());
	Visual->SetRelativeLocation(FVector(0, 0, -85.f));

	Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Boom"));
	Boom->SetupAttachment(GetCapsuleComponent());
	Boom->SetUsingAbsoluteRotation(true);
	Boom->SetWorldRotation(FRotator(-52.f, 0.f, 0.f));
	Boom->TargetArmLength = 2300.f;
	Boom->bDoCollisionTest = false;
	Boom->bEnableCameraLag = true;
	Boom->CameraLagSpeed = 8.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Boom);
	Camera->SetFieldOfView(50.f);

	Lantern = CreateDefaultSubobject<UPointLightComponent>(TEXT("Lantern"));
	Lantern->SetupAttachment(GetCapsuleComponent());
	Lantern->SetRelativeLocation(FVector(30.f, -40.f, 90.f));
	Lantern->SetIntensityUnits(ELightUnits::Candelas);
	Lantern->SetIntensity(35.f);
	Lantern->SetAttenuationRadius(1100.f);
	Lantern->SetCastShadows(true);
	Lantern->SetSourceRadius(8.f);
	Lantern->SetVolumetricScatteringIntensity(2.f);

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0, 720.f, 0);
	Move->MaxWalkSpeed = 520.f;
	Move->BrakingDecelerationWalking = 3000.f;
}

void AHollowHero::SetLook(const FPC& PC)
{
	TArray<USceneComponent*> Old;
	Visual->GetChildrenComponents(true, Old);
	for (USceneComponent* C : Old) C->DestroyComponent();

	const FString Weapon = PC.Kit == TEXT("wizard") ? TEXT("staff") : PC.Kit == TEXT("rogue") ? TEXT("dagger") : PC.Kit == TEXT("cleric") ? TEXT("mace") : TEXT("sword");
	HollowArt::Chibi(this, Visual, FVector::ZeroVector, PC.Body, PC.Skin, PC.Hat, PC.HatStyle, 1.f, Weapon);
	const bool bArcane = PC.Kit == TEXT("wizard") || PC.Kit == TEXT("cleric");
	Lantern->SetLightColor(bArcane ? FLinearColor(0.45f, 0.65f, 1.f) : FLinearColor(1.f, 0.7f, 0.4f));
}

void AHollowHero::Tick(float Dt)
{
	Super::Tick(Dt);

	FVector Dir(ManualInput.X, ManualInput.Y, 0.f);
	if (!Dir.IsNearlyZero())
	{
		bHasMoveTarget = false;
	}
	else if (bHasMoveTarget)
	{
		const FVector Delta = (MoveTarget - GetActorLocation()) * FVector(1, 1, 0);
		if (Delta.Size() < 40.f) bHasMoveTarget = false;
		else Dir = Delta.GetSafeNormal();
	}
	if (!Dir.IsNearlyZero()) AddMovementInput(Dir.GetClampedToMaxSize(1.f));

	if (ClampRadius > 0.f)
	{
		const FVector Off = (GetActorLocation() - ClampCenter) * FVector(1, 1, 0);
		if (Off.Size() > ClampRadius)
		{
			const FVector P = ClampCenter + Off.GetSafeNormal() * ClampRadius;
			SetActorLocation(FVector(P.X, P.Y, GetActorLocation().Z));
		}
	}

	const float Arm = FMath::FInterpTo(Boom->TargetArmLength, TargetArm, Dt, 6.f);
	Boom->TargetArmLength = Arm;
	ViewYaw = FMath::FInterpTo(ViewYaw, TargetYaw, Dt, 7.f);
	Boom->SetWorldRotation(FRotator(FMath::GetMappedRangeValueClamped(FVector2D(700.f, 3600.f), FVector2D(-40.f, -66.f), Arm), ViewYaw, 0.f));

	const float Speed = GetVelocity().Size2D();
	if (Speed > 20.f)
	{
		StepClock += Dt;
		if (StepClock > 0.36f)
		{
			StepClock = 0.f;
			if (AHollowGameMode* GM = GetWorld()->GetAuthGameMode<AHollowGameMode>()) GM->OnFootstep(GetActorLocation() - FVector(0, 0, 85.f));
		}
		BobTime += Dt * 14.f;
		Visual->SetRelativeLocation(FVector(0, 0, -85.f + FMath::Abs(FMath::Sin(BobTime)) * 10.f));
		Visual->SetRelativeRotation(FRotator(0, 0, FMath::Sin(BobTime) * 5.f));
	}
	else
	{
		BobTime = 0.f;
		Visual->SetRelativeLocation(FVector(0, 0, -85.f));
		Visual->SetRelativeRotation(FRotator::ZeroRotator);
	}
}
