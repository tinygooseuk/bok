// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#include "BokGameModeBase.h"

// Engine
#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/AmbientSound.h"
#include "Sound/SoundCue.h"

// Game
#include "Gameplay/BokPawnBase.h"
#include "Gameplay/BokPlayerControllerBase.h"
#include "Gameplay/EggBase.h"
#include "Gameplay/DoorBase.h"
#include "Managers/LevelManager.h"
#include "System/BokGameStateBase.h"
#include "System/BokGameInstanceBase.h"
#include "UMG/WBP_MainHUDBase.h"


ABokGameModeBase::ABokGameModeBase()
	: bCountedInitialEggs(false)
	, ThrobberRequests(0)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f / 3.0f; // = 3Hz

	// Set default classes
	DefaultPawnClass = ABokPawnBase::StaticClass();
	PlayerControllerClass = ABokPlayerControllerBase::StaticClass();
	GameStateClass = ABokGameStateBase::StaticClass();

	// Get resources
	WBP_MainHUD = FIND_CLASS(WBP_MainHUD, "UMG");
	WBP_GameComplete = FIND_CLASS(WBP_GameComplete, "UMG");
}

void ABokGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// Create the main HUD under PC#0
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	GOOSE_BAIL(PC);

	MainHUD = CreateWidget<UWBP_MainHUDBase>(PC, WBP_MainHUD);
	MainHUD->AddToViewport();

	// Update the egg counter
	ABokGameStateBase* BokState = GetBokGameState();
	CountEggs(BokState->InitialEggCount);
	CountEggs(BokState->CurrentEggCount);
	UpdateEggCountUI();

	// Show the level name
	ULevelManager* LevelMgr = UBokGameInstanceBase::Get(this)->GetLevelManager();
	FLevelEntry ThisLevel = LevelMgr->GetLevelEntryForCurrentLevel();
	MainHUD->ShowLevelName_BP(ThisLevel.LevelName);

	// Start up the BGM
	USoundCue* BGMCue = ThisLevel.LevelBGM.LoadSynchronous(); //#hitch
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	AAmbientSound* BGMActor = GetWorld()->SpawnActor<AAmbientSound>(SpawnParams);
	BGMActor->GetAudioComponent()->SetSound(BGMCue);
	BGMActor->GetAudioComponent()->bOverrideAttenuation = true;
	BGMActor->GetAudioComponent()->bAllowSpatialization = false;
	BGMActor->GetAudioComponent()->Play();

	// Find the postFX actor
	TActorIterator<APostProcessVolume> It(GetWorld());
	if (It && *It)
	{
		APostProcessVolume* Volume = *It;

		TArray<FWeightedBlendable>& Blendables = Volume->Settings.WeightedBlendables.Array;
		for (int32 Idx = 0; Idx < Blendables.Num(); Idx++)
		{	
			UMaterial* Material = Cast<UMaterial>(Blendables[Idx].Object);
			if (Material && Material->GetName().Contains(TEXT("Letterbox")))
			{
				UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Material, Material->GetOuter());
				Blendables[Idx].Object = MID;

				LetterboxMaterial = MID;
			}
		}
	}
}

void ABokGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Find all the eggs
	ABokGameStateBase* BokState = GetBokGameState();

	if (!BokState->bLevelComplete)
	{
		if (!bCountedInitialEggs)
		{
			CountEggs(BokState->InitialEggCount);
			bCountedInitialEggs = true;
		}

		CountEggs(BokState->CurrentEggCount);
	}
}

void ABokGameModeBase::OnEggFellOutOfWorld()
{
	// Recount eggs
	ABokGameStateBase* BokState = GetBokGameState();
	CountEggs(BokState->CurrentEggCount);

	// Update UI
	UpdateEggCountUI();

	// Check for win condition
	if (BokState->CurrentEggCount == 0)
	{
		OnCompleteLevel();
	}
}

void ABokGameModeBase::OnDoorEntered()
{
	ULevelManager* LevelMgr = UBokGameInstanceBase::Get(this)->GetLevelManager();
	GOOSE_BAIL(LevelMgr);

	// Tell the gamestate that it's finished (no egg count update)
	GetBokGameState()->bLevelComplete = true;

	// Tell the player's HUD via hook
	GOOSE_BAIL(MainHUD);
	MainHUD->OnBokEnteredDoor_BP();
}

void ABokGameModeBase::CountEggs(int32& OutEggCount)
{
	OutEggCount = 0;
	for (TActorIterator<AEggBase> EggIt(GetWorld()); EggIt; ++EggIt)
	{
		OutEggCount++;
	}
}

void ABokGameModeBase::OnCompleteLevel()
{
	// Update HUD
	MainHUD->OnLevelComplete_BP();

	// Start to load next level if available
	ULevelManager* LevelMgr = UBokGameInstanceBase::Get(this)->GetLevelManager();
	if (LevelMgr->IsNextLevelAvailable())
	{
		FLevelEntry NextLevel = LevelMgr->GetLevelEntryForNextLevel();
		LevelMgr->PreloadNewLevel(NextLevel);
	}
	
	// Open all doors
	for (TActorIterator<ADoorBase> It(GetWorld()); It; ++It)
	{
		ADoorBase* Door = *It;
		GOOSE_BAIL_CONTINUE(Door);

		Door->OpenDoor_BP();
	}
}

void ABokGameModeBase::UpdateEggCountUI()
{
	GOOSE_BAIL(MainHUD);

	ABokGameStateBase* BokGameState = GetBokGameState();
	MainHUD->SetEggCounts(BokGameState->CurrentEggCount, BokGameState->InitialEggCount);
}

void ABokGameModeBase::SetThrobberVisible(bool bShowThrobber)
{
	ThrobberRequests += bShowThrobber ? +1 : -1;
	GOOSE_CHECK(ThrobberRequests >= 0);

	GOOSE_BAIL(MainHUD);

	if (ThrobberRequests == 0)
	{
		MainHUD->HideThrobber_BP();
	}
	else if (ThrobberRequests == 1)
	{
		MainHUD->ShowThrobber_BP();
	}
}

void ABokGameModeBase::SetLetterBoxAmount(float Amount)
{
	GOOSE_BAIL(LetterboxMaterial);

	static FName NAME_Amount(TEXT("Amount"));
	LetterboxMaterial->SetScalarParameterValue(NAME_Amount, Amount);
}

float ABokGameModeBase::GetLetterBoxAmount()
{
	GOOSE_BAIL_RETURN(LetterboxMaterial, 0.0f);

	static FName NAME_Amount(TEXT("Amount"));

	float Value;
	LetterboxMaterial->GetScalarParameterValue(NAME_Amount, Value);
	return Value;
}

ABokGameStateBase* ABokGameModeBase::GetBokGameState() const
{
	return GetGameState<ABokGameStateBase>();
}

void ABokGameModeBase::ShowEndGameScreen()
{
	// Add game complete screen!
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	GOOSE_BAIL(PC);

	UUserWidget* GameComplete = CreateWidget<UUserWidget>(PC, WBP_GameComplete);
	GameComplete->AddToViewport();
}