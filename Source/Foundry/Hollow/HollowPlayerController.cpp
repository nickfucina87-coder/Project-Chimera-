#include "HollowPlayerController.h"

#include "Engine/World.h"
#include "HollowGameMode.h"
#include "HollowHero.h"

AHollowPlayerController::AHollowPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	DefaultMouseCursor = EMouseCursor::Default;
	bShouldPerformFullTickWhenPaused = true;   // the pause screen still takes keys
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
		// screen-relative: rotate the stick by the camera's yaw
		Hero->ManualInput = In.GetRotated(Hero->GetViewYaw());
	}

	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		HoldTime = 0.f;
		FHitResult Hit;
		if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
			GM->ClickWorld(Hit);
	}
	else if (IsInputKeyDown(EKeys::LeftMouseButton) && bFree)
	{
		// click and hold: keep walking toward the cursor until you let go
		HoldTime += DeltaTime;
		FHitResult Hit;
		if (HoldTime > 0.18f && GetHitResultUnderCursor(ECC_Visibility, false, Hit))
			if (AHollowHero* H = Cast<AHollowHero>(GetPawn())) H->MoveTo(Hit.ImpactPoint);
	}
	if (WasInputKeyJustReleased(EKeys::LeftMouseButton) && HoldTime > 0.18f)
		if (AHollowHero* H = Cast<AHollowHero>(GetPawn())) H->StopMove();
	if (WasInputKeyJustPressed(EKeys::Q)) if (AHollowHero* H = Cast<AHollowHero>(GetPawn())) H->RotateView(-60.f);
	if (WasInputKeyJustPressed(EKeys::R)) if (AHollowHero* H = Cast<AHollowHero>(GetPawn())) H->RotateView(60.f);
	if (WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::SpaceBar)) GM->Interact();
	if (WasInputKeyJustPressed(EKeys::M)) GM->ToggleMap();
	if (WasInputKeyJustPressed(EKeys::Tab)) GM->CycleTarget();
	if (WasInputKeyJustPressed(EKeys::Escape)) GM->Escape();
	if (WasInputKeyJustPressed(EKeys::C)) GM->ToggleSheet();
	if (WasInputKeyJustPressed(EKeys::I)) GM->TogglePause(EPauseTab::Inventory);
	if (WasInputKeyJustPressed(EKeys::J)) GM->TogglePause(EPauseTab::Journal);
	if (WasInputKeyJustPressed(EKeys::P)) GM->TogglePause(EPauseTab::System);
	if (WasInputKeyJustPressed(EKeys::L)) GM->ToggleLantern();
	static const FKey Nums[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
	if (GM->Talk.IsSet())
		for (int32 i = 0; i < 9; ++i) if (WasInputKeyJustPressed(Nums[i])) GM->TalkNumber(i + 1);
	if (WasInputKeyJustPressed(EKeys::MouseScrollUp) || WasInputKeyJustPressed(EKeys::Equals)) GM->Zoom(-220.f);
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown) || WasInputKeyJustPressed(EKeys::Hyphen)) GM->Zoom(220.f);
}
