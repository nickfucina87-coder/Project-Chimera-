#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HollowHero.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UPointLightComponent;
struct FPC;

// Top-down hero: a chibi built from primitives, a fixed-angle boom camera, and click-to-move steering.
UCLASS()
class AHollowHero : public ACharacter
{
	GENERATED_BODY()

public:
	AHollowHero();
	virtual void Tick(float DeltaSeconds) override;

	void SetLook(const FPC& PC);
	void MoveTo(const FVector& Target) { MoveTarget = Target; bHasMoveTarget = true; }
	void StopMove() { bHasMoveTarget = false; }
	void ZoomBy(float Delta) { TargetArm = FMath::Clamp(TargetArm + Delta, 700.f, 3600.f); }
	// The camera turns in 60 degree steps, one hex side at a time; WASD stays relative to the screen.
	void RotateView(float Deg) { TargetYaw += Deg; }
	void SetViewYaw(float Deg) { TargetYaw = ViewYaw = Deg; }
	float GetViewYaw() const { return TargetYaw; }

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Visual;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Boom;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Lantern;

	FVector2D ManualInput = FVector2D::ZeroVector;
	FVector ClampCenter = FVector::ZeroVector;
	float ClampRadius = 0.f;

private:
	FVector MoveTarget = FVector::ZeroVector;
	bool bHasMoveTarget = false;
	float BobTime = 0.f;
	float StepClock = 0.f;
	float TargetArm = 2100.f;
	float ViewYaw = 0.f, TargetYaw = 0.f;
};
