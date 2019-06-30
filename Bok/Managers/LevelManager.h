// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#pragma once

#include "Bok.h"

// Engine
#include "UObject/NoExportTypes.h"
#include "Engine/StreamableManager.h"
#include "SharedPointer.h"

// Game
#include "Model/LevelEntry.h"
#include "LevelManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLevelManager, Log, All);

UCLASS()
class BOK_API ULevelManager : public UObject
{
	GENERATED_BODY()
	
public:
	ULevelManager();

	UFUNCTION(BlueprintCallable, BlueprintPure, meta=(WorldContext="WorldContextObject"))
	static ULevelManager* GetLevelManager(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable)
	void PreloadNewLevel(const FLevelEntry& Level);

	UFUNCTION(BlueprintCallable)
	void CancelLoadNewLevel();
	
	UFUNCTION(BlueprintCallable)
	bool IsNewLevelLoading() const;

	UFUNCTION(BlueprintCallable)
	bool IsNewLevelLoaded() const;

	UFUNCTION(BlueprintCallable)
	void GoToNewLevel();

	UFUNCTION(BlueprintCallable)
	void ResetProgress();
	
	UFUNCTION(BlueprintCallable)
	FLevelEntry GetLevelEntryForCurrentLevel() const;
	
	UFUNCTION(BlueprintCallable)
	FLevelEntry GetLevelEntryForNextLevel() const;

	UFUNCTION(BlueprintCallable)
	bool IsNextLevelAvailable() const;

private:
	FLevelEntry GetLevelEntryForLevel(int32 LevelIdx) const;
	int32 GetIndexForLevelEntry(FLevelEntry Level) const;

	UFUNCTION()
	void OnNextLevelWasLoaded();

	UPROPERTY()
	FLevelEntry CurrentLevel;

	UPROPERTY()
	int32 CurrentLevelIdx;

	UPROPERTY()
	FLevelEntry NewLevel;

	UPROPERTY()
	bool bNewLevelIsReady;

	FStreamableManager Streamer;
	TSharedPtr<FStreamableHandle> LoadingHandle;

	UPROPERTY()
	class UDataTable* LevelsDB;
};
