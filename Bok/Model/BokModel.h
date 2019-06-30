#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#define DEFINE_DATA_TABLE(Name, Path)											\
	const FString Name = TEXT(Path);											\
	inline UDataTable* Get##Name()												\
	{																			\
		return ConstructorHelpers::FObjectFinder<UDataTable>(*Name).Object;		\
	}

namespace BokModel
{
	DEFINE_DATA_TABLE(DB_Levels, "DataTable'/Game/Data/DB_Levels.DB_Levels'");
}
