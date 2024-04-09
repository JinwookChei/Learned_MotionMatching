// Copyright Epic Games, Inc. All Rights Reserved.

#include "LearnedMMGameMode.h"
#include "LearnedMMCharacter.h"
#include "UObject/ConstructorHelpers.h"

ALearnedMMGameMode::ALearnedMMGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
