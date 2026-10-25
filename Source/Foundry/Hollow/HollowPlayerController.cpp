#include "HollowPlayerController.h"

#include "Engine/World.h"
#include "HollowGameMode.h"
#include "HollowHero.h"

AHollowPlayerController::AHollowPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AHollowPlayerController::BeginPlay()
{
	Super::BeginPlay();
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
}

void AHollowPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	AHollowGameMode* GM = GetWorld()->GetAuthGameMode<AHollowGameMode>();
	if (!GM) return;

	const bool bFree = GM->CanWalk();
	if (AHollowHero* Hero = Cast<AHollowHero>(GetPawn()))
	{
		FVector2D In = FVector2D::ZeroVector;
		if (bFree)
		{
			// camera looks along +X, so screen-up is +X and screen-right is +Y
			if (IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up)) In.X += 1;
			if (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down)) In.X -= 1;
			if (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right)) In.Y += 1;
			if (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left)) In.Y -= 1;
		}
		else
		{
			Hero->StopMove();
		}
		Hero->ManualInput = In;
	}

	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		FHitResult Hit;
		if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
			GM->ClickWorld(Hit);
	}
	if (WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::SpaceBar)) GM->Interact();
	if (WasInputKeyJustPressed(EKeys::M)) GM->ToggleMap();
	if (WasInputKeyJustPressed(EKeys::Tab)) GM->CycleTarget();
	if (WasInputKeyJustPressed(EKeys::Escape)) GM->Escape();
	if (WasInputKeyJustPressed(EKeys::C)) GM->ToggleSheet();
	if (WasInputKeyJustPressed(EKeys::MouseScrollUp) || WasInputKeyJustPressed(EKeys::Equals)) GM->Zoom(-220.f);
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown) || WasInputKeyJustPressed(EKeys::Hyphen)) GM->Zoom(220.f);
}
