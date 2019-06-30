// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#pragma once

#include "Bok.h"
#include "GameFramework/GameModeBase.h"
#include "BokGameModeBase.generated.h"

UCLASS()
class BOK_API ABokGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABokGameModeBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Events /////////////////////////
	void OnEggFellOutOfWorld();
	
	UFUNCTION(BlueprintCallable)
	void OnDoorEntered();

	// UI Functions /////////////////////////
	UFUNCTION(BlueprintCallable)
	void UpdateEggCountUI();

	UFUNCTION(BlueprintCallable)
	void SetThrobberVisible(bool bShowThrobber);

	UFUNCTION(BlueprintCallable)
	void SetLetterBoxAmount(float Amount);

	UFUNCTION(BlueprintCallable)
	float GetLetterBoxAmount();

private:
	FORCEINLINE class ABokGameStateBase* GetBokGameState() const;

	UFUNCTION(BlueprintCallable)
	void ShowEndGameScreen();

	// State /////////////////////////
	bool bCountedInitialEggs;
	int ThrobberRequests;

	// Resources /////////////////////////
	UPROPERTY()
	class UClass* WBP_MainHUD;

	UPROPERTY()
	class UClass* WBP_GameComplete;

	// References /////////////////////////
	UPROPERTY()
	class AAmbientSound* BGMCueActor;

	UPROPERTY()
	class UMaterialInstanceDynamic* LetterboxMaterial;

	// Private Functions /////////////////////////
	void CountEggs(int32& OutEggCount);
	void OnCompleteLevel();

	// UI /////////////////////////
	UPROPERTY()
	class UWBP_MainHUDBase* MainHUD;
};
