// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "LoadingProcessInterface.h"
#include "MyExperienceManagerComponent.generated.h"

enum class ELyraExperienceLoadState
{
	Unloaded,
	Loading,
	LoadingGameFeatures,
	LoadingChaosTestingDelay,
	ExecutingActions,
	Loaded,
	Deactivating
};

UCLASS()
class UESOURCECODESTU_API UMyExperienceManagerComponent final : public UGameStateComponent, public ILoadingProcessInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMyExperienceManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
	
	virtual void BeginPlay();
	
private:

	void OnExperienceFullLoadCompleted();
	
	ELyraExperienceLoadState LoadState = ELyraExperienceLoadState::Unloaded;
};
