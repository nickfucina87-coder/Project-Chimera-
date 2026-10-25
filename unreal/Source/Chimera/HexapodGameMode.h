#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HexapodGameMode.generated.h"

/** Spawns the hexapod at a PlayerStart tagged "Hexapod" if one exists. */
UCLASS()
class CHIMERA_API AHexapodGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHexapodGameMode();
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};
