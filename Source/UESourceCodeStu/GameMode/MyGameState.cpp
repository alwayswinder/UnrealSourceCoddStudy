// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameState.h"

#include "MyExperienceManagerComponent.h"


// Sets default values
AMyGameState::AMyGameState()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	ExperienceManagerComponent = CreateDefaultSubobject<UMyExperienceManagerComponent>(TEXT("ExperienceManagerComponent"));
}


