// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#pragma once

#include "Bok.h"
#include "Components/ActorComponent.h"
#include "DeadlyComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BOK_API UDeadlyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDeadlyComponent();
};
