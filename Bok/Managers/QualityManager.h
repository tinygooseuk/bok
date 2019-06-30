// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#pragma once

#include "Bok.h"
#include "UObject/NoExportTypes.h"
#include "QualityManager.generated.h"

UCLASS()
class BOK_API UQualityManager : public UObject
{
	GENERATED_BODY()

private:
	UQualityManager() = default;

public:
	UFUNCTION(BlueprintCallable)
	static int32 GetQualityLevel();
	
	UFUNCTION(BlueprintCallable)
	static void SetQualityLevel(int32 QualityLevel);

	UFUNCTION(BlueprintCallable)
	static FText GetNameForQualityLevel(int32 QualityLevel);
};
