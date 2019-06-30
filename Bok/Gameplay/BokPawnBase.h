// (c)2019 TinyGoose Ltd., All Rights Reserved.

#pragma once

#include "Bok.h"
#include "GameFramework/Pawn.h"
#include "SharedPointer.h"
#include "BokPawnBase.generated.h"

UCLASS()
class BOK_API ABokPawnBase : public APawn
{
	GENERATED_BODY()

public:
	ABokPawnBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void FellOutOfWorld(const class UDamageType& DamageType) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// True iff grounded
	UFUNCTION(BlueprintCallable)
	bool IsGrounded() const;

	UFUNCTION()
	void OnFullySpawned();

	UFUNCTION(BlueprintCallable)
	void SetFrozen(bool bInFrozen);
	
	UFUNCTION(BlueprintCallable)
	bool IsFrozen() const;
	
	UFUNCTION(BlueprintCallable)
	void SetForceFocus(bool bInForce, FVector ForcedDirection = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, DisplayName = "Check Flapped and Consume")
	bool CheckFlappedAndConsume_BP();

	// Inputs /////////////////////////
	UFUNCTION()
	void MoveForward(float AxisValue);
	UFUNCTION()
	void MoveRight(float AxisValue);
	
	UFUNCTION()
	void CameraMoveUp(float AxisValue);
	UFUNCTION()
	void CameraMoveRight(float AxisValue);

	UFUNCTION()
	void BokOn();

	UFUNCTION()
	void BokOff();

	UFUNCTION()
	void Bok();

	// Egg-cam shortcut
	UFUNCTION()
	void SetEggCam(float EggCamRatio);

protected:
	UFUNCTION()
	void SetBokVisibility(bool bInVisible);

	UFUNCTION()
	void Die();

	// Tunables /////////////////////////
	UPROPERTY(EditDefaultsOnly, DisplayName = "Run Impulse", Category="BokPawn")
	float R_RunImpulse = 1500.0f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Max Ground Speed", Category = "BokPawn")
	float R_MaxXYSpeed = 1000.0f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Air Control", Category="BokPawn")
	float R_AirControl = 0.2f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Jump Impulse", Category="BokPawn")
	float R_JumpImpulse = 250.0f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Big Jump Impulse", Category="BokPawn")
	float R_BigJumpImpulse = 400.0f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Bok Sphere Radius", Category = "BokPawn")
	float R_BokAffectorRadius = 200.0f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Bok Force", Category = "BokPawn")
	float R_BokForce = 50000.0f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Jump Debounce Time", Category = "BokPawn")
	float R_JumpDebounceTime = 0.3f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Camera Max Height Angle", Category="BokPawn")
	float R_CameraMaxHeightAngle = 50.0f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Camera SpringArm Length", Category="BokPawn")
	float R_CameraSpringArmLength = 350.0f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Camera Rotation Speed (Yaw)", Category = "BokPawn")
	float R_CameraRotationSpeedYaw = 2.0f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Camera Rotation Speed (Pitch)", Category = "BokPawn")
	float R_CameraRotationSpeedPitch = 2.0f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Floor Trace Depth", Category="BokPawn")
	float R_FloorTraceDepth = 40.0f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Leg Width", Category="BokPawn")
	float R_LegWidth = 10.0f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Min Turn Speed", Category="BokPawn")
	float R_MinTurnSpeed = 20.0f;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Death Particle Effect", Category = "BokPawn")
	class UParticleSystem* R_DeathParticles;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Spawn Particle Effect", Category = "BokPawn")
	class UParticleSystem* R_SpawnParticles;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Jump Particle Effect", Category = "BokPawn")
	class UParticleSystem* R_JumpParticles;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Jump Sound Effect", Category = "BokPawn")
	class USoundWave* R_JumpSound;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Inflate Sound Effect", Category = "BokPawn")
	class USoundWave* R_InflateSound;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Death Sound Effect", Category = "BokPawn")
	class USoundWave* R_DeathSound;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Footstep Sound Effect", Category = "BokPawn")
	class USoundWave* R_FootstepSound;

private:
	// Private functions /////////////////////////
	void Move(const FVector& InputVector);

	void UpdateFloorCheck();

	void SpawnParticlesAtPosition(class UParticleSystem* System);

	void Vibrate(float Intensity, float Duration);

	// State /////////////////////////
	FRotator CameraRotator;
	FHitResult FloorTrace;
	float BokDebounceTimer;

	FVector BokUp;
	FVector BokForward;
	
	FQuat EggCamQuat;
	float EggCamRatio;

	FVector ForcedFocusOverrideDirection;

	int32 bDead : 1;
	int32 bFlapping : 1;
	int32 bFlapped : 1;
	int32 bFrozen : 1;
	int32 bForcedFocus : 1;

	UPROPERTY()
	UMaterialInstanceDynamic* MI_Bok;

	// Resources /////////////////////////
	UPROPERTY()
	class USkeletalMesh* SM_Bok;

	UPROPERTY()
	class UClass* SM_Bok_AnimBlueprint;

	UPROPERTY()
	class UMaterial* M_Leg;

	UPROPERTY()
	class UMaterial* M_DeadlyRed;

	UPROPERTY()
	class UPhysicalMaterial* PHYS_NoRestitution;

protected:
	// Components /////////////////////////
	UPROPERTY(EditDefaultsOnly)
	class USphereComponent* BokSphereComponent;

	UPROPERTY(EditDefaultsOnly)
	class USkeletalMeshComponent* BokSkeletalMeshComponent;

	UPROPERTY(EditDefaultsOnly)
	class UBokCableComponent* BokLeftLegComponent;
	
	UPROPERTY(EditDefaultsOnly)
	class UBokCableComponent* BokRightLegComponent;

	UPROPERTY(EditDefaultsOnly)
	class USpringArmComponent* BokSpringArmComponent;

	UPROPERTY(EditDefaultsOnly)
	class UCameraComponent* BokCameraComponent;

	UPROPERTY(EditDefaultsOnly)
	class UParticleSystemComponent* BokSpawnParticlesComponent;
};
