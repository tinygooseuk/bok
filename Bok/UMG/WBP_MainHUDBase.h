// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#pragma once

#include "Bok.h"
#include "Blueprint/UserWidget.h"
#include "WBP_MainHUDBase.generated.h"

UCLASS()
class BOK_API UWBP_MainHUDBase : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UWBP_MainHUDBase(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable)
	void SetEggCounts(int32 NumEggsLeft, int32 NumEggsTotal);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ShowLevelName_BP(const FText& LevelName);

	UFUNCTION(BlueprintCallable)
	void RunEggCollectedAnimation();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetTargetImageFillPercent_BP(float InPerc);

protected: /* friend class ABokPlayerControllerBase */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnLevelComplete_BP();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnBokEnteredDoor_BP();

	UFUNCTION(BlueprintImplementableEvent)
	void ShowThrobber_BP();

	UFUNCTION(BlueprintImplementableEvent)
	void HideThrobber_BP();

protected:
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UWBP_EggCounterBase* EggImage;

	UPROPERTY(meta=(BindWidget))
	class UTextBlock* EggsText;

private:
	int32 CurrentEggCount;

	friend class ABokGameModeBase;
};
