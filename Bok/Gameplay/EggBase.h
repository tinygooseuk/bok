// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#pragma once

#include "Bok.h"
#include "GameFramework/Actor.h"
#include "EggBase.generated.h"

UCLASS()
class BOK_API AEggBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AEggBase();

	virtual void BeginPlay() override;
	virtual void FellOutOfWorld(const class UDamageType& DamageType) override;

	UFUNCTION(BlueprintCallable)
	void SetXray(bool bIsXray);
	
	UFUNCTION(BlueprintCallable)
	bool IsFrozen() const;
	
	UFUNCTION(BlueprintCallable)
	void SetFrozen(bool bInFrozen);

protected:
	// Components /////////////////////////
	UPROPERTY(BlueprintReadOnly)
	class UStaticMeshComponent* EggMeshComponent;

	// Resources /////////////////////////
	UPROPERTY(EditDefaultsOnly, DisplayName = "Collection Sound Effect", Category = "Egg")
	class USoundWave* R_CollectedSound;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Collection Final Egg Sound Effect", Category = "Egg")
	class USoundWave* R_LastEggCollectedSound;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Visual Flair BP", Category = "Egg")
	class UClass* R_VisualFlairBlueprintClass;

	// State /////////////////////////
	UPROPERTY(EditAnywhere)
	bool bIsFrozen;
};
