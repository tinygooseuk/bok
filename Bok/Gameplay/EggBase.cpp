// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#include "EggBase.h"

// Engine
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

// Game
#include "System/BokGameModeBase.h"
#include "System/BokGameStateBase.h"

AEggBase::AEggBase()
	: bIsFrozen(false)
{
	PrimaryActorTick.bCanEverTick = false;

	UStaticMesh* SM_Egg = FIND_RESOURCE(StaticMesh, SM_Egg, TEXT("StaticMeshes"));

	// Main mesh component
	EggMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EggMesh"));
	EggMeshComponent->SetStaticMesh(SM_Egg);
	EggMeshComponent->SetSimulatePhysics(true);
	EggMeshComponent->BoundsScale = 100.0f; // To render see-thru from a distance and when occluded
	EggMeshComponent->bRenderCustomDepth = true;
	EggMeshComponent->CustomDepthStencilValue = 0x40;

	SetRootComponent(EggMeshComponent);
}

void AEggBase::BeginPlay()
{
	Super::BeginPlay();

	if (bIsFrozen)
	{
		EggMeshComponent->SetSimulatePhysics(false);
	}
}

void AEggBase::FellOutOfWorld(const class UDamageType& DamageType)
{
	Super::FellOutOfWorld(DamageType);

	// Tell game mode
	ABokGameModeBase* GameMode = Cast<ABokGameModeBase>(GetWorld()->GetAuthGameMode());
	GOOSE_BAIL(GameMode);

	GameMode->OnEggFellOutOfWorld();

	// Vibrate PC#0	
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		PC->PlayDynamicForceFeedback(1.0f, 0.25f, true, true, true, true);
	}

	// Add visual flair
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GetWorld()->SpawnActor<AActor>(R_VisualFlairBlueprintClass, GetActorLocation(), FRotator::ZeroRotator, SpawnParams);

	// Recount eggs
	ABokGameStateBase* BokState = GetWorld()->GetAuthGameMode()->GetGameState<ABokGameStateBase>();
	GOOSE_BAIL(BokState);
	
	float EggPercent = 1.0f - (float(BokState->CurrentEggCount) / float(BokState->InitialEggCount));
	bool bLastEgg = (BokState->CurrentEggCount == 0);

	if (bLastEgg)
	{
		UGameplayStatics::PlaySoundAtLocation(this, R_LastEggCollectedSound, GetActorLocation(), 1.f, FMath::RandRange(0.8f, 1.2f));
	}
	else
	{
		float Frequency = FMath::Lerp(0.7f, 1.25f, EggPercent);
		UGameplayStatics::PlaySoundAtLocation(this, R_CollectedSound, GetActorLocation(), 1.f, Frequency);
	}
}

void AEggBase::SetXray(bool bIsXray)
{
	EggMeshComponent->bRenderCustomDepth = bIsXray;
}

bool AEggBase::IsFrozen() const
{
	return bIsFrozen;
}

void AEggBase::SetFrozen(bool bInFrozen)
{
	bIsFrozen = bInFrozen;

	EggMeshComponent->SetSimulatePhysics(!bIsFrozen);
}