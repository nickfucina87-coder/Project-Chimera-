#include "HexapodGameMode.h"
#include "HexapodCharacter.h"
#include "HexapodPlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"

AHexapodGameMode::AHexapodGameMode()
{
	DefaultPawnClass = AHexapodCharacter::StaticClass();
	PlayerControllerClass = AHexapodPlayerController::StaticClass();
}

AActor* AHexapodGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerStartTag == FName("Hexapod"))
		{
			return *It;
		}
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}
