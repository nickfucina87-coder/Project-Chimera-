#include "HexapodPlayerController.h"
#include "HexapodGravity.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void AHexapodPlayerController::UpdateRotation(float DeltaTime)
{
	FVector GravityDirection = FVector::DownVector;
	if (const ACharacter* PlayerCharacter = Cast<ACharacter>(GetPawn()))
	{
		if (const UCharacterMovementComponent* MoveComp = PlayerCharacter->GetCharacterMovement())
		{
			GravityDirection = MoveComp->GetGravityDirection();
		}
	}

	FRotator ViewRotation = GetControlRotation();

	// Carry the view along when gravity changes so the camera turns with the surface instead of snapping.
	if (!LastFrameGravity.Equals(FVector::ZeroVector))
	{
		const FQuat DeltaGravityRotation = FQuat::FindBetweenNormals(LastFrameGravity, GravityDirection);
		ViewRotation = (DeltaGravityRotation * FQuat(ViewRotation)).Rotator();
	}
	LastFrameGravity = GravityDirection;

	ViewRotation = HexGravity::ToGravityRelative(ViewRotation, GravityDirection);

	FRotator DeltaRot(RotationInput);
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ProcessViewRotation(DeltaTime, ViewRotation, DeltaRot);
		ViewRotation.Roll = 0.f;
		SetControlRotation(HexGravity::ToGravityWorld(ViewRotation, GravityDirection));
	}

	if (APawn* const P = GetPawnOrSpectator())
	{
		P->FaceRotation(ViewRotation, DeltaTime);
	}
}
