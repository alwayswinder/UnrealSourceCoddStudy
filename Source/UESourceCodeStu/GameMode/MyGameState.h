// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MyGameState.generated.h"

class UMyExperienceManagerComponent;

UCLASS()
class UESOURCECODESTU_API AMyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMyGameState();
	
private:
	UPROPERTY()
	TObjectPtr<UMyExperienceManagerComponent> ExperienceManagerComponent;
};
