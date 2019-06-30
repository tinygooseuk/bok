// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#include "BokGameInstanceBase.h"

// Engine
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

// Game
#include "Managers/LevelManager.h"

UBokGameInstanceBase::UBokGameInstanceBase()
{
	LevelManager = CreateDefaultSubobject<ULevelManager>(TEXT("LevelManager"));
}

/*static*/ UBokGameInstanceBase* UBokGameInstanceBase::Get(UObject* WorldContextObj)
{
	FWorldContext* WorldCtx = GEngine->GetWorldContextFromWorld(WorldContextObj->GetWorld());
	UBokGameInstanceBase* GI = Cast<UBokGameInstanceBase>(WorldCtx->OwningGameInstance);
	GOOSE_BAIL_RETURN(GI, nullptr);

	return GI;
}

ULevelManager* UBokGameInstanceBase::GetLevelManager() const
{
	return LevelManager;
}

void UBokGameInstanceBase::ResetProgress()
{
	TotalEggsCollected = 0;
	LevelManager->ResetProgress();
}


FText UBokGameInstanceBase::GetTotalEggCountText() const
{
	const int32 Dozens = TotalEggsCollected / 12;
	const int32 Singles = TotalEggsCollected % 12;

	FString OutString;

	if (Dozens == 0)
	{
		if (Singles == 1)
		{
			OutString = FString::Printf(TEXT("%d Egg"), Singles);
		}
		else
		{
			OutString = FString::Printf(TEXT("%d Eggs"), Singles);
		}
	}
	else if (Singles == 0)
	{
		OutString = FString::Printf(TEXT("%d Dozen Eggs"), Dozens);
	}
	else
	{
		OutString = FString::Printf(TEXT("%d Dozen and %d Eggs"), Dozens, Singles);
	}

	return FText::FromString(OutString);
}

int32 UBokGameInstanceBase::GetTotalEggCount() const
{
	return TotalEggsCollected;
}

void UBokGameInstanceBase::SetTotalEggCount(int32 InEggCount)
{
	TotalEggsCollected = InEggCount;
}
