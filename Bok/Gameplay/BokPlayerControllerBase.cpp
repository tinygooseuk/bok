// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#include "BokPlayerControllerBase.h"

// Engine
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"

// Game
#include "BokPawnBase.h"
#include "Managers/LevelManager.h"
#include "System/BokGameStateBase.h"
#include "System/BokGameInstanceBase.h"
#include "System/BokGameModeBase.h"
#include "UMG/WBP_MainHUDBase.h"

#if !UE_BUILD_SHIPPING
#include "EggBase.h"
#endif

ABokPlayerControllerBase::ABokPlayerControllerBase()
{
	// Resources
	BP_BokPawn = FIND_CLASS(BP_BokPawn, "Blueprint/Gameplay");

	WBP_PauseMenu = FIND_CLASS(WBP_PauseMenu, "UMG");
}

void ABokPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	
}

void ABokPlayerControllerBase::SetupInputComponent() 
{
	Super::SetupInputComponent();

	static FName NAME_Pause("Pause");
	InputComponent->BindAction(NAME_Pause, EInputEvent::IE_Pressed, this, &ABokPlayerControllerBase::OnPause);

#if !UE_BUILD_SHIPPING
	static FName NAME_Debug_LevelSkip("Debug_LevelSkip");
	InputComponent->BindAction(NAME_Debug_LevelSkip, EInputEvent::IE_Pressed, this, &ABokPlayerControllerBase::Debug_LevelSkip);
#endif
}

void ABokPlayerControllerBase::OnPawnDied()
{
	// After 2 secs, respawn
	FTimerDelegate TimerCallback;
	TimerCallback.BindUObject(this, &ABokPlayerControllerBase::SpawnPawn);

	FTimerHandle Timer;
	GetWorld()->GetTimerManager().SetTimer(Timer, TimerCallback, 2.0f, false);
}

void ABokPlayerControllerBase::OnPause()
{
	ABokPawnBase* BokPawn = Cast<ABokPawnBase>(GetPawn());
	if (BokPawn && BokPawn->IsFrozen())
	{
		// No pause when frozen
		return;
	}

	// Pause!
	SetPause(true);

	// Add pause menu
	UUserWidget* PauseMenu = CreateWidget<UUserWidget>(this, WBP_PauseMenu);
	PauseMenu->AddToViewport();
}

void ABokPlayerControllerBase::SpawnPawn()
{
	// Kill the pawn
	APawn* CurrentPawn = GetPawn();
	if (CurrentPawn)
	{
		CurrentPawn->Destroy();
	}

	// Respawn
	FVector SpawnLoc = FVector::ZeroVector;
	FRotator SpawnRot = FRotator::ZeroRotator;

	TActorIterator<APlayerStart> It(GetWorld());
	if (It)
	{
		SpawnLoc = It->GetActorLocation();
		SpawnRot = It->GetActorRotation();
	}

	// Spawn new player
	UWorld* World = GetWorld();
	GOOSE_BAIL(World);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABokPawnBase* NewPawn = World->SpawnActor<ABokPawnBase>(BP_BokPawn, SpawnLoc, SpawnRot, SpawnParams);
	GOOSE_BAIL(NewPawn);

	// Finally possess the new pawn
	Possess(NewPawn);
}

void ABokPlayerControllerBase::Debug_LevelSkip()
{	
#if !UE_BUILD_SHIPPING
	// Force egg recount	
	ABokGameStateBase* GameState = GetWorld()->GetGameState<ABokGameStateBase>();
	GameState->CurrentEggCount = 0;

	UDamageType* DT = NewObject<UDamageType>();
	for (TActorIterator<AEggBase> It(GetWorld()); It; ++It)
	{
		It->FellOutOfWorld(*DT);
		It->Destroy();
	}

	ABokGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ABokGameModeBase>();
	GameMode->OnDoorEntered();
#endif
}
