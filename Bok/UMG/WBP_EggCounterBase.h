// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_EggCounterBase.generated.h"

UCLASS()
class BOK_API UWBP_EggCounterBase : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeom, float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable)
	void SetFillLevel(float FillLevel, bool bInstantaneously = false);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void RunCollectedAnimation_BP();

protected:
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UImage* EggImage;

private:
	float CurrentFillLevel = 0.0f;
	float TargetFillLevel = 0.0f;	
};
