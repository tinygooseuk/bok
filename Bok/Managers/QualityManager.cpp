// (c) 2019 TinyGoose Ltd., All Rights Reserved.

#include "QualityManager.h"

#include "Scalability.h"

int32 UQualityManager::GetQualityLevel()
{
	return Scalability::GetQualityLevels().GetSingleQualityLevel();
}

void UQualityManager::SetQualityLevel(int32 QualityLevel)
{
	Scalability::FQualityLevels Quality;
	Quality.SetFromSingleQualityLevel(QualityLevel);

	Scalability::SetQualityLevels(Quality);
}

FText UQualityManager::GetNameForQualityLevel(int32 QualityLevel)
{
	if (QualityLevel == -1)
	{
		return FText::FromString(TEXT("Custom"));
	}

	static const TCHAR* LEVEL_NAMES[] = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic") };
	return FText::FromString(LEVEL_NAMES[QualityLevel]);
}