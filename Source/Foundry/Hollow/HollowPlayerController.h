#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HollowPlayerController.generated.h"

// Polls keys and mouse each frame and forwards intent to AHollowGameMode, which owns all game state.
UCLASS()
class AHollowPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AHollowPlayerController();
	virtual void PlayerTick(float DeltaTime) override;
	virtual void BeginPlay() override;
};
