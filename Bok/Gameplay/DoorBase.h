// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#pragma once

#include "Bok.h"
#include "GameFramework/Actor.h"
#include "DoorBase.generated.h"

UCLASS()
class BOK_API ADoorBase : public AActor
{
	GENERATED_BODY()
	
public:	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Open Door", Category = Doors)
	void OpenDoor_BP();
};
