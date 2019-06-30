// (c) 2019 TinyGoose Ltd., All Rights Reserved.
#include "DeadlyComponent.h"

UDeadlyComponent::UDeadlyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}