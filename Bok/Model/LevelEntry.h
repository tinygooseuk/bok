// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#pragma once

#include "Bok.h"
#include "Engine/DataTable.h"
#include "LevelEntry.generated.h"

USTRUCT(BlueprintType)
struct BOK_API FLevelEntry : public FTableRowBase
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UWorld> LevelAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText LevelName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<class USoundCue> LevelBGM;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bShowControlsOverlay = false;

	// Static empty level
	static FLevelEntry Empty;
	FORCEINLINE bool IsValid() const { return !LevelAsset.IsNull(); }
};
