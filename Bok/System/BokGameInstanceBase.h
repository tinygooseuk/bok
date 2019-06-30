// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#pragma once

#include "Bok.h"
#include "Engine/GameInstance.h"
#include "BokGameInstanceBase.generated.h"

UCLASS()
class BOK_API UBokGameInstanceBase : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UBokGameInstanceBase();
	static UBokGameInstanceBase* Get(UObject* WorldContext);

	UFUNCTION(BlueprintCallable)
	class ULevelManager* GetLevelManager() const;

	//#savegame TODO: this should go in a SaveGame!
	UFUNCTION(BlueprintCallable)
	void ResetProgress();

	UFUNCTION(BlueprintCallable)
	FText GetTotalEggCountText() const;

	UFUNCTION(BlueprintCallable)
	int32 GetTotalEggCount() const;

	UFUNCTION(BlueprintCallable)
	void SetTotalEggCount(int32 InEggCount);

private:
	// State /////////////////////////
	//#savegame TODO: this should go in a SaveGame!
	int32 TotalEggsCollected = 0;

	// Managers /////////////////////////
	UPROPERTY()
	class ULevelManager* LevelManager;
	
	// Resources /////////////////////////
	UPROPERTY()
	class UDataTable* DB_Levels;
};
