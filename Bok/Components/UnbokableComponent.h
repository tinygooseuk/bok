// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnbokableComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BOK_API UUnbokableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUnbokableComponent();
};
