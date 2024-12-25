// Copyright Epic Games, Inc. All Rights Reserved.

#include "LinBlueprintLibBPLibrary.h"

#include "ActorFolder.h"
#include "ActorFolderDesc.h"
#include "AssetDefinitionRegistry.h"
#include "AssetToolsModule.h"
#include "LinBlueprintLib.h"
#include "DataTableEditorUtils.h"
#include "FileHelpers.h"
#include "ISourceControlModule.h"
#include "SourceControlAssetDataCache.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"
#include "Algo/Count.h"
#include "Editor/SourceControlWindows/Public/SourceControlWindows.h"

static FString RetrieveAssetName(const FAssetData& InAssetData)
{
	static const FName NAME_ActorLabel(TEXT("ActorLabel"));

	if (InAssetData.FindTag(NAME_ActorLabel))
	{
		FString ResultAssetName;
		InAssetData.GetTagValue(NAME_ActorLabel, ResultAssetName);
		return ResultAssetName;
	}
	else if (InAssetData.FindTag(FPrimaryAssetId::PrimaryAssetDisplayNameTag))
	{
		FString ResultAssetName;
		InAssetData.GetTagValue(FPrimaryAssetId::PrimaryAssetDisplayNameTag, ResultAssetName);
		return ResultAssetName;
	}
	else if (InAssetData.AssetClassPath == UActorFolder::StaticClass()->GetClassPathName())
	{
		FString ActorFolderPath = UActorFolder::GetAssetRegistryInfoFromPackage(InAssetData.PackageName).GetDisplayName();
		if (!ActorFolderPath.IsEmpty())
		{
			return ActorFolderPath;
		}
	}

	return InAssetData.AssetName.ToString();
}

static void RefreshAssetInformationInternal(const TArray<FAssetData>& Assets, const FString& InFilename, FString& OutAssetName)
{
	// Initialize display-related members
	FString Filename = InFilename;
	FString Extension = FPaths::GetExtension(Filename);
	FString TempAssetName = "_Default";
	FString TempAssetPath = Filename;
	FString TempPackageName = Filename;
	
	bool bIsPackageExtension = 
		FPackageName::IsPackageExtension(*Extension) ||
		FPackageName::IsVerseExtension(*Extension);

	if (Assets.Num() > 0)
	{
		auto IsNotRedirector = [](const FAssetData& InAssetData) { return !InAssetData.IsRedirector(); };
		int32 NumUserFacingAsset = Algo::CountIf(Assets, IsNotRedirector);

		if (NumUserFacingAsset == 1)
		{
			const FAssetData& AssetData = *Algo::FindByPredicate(Assets, IsNotRedirector);

			TempAssetName = RetrieveAssetName(AssetData);
		}
		else
		{
			TempAssetName = RetrieveAssetName(Assets[0]);

			for (int32 i = 1; i < Assets.Num(); ++i)
			{
				TempAssetName += TEXT(";") + RetrieveAssetName(Assets[i]);
			}
		}
	}
	else if (bIsPackageExtension && FPackageName::TryConvertFilenameToLongPackageName(Filename, TempPackageName))
	{
		// Fake asset name, asset path from the package name
		TempAssetPath = TempPackageName;

		int32 LastSlash = -1;
		if (TempPackageName.FindLastChar('/', LastSlash))
		{
			TempAssetName = TempPackageName;
			TempAssetName.RightChopInline(LastSlash + 1);
		}
	}
	else
	{
		TempAssetName = FPaths::GetCleanFilename(Filename);
	}

	// Finally, assign the temp variables to the member variables
	OutAssetName = TempAssetName;
}


ULinBlueprintLibBPLibrary::ULinBlueprintLibBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

float ULinBlueprintLibBPLibrary::LinBlueprintLibSampleFunction(float Param)
{
	return -1;
}


bool ULinBlueprintLibBPLibrary::Generic_AddRowDT(void* Target, FProperty* Pro, UDataTable* DT, FName NewRowName)
{
 	if (!Target || !Pro || !DT)
 	{
 		return false;
 	}
 	FStructProperty* StructPro = CastField<FStructProperty>(Pro);
 	if (StructPro->Struct != DT->RowStruct)
 	{
 		return false;
 	}
 	TMap<FName, uint8*>& DTMap = const_cast<TMap<FName, uint8*>&>(DT->GetRowMap());
 	UScriptStruct& EmptyUsingStruct = *DT->RowStruct;
 	uint8* NewRawRowData = (uint8*)FMemory::Malloc(EmptyUsingStruct.GetStructureSize());
 	EmptyUsingStruct.InitializeStruct(NewRawRowData);
 	EmptyUsingStruct.CopyScriptStruct(NewRawRowData, Target);
 	DTMap.Add(NewRowName, NewRawRowData);
 	DT->Modify();
 	FDataTableEditorUtils::BroadcastPostChange(DT, FDataTableEditorUtils::EDataTableChangeInfo::RowData);
 	FDataTableEditorUtils::BroadcastPostChange(DT, FDataTableEditorUtils::EDataTableChangeInfo::RowList);
 	return true;
}

TArray<FString> ULinBlueprintLibBPLibrary::GetChangedListLabels()
{
	FEditorFileUtils::SaveCurrentLevel();
	//TArray<FString> Filenames = SourceControlHelpers::GetSourceControlLocations();
	TArray<FString> Filenames;
	FString PrijectPathAbs = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	Filenames.Add(PrijectPathAbs);
	TArray<FString> ActorNames;

	if (Filenames.Num() > 0)
	{
		ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
		SourceControlProvider.Execute(ISourceControlOperation::Create<FUpdateStatus>(), Filenames);

		//SourceControlProvider.GetState(Filenames, Items, EStateCacheUsage::Use);

		// Get a list of all the checked out packages
		TArray<FString> PackageNames;
		TArray<UPackage*> LoadedPackages;
		TMap<FString, FSourceControlStatePtr> PackageStates;
		FEditorFileUtils::FindAllSubmittablePackageFiles(PackageStates, true);
		
		for (TMap<FString, FSourceControlStatePtr>::TConstIterator PackageIter(PackageStates); PackageIter; ++PackageIter)
		{
			const FString PackageName = *PackageIter.Key();

			UPackage* Package = FindPackage(nullptr, *PackageName);
			if (Package != nullptr)
			{
				LoadedPackages.Add(Package);
			}

			PackageNames.Add(PackageName);
		}
		
		TArray<FString> AllFiles = SourceControlHelpers::PackageFilenames(PackageNames);
		
		TArray<FSourceControlStateRef> States;
		SourceControlProvider.GetState(AllFiles, States, EStateCacheUsage::Use);
		
		for (const auto& Item : States)
		{
			FSourceControlAssetDataCache& AssetDataCache = ISourceControlModule::Get().GetAssetDataCache();
			FAssetDataArrayPtr Assets;
			AssetDataCache.GetAssetDataArray(Item, Assets);
			
			static TArray<FAssetData> NoAssets;
			FString AssetNameStr;
			RefreshAssetInformationInternal(Assets.IsValid() ? *Assets : NoAssets, Item->GetFilename(), AssetNameStr);
			ActorNames.Add(AssetNameStr);
		}
		//States[0]->GetHistoryItem()
	}
	return ActorNames;
}

FString ULinBlueprintLibBPLibrary::GetUserName()
{
	return FPaths::GameUserDeveloperFolderName();
}

