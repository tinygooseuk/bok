// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#include "WBP_MainHUDBase.h"

// Engine
#include "Materials/MaterialInstanceDynamic.h"

// UMG
#include "Components/Image.h"
#include "Components/TextBlock.h"

// Game
#include "WBP_EggCounterBase.h"

UWBP_MainHUDBase::UWBP_MainHUDBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CurrentEggCount(-1)
{

}

void UWBP_MainHUDBase::NativeConstruct()
{
	Super::NativeConstruct();
}

void UWBP_MainHUDBase::SetEggCounts(int32 NumEggsLeft, int32 NumEggsTotal)
{
	if (CurrentEggCount != NumEggsLeft)
	{
		RunEggCollectedAnimation();
		CurrentEggCount = NumEggsLeft;
	}

	// Set the image fill amount required
	int32 NumEggsCollected = NumEggsTotal - NumEggsLeft;
	float FillPerc = float(NumEggsCollected) / float(NumEggsTotal);
	EggImage->SetFillLevel(FillPerc);

	// Update the text label
	//FString EggString = FString::Printf(TEXT("%d / %d"), NumEggsCollected, NumEggsTotal);
	FString EggString = FString::Printf(TEXT("%d %s Left"), NumEggsLeft, (NumEggsLeft == 1) ? TEXT("Egg") : TEXT("Eggs"));
	EggsText->SetText(FText::FromString(EggString));
}

void UWBP_MainHUDBase::RunEggCollectedAnimation()
{
	GOOSE_BAIL(EggImage);
	EggImage->RunCollectedAnimation_BP();
}
