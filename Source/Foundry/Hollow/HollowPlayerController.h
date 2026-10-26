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
private:
	float HoldTime = 0.f;   // how long the left button has been held (click-and-hold follows the cursor)
};
