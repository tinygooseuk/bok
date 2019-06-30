// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#include "BokPawnBase.h"

// Engine
#include "Animation/AnimBlueprint.h"
#include "Camera/CameraComponent.h"
#include "Components/BokCableComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/UnbokableComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/KillZVolume.h"
#include "Gameplay/EggBase.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Particles/Emitter.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundWave.h"
#include "TimerManager.h"

// Game
#include "BokPlayerControllerBase.h"
#include "Components/DeadlyComponent.h"
#include "Gameplay/DoorBase.h"
#include "Shared/Tween/GooseTweenComponent.h"
#include "System/BokGameModeBase.h"

ABokPawnBase::ABokPawnBase()
	: CameraRotator(-30.0f, 0.0f, 0.0f)
	, BokDebounceTimer(0.0f)
	, EggCamRatio(0.0f)
	, bDead(false)
	, bFlapping(false)
	, bFlapped(false)
	, bFrozen(false)
	, bForcedFocus(false)
{
	PrimaryActorTick.bCanEverTick = true;

	// Resources /////////////////////////
	SM_Bok = FIND_RESOURCE(SkeletalMesh, SM_Bok, TEXT("SkeletalMeshes/SM_Bok"));
	SM_Bok_AnimBlueprint = FIND_CLASS(SM_Bok_Skeleton_AnimBlueprint, TEXT("SkeletalMeshes/SM_Bok"));
	M_Leg = FIND_RESOURCE(Material, M_Leg, TEXT("Materials"));
	M_DeadlyRed = FIND_RESOURCE(Material, M_DeadlyRed, TEXT("Materials"));

	PHYS_NoRestitution = FIND_RESOURCE(PhysicalMaterial, PHYS_NoRestitution, TEXT("PhysicsMaterials"));
	//////////////////////////////////////
	
	BokSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("BokSphere"));
	BokSphereComponent->InitSphereRadius(20.0f);
	BokSphereComponent->SetGenerateOverlapEvents(true);
	BokSphereComponent->SetNotifyRigidBodyCollision(true);
	BokSphereComponent->OnComponentHit.AddDynamic(this, &ABokPawnBase::OnHit);
	BokSphereComponent->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	SetRootComponent(BokSphereComponent);
	
	BokSkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BokSkeletalMesh"));
	//BokSkeletalMeshComponent->bAbsoluteRotation = true;
	BokSkeletalMeshComponent->SetSkeletalMesh(SM_Bok);
	BokSkeletalMeshComponent->SetAnimInstanceClass(SM_Bok_AnimBlueprint);
	BokSkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BokSkeletalMeshComponent->SetRenderCustomDepth(true);
	BokSkeletalMeshComponent->SetCustomDepthStencilValue(0xDD);
	BokSkeletalMeshComponent->SetupAttachment(BokSphereComponent);

	// Left leg
	BokLeftLegComponent = CreateDefaultSubobject<UBokCableComponent>(TEXT("BokLeftLegCable"));
	BokLeftLegComponent->bAbsoluteRotation = true;
	BokLeftLegComponent->RelativeLocation = FVector(0.0f, -R_LegWidth, 0.0f);
	BokLeftLegComponent->EndLocation = FVector::ZeroVector;
	BokLeftLegComponent->bAttachStart = true;
	BokLeftLegComponent->bAttachEnd = true;
	BokLeftLegComponent->CableLength = 10.0f;
	BokLeftLegComponent->CableWidth = 5.0f;
	BokLeftLegComponent->SubstepTime = 0.015f;
	BokLeftLegComponent->NumSides = 8;
	BokLeftLegComponent->bEnableStiffness = true;
	BokLeftLegComponent->bEnableCollision = false;
	BokLeftLegComponent->SetRenderCustomDepth(true);
	BokLeftLegComponent->SetCustomDepthStencilValue(0xDD);
	BokLeftLegComponent->SetMaterial(0, M_Leg);
	BokLeftLegComponent->SetupAttachment(BokSkeletalMeshComponent);

	// Right leg
	BokRightLegComponent = CreateDefaultSubobject<UBokCableComponent>(TEXT("BokRightLegCable"));
	BokRightLegComponent->bAbsoluteRotation = true;
	BokRightLegComponent->RelativeLocation = FVector(0.0f, +R_LegWidth, 0.0f);
	BokRightLegComponent->EndLocation = FVector::ZeroVector;
	BokRightLegComponent->bAttachStart = true;
	BokRightLegComponent->bAttachEnd = true;
	BokRightLegComponent->CableLength = 10.0f;
	BokRightLegComponent->CableWidth = 5.0f;
	BokRightLegComponent->SubstepTime = 0.015f;
	BokRightLegComponent->NumSides = 8;
	BokRightLegComponent->bEnableStiffness = true;
	BokRightLegComponent->bEnableCollision = false;
	BokRightLegComponent->SetRenderCustomDepth(true);
	BokRightLegComponent->SetCustomDepthStencilValue(0x80);
	BokRightLegComponent->SetMaterial(0, M_Leg);
	BokRightLegComponent->SetupAttachment(BokSkeletalMeshComponent);

	BokSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	BokSpringArmComponent->bEnableCameraRotationLag = true;
	BokSpringArmComponent->CameraRotationLagSpeed = 4.0f;
	BokSpringArmComponent->bEnableCameraLag = true;
	BokSpringArmComponent->CameraLagSpeed = 4.0f;
	BokSpringArmComponent->TargetArmLength = R_CameraSpringArmLength;
	BokSpringArmComponent->bAbsoluteRotation = true;
	//BokSpringArmComponent->bDoCollisionTest = false;
	BokSpringArmComponent->ProbeSize = 5.0f;
	BokSpringArmComponent->SetupAttachment(BokSphereComponent);
	
	BokCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("MainCamera"));
	BokCameraComponent->SetupAttachment(BokSpringArmComponent, USpringArmComponent::SocketName);	

	BokSpawnParticlesComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BokSpawnParticles"));
	BokSpawnParticlesComponent->bAbsoluteRotation = true;
	BokSpawnParticlesComponent->SetupAttachment(BokSphereComponent);
}

void ABokPawnBase::BeginPlay()
{
	Super::BeginPlay();

	// Set up body instance once created
	FBodyInstance* BI = BokSphereComponent->GetBodyInstance();
	GOOSE_BAIL(BI);

	BI->SetUseCCD(true);
	BI->SetPhysMaterialOverride(PHYS_NoRestitution);
	
	// Create material instance
	MI_Bok = BokSkeletalMeshComponent->CreateDynamicMaterialInstance(0);

	// Rotate camera and body to face PlayerStart orientation
	FRotator InitialCameraRotator = GetActorRotation();
	InitialCameraRotator.Pitch = -30.0f;
	CameraRotator = InitialCameraRotator;

	BokSpringArmComponent->SetWorldRotation(InitialCameraRotator);
	BokForward = GetActorRotation().Vector();
	
#if WITH_EDITOR
	// Editor builds should start ASAP
	BokSphereComponent->SetSimulatePhysics(true);
#else
	// Play spawn particles
	BokSpawnParticlesComponent->SetTemplate(R_SpawnParticles);

	// Become visible after 2 sec
	SetBokVisibility(false);

	FTimerDelegate Delegate;
	Delegate.BindUObject(this, &ABokPawnBase::OnFullySpawned);

	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, Delegate, 2.0f, false);
#endif

	// Finally scale legs if needed
	//BokLeftLegComponent->CableLength *= GetActorScale().X;
	//BokLeftLegComponent->CableWidth *= GetActorScale().X;
	BokRightLegComponent->CableLength *= GetActorScale().X;
	BokRightLegComponent->CableWidth *= GetActorScale().X;
}

void ABokPawnBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Rotate camera if required 
	if (bDead)
	{
		CameraRotator.Yaw += DeltaTime * 100.0f;
	}

	BokSpringArmComponent->SetRelativeRotation(CameraRotator);
	BokSpringArmComponent->bDoCollisionTest = CameraRotator.Pitch >= 0.0f; // Only try to solve for floor (i.e. looking UP)

	// Reset leg rotation
	GetRootComponent()->SetWorldRotation(FRotator::ZeroRotator);

	// Freeze if needed
	if (bFrozen)
	{
		BokSphereComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
	}

	// Tick down the debounce timer
	if (BokDebounceTimer >= 0.0f)
	{
		BokDebounceTimer -= DeltaTime;
	}
	else if (bFlapping)
	{
		Bok();
	}

	// Update floor check & up vector
	UpdateFloorCheck();

	// Update forward vector
	FVector LinearVelocity = BokSphereComponent->GetPhysicsLinearVelocity();
	if (LinearVelocity.SizeSquared2D() > R_MinTurnSpeed * R_MinTurnSpeed)
	{
		BokForward = LinearVelocity.GetSafeNormal();
	}

	// Clamp movement speed
	float SpeedXY = LinearVelocity.Size2D();
	if (SpeedXY > R_MaxXYSpeed)
	{
		float OriginalZ = LinearVelocity.Z;
		LinearVelocity = LinearVelocity.GetSafeNormal() * R_MaxXYSpeed;
		LinearVelocity.Z = OriginalZ;

		BokSphereComponent->SetPhysicsLinearVelocity(LinearVelocity);
	}

	// Update rotation
	const FQuat CurrentQuat = BokSkeletalMeshComponent->GetComponentQuat();
	const FQuat NewQuat = FRotationMatrix::MakeFromXZ(BokForward, BokUp).ToQuat();
	
	BokSkeletalMeshComponent->SetRelativeRotation(FMath::QInterpTo(CurrentQuat, NewQuat, DeltaTime, 10.0f));

	// Compute new EggCam Quat
	if (EggCamRatio > 0.01f)
	{
		FVector FocusForward = ForcedFocusOverrideDirection;
		
		// No forced direction - aim towards next "target"
		if (FocusForward == FVector::ZeroVector)
		{
			AActor* POI = nullptr;
			float NearestSqr = TNumericLimits<float>::Max();

			for (TActorIterator<AEggBase> EggIt(GetWorld()); EggIt; ++EggIt)
			{
				EggIt->SetXray(true);

				float ThisDistSqr = FVector::DistSquared(EggIt->GetActorLocation(), GetActorLocation());
				if (ThisDistSqr < NearestSqr)
				{
					NearestSqr = ThisDistSqr;
					POI = *EggIt;
				}
			}

			if (!POI)
			{
				TActorIterator<ADoorBase> DoorIt(GetWorld());
				if (DoorIt)
				{
					POI = *DoorIt;
				}
			}

			if (POI)
			{
				FocusForward = POI->GetActorLocation() - GetActorLocation();

				if (FocusForward.Z > 0.0f)
				{
					FocusForward.Z = 0.0f;
					FocusForward.Normalize();
					FocusForward.Z = -0.25f; // Don't allow pitching UPof camera!
				}
			}
		}

		const FQuat TargetCamQuat = FRotationMatrix::MakeFromXZ(FocusForward, BokUp).ToQuat();
		const FQuat CurrentCamQuat = CameraRotator.Quaternion();

		const FQuat LerpedQuat = FQuat::Slerp(CurrentCamQuat, TargetCamQuat, EggCamRatio * 0.25f);
		CameraRotator = LerpedQuat.Rotator();
	}
	else
	{
		// Turn off eggs-ray mode
		for (TActorIterator<AEggBase> EggIt(GetWorld()); EggIt; ++EggIt)
		{
			EggIt->SetXray(false);
		}

		// Set camera to normal, user-controlled values
		FRotator FixedCameraRotator = CameraRotator;
		FixedCameraRotator.Pitch = FMath::Min(R_CameraMaxHeightAngle, FMath::Max(-R_CameraMaxHeightAngle, FixedCameraRotator.Pitch));
		FixedCameraRotator.Roll = 0.0f;
		
		const FQuat FixedCamQuat = FixedCameraRotator.Quaternion();
		const FQuat CurrentCamQuat = CameraRotator.Quaternion();

		const FQuat LerpedQuat = FQuat::Slerp(CurrentCamQuat, FixedCamQuat, 0.1f);
		CameraRotator = LerpedQuat.Rotator();
	}
}

void ABokPawnBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

#define AUTOBIND_AXIS(AxisName)				PlayerInputComponent->BindAxis(TEXT(#AxisName), this, &ABokPawnBase::##AxisName)
#define BIND_AXIS(AxisName, Func)			PlayerInputComponent->BindAxis(TEXT(#AxisName), this, &ABokPawnBase::##Func)
#define AUTOBIND_ACTION(ActionName)			PlayerInputComponent->BindAction(TEXT(#ActionName), EInputEvent::IE_Pressed, this, &ABokPawnBase::##ActionName)
#define AUTOBIND_ONOFF_ACTION(ActionName)	do { PlayerInputComponent->BindAction(TEXT(#ActionName), EInputEvent::IE_Pressed, this, &ABokPawnBase::##ActionName##On); PlayerInputComponent->BindAction(TEXT(#ActionName), EInputEvent::IE_Released, this, &ABokPawnBase::##ActionName##Off);  } while (false)
	{
		AUTOBIND_AXIS(MoveForward);
		AUTOBIND_AXIS(MoveRight);
		AUTOBIND_AXIS(CameraMoveUp);
		AUTOBIND_AXIS(CameraMoveRight);

		BIND_AXIS(EggCam, SetEggCam);

		AUTOBIND_ONOFF_ACTION(Bok);
	}
#undef AUTOBIND_AXIS
#undef AUTOBIND_ACTION
#undef BIND_AXIS
}

void ABokPawnBase::FellOutOfWorld(const class UDamageType& DamageType)
{
	Super::FellOutOfWorld(DamageType);

	Die();
}

void ABokPawnBase::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (Cast<AKillZVolume>(OtherActor))
	{
		Die();
	}
}


void ABokPawnBase::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	static constexpr float MAX_HIT = 180'000.0f;

	// Apply vibration
	float HitImpulse = BokSphereComponent->GetPhysicsLinearVelocity().Size();
	HitImpulse = FMath::Abs(FVector::DotProduct(BokSphereComponent->GetPhysicsLinearVelocity(), Hit.ImpactNormal) * HitImpulse);

	float VibrateIntensity = FMath::Min(HitImpulse / MAX_HIT, 1.0f);
	Vibrate(VibrateIntensity, 0.1f);

	// Die if touching something bad
	if (OtherActor && OtherActor->FindComponentByClass<UDeadlyComponent>())
	{
		Die();
		return;
	} 
}

bool ABokPawnBase::IsGrounded() const 
{
	return FloorTrace.bBlockingHit;
}

void ABokPawnBase::OnFullySpawned()
{
	SetFrozen(false);
	SetBokVisibility(true);
}

void ABokPawnBase::SetFrozen(bool bInFrozen)
{
	bFrozen = bInFrozen;
	BokSphereComponent->SetSimulatePhysics(!bInFrozen);
}

bool ABokPawnBase::IsFrozen() const
{
	return bFrozen;
}

void ABokPawnBase::SetForceFocus(bool bInForce, FVector ForcedDirection)
{
	bForcedFocus = bInForce;

	if (bForcedFocus)
	{
		ForcedFocusOverrideDirection = ForcedDirection;
	}
}

bool ABokPawnBase::CheckFlappedAndConsume_BP()
{
	bool bOldValue = bFlapped;
	bFlapped = false;

	return bOldValue;
}

void ABokPawnBase::MoveForward(float AxisValue)
{
	Move(AxisValue * FVector::ForwardVector);
}
void ABokPawnBase::MoveRight(float AxisValue)
{
	Move(AxisValue * FVector::RightVector);
}

void ABokPawnBase::CameraMoveUp(float AxisValue)
{
	CameraRotator.Pitch += AxisValue * R_CameraRotationSpeedPitch;
	CameraRotator.Pitch = FMath::Min(R_CameraMaxHeightAngle, FMath::Max(-R_CameraMaxHeightAngle, CameraRotator.Pitch));
}
void ABokPawnBase::CameraMoveRight(float AxisValue)
{
	CameraRotator.Yaw += AxisValue * R_CameraRotationSpeedYaw;
}

void ABokPawnBase::BokOn()
{
	bFlapping = true;
}

void ABokPawnBase::BokOff()
{
	bFlapping = false;
}

void ABokPawnBase::Bok()
{
	if (bDead || bFrozen || !BokSphereComponent->IsSimulatingPhysics() || BokDebounceTimer > 0.0f)
	{
		return;
	}
	
	// Add impulse to self
	float JumpImpulse = IsGrounded() ? R_BigJumpImpulse : R_JumpImpulse;
	BokSphereComponent->AddImpulse(BokUp * JumpImpulse, NAME_None, true);

	// Add impulse to nearby things
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->SweepMultiByChannel
	(
		Hits,
		GetActorLocation(),
		GetActorLocation() + FVector(0.0f, 0.0f, 1.0f),
		FQuat::Identity, 
		ECollisionChannel::ECC_WorldDynamic, 
		FCollisionShape::MakeSphere(R_BokAffectorRadius),
		Params
	);

	for (const FHitResult& Hit : Hits)
	{
		AEggBase* Egg = Cast<AEggBase>(Hit.Actor);
		if (Egg && Egg->IsFrozen())
		{
			// Frozen egg - unfreeze it!
			Egg->SetFrozen(false);
		}

		UPrimitiveComponent* HitComp = Hit.GetComponent();
		if (!HitComp || !HitComp->IsSimulatingPhysics())
		{
			// Component isn't physics enabled (even after thawing above)
			continue;
		}
		
		if (Hit.Actor.IsValid() && Hit.Actor->FindComponentByClass<UUnbokableComponent>())
		{
			// Actor has an "unbokable" component
			continue;
		}

		// Work out force dir and scale based on distance
		FVector ForceDir = (HitComp->GetComponentLocation() - GetActorLocation()).GetSafeNormal();
		float ForceScale = 1.0f - (FVector::Dist(HitComp->GetComponentLocation(), Hit.Location) / R_BokAffectorRadius);

		// If this is still a valid PUSH force, apply it
		if (ForceScale > 0.0f)
		{
			HitComp->AddImpulseAtLocation(ForceDir * R_BokForce * ForceScale, Hit.Location);
		}			
	}
		 
	// Play sound
	float GroundedMultiplier = IsGrounded() ? 2.0f : 1.0f;
	UGameplayStatics::PlaySoundAtLocation(this, R_JumpSound, GetActorLocation(), FMath::RandRange(0.8f, 1.2f) * GroundedMultiplier, FMath::RandRange(0.8f, 1.2f));

	// Add feathers
	SpawnParticlesAtPosition(R_JumpParticles);

	// Flag as flapped for anim
	bFlapped = true;

	// Debounce
	BokDebounceTimer = (IsGrounded() ? 2.0f : 1.0f) * R_JumpDebounceTime;
}

void ABokPawnBase::SetEggCam(float InEggCamRatio)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	GOOSE_BAIL(PC);

	if (bFrozen)
	{
		InEggCamRatio = 0.0f;
		GetWorld()->GetWorldSettings()->TimeDilation = 1.0f;
		PC->PlayerCameraManager->UnlockFOV();
	}
	else if (bForcedFocus)
	{
		InEggCamRatio = 1.0f;
	}
	else if (!bForcedFocus && EggCamRatio < 0.01f)
	{
		ForcedFocusOverrideDirection = FVector::ZeroVector;
	}

	EggCamRatio = FMath::Lerp(FMath::Min(1.0f, InEggCamRatio), EggCamRatio, 0.9f);

	ABokGameModeBase* GameMode = Cast<ABokGameModeBase>(GetWorld()->GetAuthGameMode());
	GOOSE_BAIL(GameMode);

	// Letterboxing
	GameMode->SetLetterBoxAmount(EggCamRatio * 0.15f);

	// Time dilation
	GetWorld()->GetWorldSettings()->TimeDilation = FMath::Lerp(1.0f, 0.5f, EggCamRatio);

	if (!bFrozen)
	{
		// Set FOV
		PC->PlayerCameraManager->SetFOV(FMath::Lerp(90.0f, 55.0f, EggCamRatio));
	}
}

void ABokPawnBase::SetBokVisibility(bool bInVisible)
{
	BokSkeletalMeshComponent->SetVisibility(bInVisible);
	BokLeftLegComponent->SetVisibility(bInVisible);
	BokRightLegComponent->SetVisibility(bInVisible);
}

void ABokPawnBase::Die()
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	SetFrozen(true);

	// Play inflation sound
	UGameplayStatics::PlaySoundAtLocation(this, R_InflateSound, GetActorLocation(), FMath::RandRange(0.8f, 1.2f), FMath::RandRange(0.8f, 1.2f));

	TWeakObjectPtr<ABokPawnBase> This(this);
	RunGooseTween
	(
		GT_Ease(QuadraticIn, GT_Lambda(0.3f, 1.0f, 50.0f, [This](float Value)
		{
			FName NAME_Inflation(TEXT("Inflation"));
			This->MI_Bok->SetScalarParameterValue(NAME_Inflation, Value);
		}))
	);
	
	auto Callback = [This] 
	{
		if (!This.IsValid())
		{
			return;
		}

		// Notify PC
		ABokPlayerControllerBase* PC = This->GetController<ABokPlayerControllerBase>();
		GOOSE_BAIL(PC);

		PC->OnPawnDied();

		// Play death sound
		UGameplayStatics::PlaySoundAtLocation(This.Get(), This->R_DeathSound, This->GetActorLocation(), FMath::RandRange(0.8f, 1.2f), FMath::RandRange(0.8f, 1.2f));

		// Add a fancy particle effect
		This->SpawnParticlesAtPosition(This->R_DeathParticles);
		
		// Run vibration effect
		This->Vibrate(1.0f, 1.0f);

		// Stop moving and be invisible
		This->BokSphereComponent->SetSimulatePhysics(false);
		This->SetBokVisibility(false);
	};

	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, Callback, 0.3f, false);
}

void ABokPawnBase::Move(const FVector& InputVector)
{
	if (bDead || !BokSphereComponent->IsSimulatingPhysics())
	{
		return;
	}

	float Multiplier = IsGrounded() ? 1.0f : R_AirControl;
	
	FRotator ModifiedCameraRotator = CameraRotator;
	ModifiedCameraRotator.Pitch = 0.0f;
	
	FVector RotatedVector = ModifiedCameraRotator.RotateVector(InputVector * Multiplier * R_RunImpulse);

	BokSphereComponent->AddForce(RotatedVector, NAME_None, true);
}

void ABokPawnBase::UpdateFloorCheck() 
{
	// Update floor check
	UWorld* World = GetWorld();
	GOOSE_BAIL(World);

	FCollisionQueryParams QueryParams = FCollisionQueryParams(TEXT("FloorTrace"), true, this);

	World->LineTraceSingleByChannel
	(
		FloorTrace,
		GetActorLocation(), GetActorLocation() + FVector(0.0f, 0.0f, -R_FloorTraceDepth),
		ECollisionChannel::ECC_WorldDynamic,
		QueryParams
	);

	FQuat YawQuat = FRotationMatrix::MakeFromXZ(BokForward, FVector::UpVector).ToQuat();
	FVector LegPositions[] = { YawQuat.RotateVector(FVector(0.0f, -10.0f, 0.0f)), YawQuat.RotateVector(FVector(0.0f, +10.0f, 0.0f)) };
	UBokCableComponent* LegComponents[] = { BokLeftLegComponent, BokRightLegComponent };

	if (FloorTrace.bBlockingHit)
	{
		// Get surface directions
		FVector SurfaceUp = FloorTrace.ImpactNormal;
		FVector SurfaceForward = FVector::CrossProduct(FVector::RightVector, SurfaceUp);

		// Get rotation of surface
		FQuat SurfaceQuat = FRotationMatrix::MakeFromXZ(-SurfaceForward, SurfaceUp).ToQuat();
			
		bool bPlayStepSound = false;
		for (int32 LegIdx = 0; LegIdx < ARRAY_COUNT(LegPositions); ++LegIdx)
		{
			// Get location of player in the surface plane's space
			FVector PlanarLocation = SurfaceQuat.RotateVector(FloorTrace.Location + LegPositions[LegIdx]);

			float LegOffset = 5.0f * (float(LegIdx * 2) - 1.0f);

			// Snap to grid
			static constexpr float GridSize = 30.0f;
			PlanarLocation.X = FMath::RoundToFloat(PlanarLocation.X / GridSize) * GridSize;
			PlanarLocation.Y = FMath::RoundToFloat(PlanarLocation.Y / GridSize) * GridSize;

			// Get position back in world coords
			FVector SnappedLocation = SurfaceQuat.UnrotateVector(PlanarLocation);
			FVector LocalSnappedLocation = LegComponents[LegIdx]->GetComponentTransform().InverseTransformPosition(SnappedLocation);

			float Dist = FVector::DistSquared2D(LegComponents[LegIdx]->EndLocation, LocalSnappedLocation);
			bPlayStepSound |= Dist > (8.0f * 8.0f);

			LegComponents[LegIdx]->EndLocation = LocalSnappedLocation;
			LegComponents[LegIdx]->bAttachEnd = true;
		}

		if (bPlayStepSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, R_FootstepSound, GetActorLocation(), 1.0f, FMath::RandRange(0.5f, 1.8f));
		}

		BokUp = SurfaceUp;
	}
	else
	{
		BokUp = FVector::UpVector;

		for (int32 LegIdx = 0; LegIdx < ARRAY_COUNT(LegPositions); ++LegIdx)
		{			
			FQuat BodyQuat = BokSkeletalMeshComponent->GetComponentQuat();

			FVector FloorLoc = GetActorLocation() + BodyQuat.RotateVector(FVector(0.0f, 0.0f, -R_FloorTraceDepth) + LegPositions[LegIdx]);
			FVector LocalFloorLoc = LegComponents[LegIdx]->GetComponentTransform().InverseTransformPosition(FloorLoc);

			LegComponents[LegIdx]->bAttachEnd = false;
		}
	} 
}

void ABokPawnBase::SpawnParticlesAtPosition(UParticleSystem* System)
{
	// Add feathers
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AEmitter* ParticleEmitter = GetWorld()->SpawnActor<AEmitter>(AEmitter::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
	GOOSE_BAIL(ParticleEmitter);

	ParticleEmitter->SetTemplate(System);
}

void ABokPawnBase::Vibrate(float Intensity, float Duration)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	
	if (PC)
	{
		PC->PlayDynamicForceFeedback(Intensity, Duration, true, true, true, true);
	}
}