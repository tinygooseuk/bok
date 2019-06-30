// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#include "LevelManager.h"

// Engine
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

// Game
#include "Model/BokModel.h"
#include "System/BokGameInstanceBase.h"
#include "System/BokGameModeBase.h"

DEFINE_LOG_CATEGORY(LogLevelManager);

ULevelManager::ULevelManager()
	: CurrentLevelIdx(-1)
	, bNewLevelIsReady(false)
{
	LevelsDB = BokModel::GetDB_Levels();

	// Set initial level
	CurrentLevel = FLevelEntry();
	
#if WITH_EDITOR
	CurrentLevel.LevelName = FText::FromString(TEXT("Bok"));
	CurrentLevel.bShowControlsOverlay = true;
#endif

	UE_LOG(LogLevelManager, Log, TEXT("LevelManager Initialised with %d levels"), LevelsDB->GetRowMap().Num());
}

ULevelManager* ULevelManager::GetLevelManager(UObject* WorldContextObject)
{
	UBokGameInstanceBase* GI = UBokGameInstanceBase::Get(WorldContextObject);
	GOOSE_BAIL_RETURN(GI, nullptr);

	return GI->GetLevelManager();
}

void ULevelManager::PreloadNewLevel(const FLevelEntry& Level)
{
	GOOSE_BAIL(!LoadingHandle.IsValid());

	// Request level streaming
	FStreamableDelegate Delegate;
	Delegate.BindUObject(this, &ULevelManager::OnNextLevelWasLoaded);
	LoadingHandle = Streamer.RequestAsyncLoad(Level.LevelAsset.ToSoftObjectPath(), Delegate, FStreamableManager::DefaultAsyncLoadPriority, false, false, Level.LevelName.ToString());
	
	// Store ref. to new level
	NewLevel = Level;

	// Add throbber
	ABokGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ABokGameModeBase>();
	GOOSE_BAIL(GameMode);

	GameMode->SetThrobberVisible(true);
}
void ULevelManager::CancelLoadNewLevel()
{
	if (LoadingHandle.IsValid())
	{
		LoadingHandle->CancelHandle();
		LoadingHandle = nullptr;
	}

	// Remove throbber
	ABokGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ABokGameModeBase>();
	GOOSE_BAIL(GameMode);

	GameMode->SetThrobberVisible(false);

	NewLevel = FLevelEntry::Empty;
	bNewLevelIsReady = false;
}

bool ULevelManager::IsNewLevelLoading() const
{
	return LoadingHandle.IsValid() && LoadingHandle->IsLoadingInProgress();
}

bool ULevelManager::IsNewLevelLoaded() const
{
	return bNewLevelIsReady;
}
void ULevelManager::GoToNewLevel()
{
	GOOSE_BAIL(IsNewLevelLoaded());

	bNewLevelIsReady = false;
	CurrentLevel = NewLevel;
	NewLevel = FLevelEntry::Empty;
	CurrentLevelIdx = GetIndexForLevelEntry(CurrentLevel);

	UWorld* Level = Cast<UWorld>(LoadingHandle->GetLoadedAsset());
	GOOSE_BAIL(Level);

	GEngine->SetClientTravel(GetWorld(), *Level->GetName(), ETravelType::TRAVEL_Absolute);
	LoadingHandle.Reset();
}

void ULevelManager::ResetProgress() 
{
	CurrentLevelIdx = 0;
	CurrentLevel = GetLevelEntryForCurrentLevel();
}

FLevelEntry ULevelManager::GetLevelEntryForCurrentLevel() const
{
	return GetLevelEntryForLevel(CurrentLevelIdx);
}

FLevelEntry ULevelManager::GetLevelEntryForNextLevel() const
{
	return GetLevelEntryForLevel(CurrentLevelIdx + 1);
}

bool ULevelManager::IsNextLevelAvailable() const 
{
	GOOSE_BAIL_RETURN(LevelsDB, false);

	int32 NumLevels = LevelsDB->GetRowMap().Num();
	return CurrentLevelIdx+1 < NumLevels;
}

FLevelEntry ULevelManager::GetLevelEntryForLevel(int32 LevelIdx) const
{
	GOOSE_BAIL_RETURN(LevelsDB, FLevelEntry::Empty);

	// Load definitions for all levels
	TArray<FLevelEntry*> AllLevels;
	LevelsDB->GetAllRows(TEXT(""), AllLevels);

	// Return requestedlevel
	if (LevelIdx == -1)
	{
		return CurrentLevel;
	}

	GOOSE_BAIL_RETURN(LevelIdx < AllLevels.Num(), FLevelEntry::Empty);
	return *AllLevels[LevelIdx];
}

int32 ULevelManager::GetIndexForLevelEntry(FLevelEntry Level) const
{
	GOOSE_BAIL_RETURN(LevelsDB, -1);

	// Load definitions for all levels
	TArray<FLevelEntry*> AllLevels;
	LevelsDB->GetAllRows(TEXT(""), AllLevels);

	int32 OutIdx = 0;
	for (FLevelEntry* Entry : AllLevels)
	{
		if (Entry->LevelAsset == Level.LevelAsset)
		{
			return OutIdx;
		}

		OutIdx++;
	}

	return -1;
}

void ULevelManager::OnNextLevelWasLoaded()
{
	bNewLevelIsReady = LoadingHandle->HasLoadCompleted();

	// Remove throbber
	ABokGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ABokGameModeBase>();
	GOOSE_BAIL(GameMode);

	GameMode->SetThrobberVisible(false);
}