// (c) 2019 TinyGoose Ltd., All Rights Reserved.
#pragma once

#include "Bok.h"
#include "GameFramework/GameStateBase.h"
#include "BokGameStateBase.generated.h"

UCLASS()
class BOK_API ABokGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	ABokGameStateBase();

	UPROPERTY(BlueprintReadWrite)
	int32 InitialEggCount;

	UPROPERTY(BlueprintReadWrite)
	int32 CurrentEggCount;

	UPROPERTY(BlueprintReadWrite)
	bool bLevelComplete;
};
