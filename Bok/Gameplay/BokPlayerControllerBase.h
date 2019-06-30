// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#pragma once

#include "Bok.h"
#include "GameFramework/PlayerController.h"
#include "BokPlayerControllerBase.generated.h"

UCLASS()
class BOK_API ABokPlayerControllerBase : public APlayerController
{
	GENERATED_BODY()
	
public:
	ABokPlayerControllerBase();
	
	// AActor /////////////////////////
	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

	// Called by the Pawn when it dies
	void OnPawnDied();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnLevelComplete();

	// Handles pausing the game
	UFUNCTION()
	void OnPause();

	// Called after a timer to (re-)spawn the pawn
	UFUNCTION()
	void SpawnPawn();

	UFUNCTION()
	void Debug_LevelSkip();

private:
	// Resources /////////////////////////
	UPROPERTY()
	class UClass* BP_BokPawn;
	
	UPROPERTY()
	class UClass* WBP_PauseMenu;
};
