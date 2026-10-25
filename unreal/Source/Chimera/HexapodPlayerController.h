#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HexapodPlayerController.generated.h"

/** Keeps the camera's control rotation relative to the pawn's current gravity, so mouse look stays sane on walls/ceilings. */
UCLASS()
class CHIMERA_API AHexapodPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void UpdateRotation(float DeltaTime) override;

private:
	FVector LastFrameGravity = FVector::ZeroVector;
};
