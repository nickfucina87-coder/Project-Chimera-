#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "HexapodCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UAnimSequence;
class UBlendSpace;

/** What the hexapod is doing right now, as far as animation is concerned. */
UENUM(BlueprintType)
enum class EHexAction : uint8
{
	None,
	Grabbing,   // Grab_Reach -> Grab_Hold loop until released
	Upright,    // Stand_Upright_Start -> Loop until released
	OneShot     // any non-looping clip (attack, land, release, end...)
};

/**
 * Playable hexapod. Walks on any static surface: walking into a wall climbs it, walking off an edge wraps around
 * onto the face below, ceilings work the same way. Jump detaches back to world gravity.
 * Uses a spherical capsule so re-orienting onto a new surface can never push it into geometry.
 */
UCLASS()
class CHIMERA_API AHexapodCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHexapodCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnJumped_Implementation() override;

	/** World-space "up" of the surface the hexapod is attached to. */
	UFUNCTION(BlueprintPure, Category = "Hexapod|Climb")
	FVector GetSurfaceUp() const { return SurfaceUp; }

	// ---- tuning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Climb")
	bool bCanClimb = true;

	/** Surfaces steeper than this (dot with current up) count as a new surface to climb onto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Climb")
	float NewSurfaceDot = 0.6f;

	/** How far ahead of the body (beyond the capsule) to probe for walls and edges. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Climb")
	float ProbeAhead = 35.f;

	/** Seconds to crawl around a 90 degree corner onto a new surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Climb")
	float CornerTime = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Movement")
	float RunSpeed = 140.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Movement")
	float SprintSpeed = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Movement")
	float WalkSpeed = 45.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Movement")
	float StealthSpeed = 30.f;

	// ---- animation assets (defaults are loaded from /Game/Characters/HexapodV2)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimIdle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimWalk;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimRun;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimSprint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimGallop;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimStrafeL;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimStrafeR;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimBackup;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimTurnL;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimTurnR;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimCeilingIdle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimStealth;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimJumpStart;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimFall;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimLand;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimGrabReach;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimGrabHold;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimGrabRelease;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimAttack;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimUprightStart;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimUprightLoop;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimUprightEnd;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimGrabWalkFwd;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimGrabWalkBack;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimGrabStrafeL;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimGrabStrafeR;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimUprightWalkFwd;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimUprightWalkBack;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimPounce;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Anim") TObjectPtr<UAnimSequence> AnimBuck;

	/** Pounce launch (cm/s) applied on the push frame of Upright_Pounce. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Movement") float PounceForward = 650.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Movement") float PounceUp = 380.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Movement") float HoldingSpeed = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Movement") float UprightSpeed = 22.f;

	/** Camera distance limits for mouse-wheel zoom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Camera") float ZoomMin = 160.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Camera") float ZoomMax = 1100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hexapod|Camera") float ZoomStep = 60.f;

	// ---- input
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputMappingContext> DefaultContext;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputMappingContext> MouseLookContext;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputMappingContext> HexContext;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputAction> MouseLookAction;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputAction> SprintAction;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputAction> StealthAction;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputAction> WalkAction;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputAction> GrabAction;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputAction> AttackAction;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputAction> RearAction;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputAction> ZoomAction;
	UPROPERTY(EditAnywhere, Category = "Hexapod|Input") TObjectPtr<UInputAction> BuckAction;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hexapod") TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hexapod") TObjectPtr<UCameraComponent> FollowCamera;

private:
	// input handlers
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void JumpPressed();
	void SprintOn() { bSprint = true; }
	void SprintOff() { bSprint = false; }
	void StealthToggle() { bStealth = !bStealth; }
	void WalkOn() { bWalk = true; }
	void WalkOff() { bWalk = false; }
	void GrabPressed();
	void GrabReleased();
	void AttackPressed();
	void RearPressed();
	void RearReleased();
	void Zoom(const FInputActionValue& Value);
	void BuckPressed();
	bool IsUprightIdleOrWalking() const;
	bool IsHoldingIdleOrWalking() const;

	// climbing
	void UpdateSurface(float DeltaSeconds);
	bool Probe(const FVector& From, const FVector& To, FHitResult& OutHit) const;
	bool IsClimbable(const FHitResult& Hit) const;
	void AttachToSurface(const FVector& NewUp, const FVector* NewLocation = nullptr);
	/** Crawl around a corner: arc the body about Pivot from its current pose to EndLocation with up = NewUp. */
	void BeginCorner(const FVector& Pivot, const FVector& NewUp, const FVector& EndLocation);
	void UpdateCorner(float DeltaSeconds);
	UAnimSequence* PickGait(float Speed, float Direction, float& OutRate) const;

	// animation
	void UpdateAnimation(float DeltaSeconds);
	void PlayClip(UAnimationAsset* Asset, bool bLoop, float Rate = 1.f);
	bool ClipFinished() const;

	FVector SurfaceUp = FVector::UpVector;
	float AirTime = 0.f;
	float SurfaceCooldown = 0.f;

	// visual smoothing of surface changes: mesh carries the old orientation and blends it away
	FQuat VisualOffset = FQuat::Identity;
	FQuat MeshBaseRotation = FQuat::Identity;

	bool bSprint = false, bStealth = false, bWalk = false;
	EHexAction Action = EHexAction::None;
	bool bActionHeld = false;
	TObjectPtr<UAnimationAsset> CurrentClip = nullptr;
	bool bCurrentLoops = false;
	bool bPlayingLocomotion = false;
	FVector LastForward = FVector::ZeroVector;

	// corner crawl state
	bool bInCorner = false;
	float CornerAlpha = 0.f, CornerDuration = 0.5f;
	FVector CornerPivot, CornerDir0, CornerDir1, CornerUp0, CornerUp1;
	float CornerLen0 = 0.f, CornerLen1 = 0.f;
	FQuat CornerRot0, CornerRot1;
	FVector CornerArcAxis = FVector::ZeroVector;
	float CornerArcAngle = 0.f;
	bool bJumpStartPlaying = false;
	float PendingJump = -1.f;
	float PendingPounce = -1.f;
	FVector2D MoveInput = FVector2D::ZeroVector;
};
