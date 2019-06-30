// (c) 2019 TinyGoose Ltd., All Rights Reserved.
#include "UnbokableComponent.h"

UUnbokableComponent::UUnbokableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}
