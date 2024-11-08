// Fill out your copyright notice in the Description page of Project Settings.


#include "MyExperienceManagerComponent.h"


// Sets default values for this component's properties
UMyExperienceManagerComponent::UMyExperienceManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	
	// ...
}

bool UMyExperienceManagerComponent::ShouldShowLoadingScreen(FString& OutReason) const
{
	if (LoadState != ELyraExperienceLoadState::Loaded)
	{
		OutReason = TEXT("Experience still loading");
		return true;
	}
	else
	{
		return false;
	}
}

void UMyExperienceManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	
	FTimerHandle DummyHandle;
	GetWorld()->GetTimerManager().SetTimer(DummyHandle, this, &ThisClass::OnExperienceFullLoadCompleted, 2.f, /*bLooping=*/ false);

}

void UMyExperienceManagerComponent::OnExperienceFullLoadCompleted()
{
	LoadState = ELyraExperienceLoadState::Loaded;
}

