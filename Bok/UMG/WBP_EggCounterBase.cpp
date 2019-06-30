// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#include "WBP_EggCounterBase.h"

// Engine
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

static FName NAME_FillLevel(TEXT("FillLevel"));

void UWBP_EggCounterBase::NativeConstruct()
{
	Super::NativeConstruct();

	UMaterialInstanceDynamic* EggMat = EggImage->GetDynamicMaterial();
	EggMat->SetScalarParameterValue(NAME_FillLevel, CurrentFillLevel);
}

void UWBP_EggCounterBase::NativeTick(const FGeometry& MyGeom, float DeltaSeconds)
{
	Super::NativeTick(MyGeom, DeltaSeconds);

	CurrentFillLevel = FMath::FInterpTo(CurrentFillLevel, TargetFillLevel, DeltaSeconds, 1.5f);
	
	UMaterialInstanceDynamic* EggMat = EggImage->GetDynamicMaterial();
	EggMat->SetScalarParameterValue(NAME_FillLevel, CurrentFillLevel);
}

void UWBP_EggCounterBase::SetFillLevel(float FillLevel, bool bInstantaneously)
{
	TargetFillLevel = FillLevel;

	if (bInstantaneously)
	{
		CurrentFillLevel = TargetFillLevel;
	}
}