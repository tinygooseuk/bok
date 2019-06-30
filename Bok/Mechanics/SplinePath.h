// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#pragma once
#include "Bok.h"

#include "GameFramework/Actor.h"
#include "SplinePath.generated.h"

UCLASS()
class BOK_API ASplinePath : public AActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Furniture")
	FVector GetAttachedActorPreviousLocation() const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Furniture")
	AActor *GetAttachedActor() const;
};
