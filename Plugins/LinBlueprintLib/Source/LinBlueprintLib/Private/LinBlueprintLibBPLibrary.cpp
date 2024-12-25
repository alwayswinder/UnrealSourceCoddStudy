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

static FString RetrieveAssetPath(const FAssetData& InAssetData)
{
	int32 LastDot = -1;
	FString Path = InAssetData.GetObjectPathString();

	// Strip asset name from object path
	if (Path.FindLastChar('.', LastDot))
	{
		Path.LeftInline(LastDot);
	}

	return Path;
}

static FString RetrieveAssetTypeName(const FAssetData& InAssetData)
{
	if (UAssetDefinitionRegistry* AssetDefinitionRegistry = UAssetDefinitionRegistry::Get())
	{
		const UAssetDefinition* AssetDefinition = AssetDefinitionRegistry->GetAssetDefinitionForAsset(InAssetData);
		if (AssetDefinition)
		{
			return AssetDefinition->GetAssetDisplayName().ToString();
		}
	}

	return InAssetData.AssetClassPath.ToString();
}
static void RefreshAssetInformationInternal(const TArray<FAssetData>& Assets, const FString& InFilename, FString& OutAssetName, FString& OutAssetPath, FString& OutAssetType, FString& OutAssetTypeName, FText& OutPackageName, FColor& OutAssetTypeColor)
{
	// Initialize display-related members
	FString Filename = InFilename;
	FString Extension = FPaths::GetExtension(Filename);
	FString TempAssetName = "_Default";
	FString TempAssetPath = Filename;
	FString TempAssetType = "_Default";
	FString TempAssetTypeName = "_Default";
	FString TempPackageName = Filename;
	FColor TempAssetColor = FColor(		// Copied from ContentBrowserCLR.cpp
		127 + FColor::Red.R / 2,	// Desaturate the colors a bit (GB colors were too.. much)
		127 + FColor::Red.G / 2,
		127 + FColor::Red.B / 2,
		200); // Opacity


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
			TempAssetPath = RetrieveAssetPath(AssetData);
			TempAssetType = AssetData.AssetClassPath.ToString();
			TempAssetTypeName = RetrieveAssetTypeName(AssetData);

			const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			const TSharedPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(AssetData.GetClass()).Pin();

			if (AssetTypeActions.IsValid())
			{
				TempAssetColor = AssetTypeActions->GetTypeColor();
			}
			else
			{
				TempAssetColor = FColor::White;
			}
		}
		else
		{
			TempAssetName = RetrieveAssetName(Assets[0]);
			TempAssetPath = RetrieveAssetPath(Assets[0]);
			TempAssetTypeName = RetrieveAssetTypeName(Assets[0]);

			for (int32 i = 1; i < Assets.Num(); ++i)
			{
				TempAssetName += TEXT(";") + RetrieveAssetName(Assets[i]);
			}

			TempAssetType = "_Default";
			TempAssetColor = FColor::White;
		}

		// Beautify the package name
		TempPackageName = TempAssetPath + "." + TempAssetName;
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
		TempPackageName = Filename; // Put back original package name if the try failed
		FText Tmp = FText::FromString("_Default");
		
		TempAssetType = FText::Format(Tmp, FText::FromString(FPaths::GetExtension(Filename).ToUpper())).ToString();
		TempAssetTypeName = TempAssetType;

		// Attempt to make package name relative to one of the project roots instead of a full absolute path
		TArray<FSourceControlProjectInfo> CustomProjects = ISourceControlModule::Get().GetCustomProjects();
		for (const FSourceControlProjectInfo& ProjectInfo : CustomProjects)
		{
			FStringView RelativePackageName;
			if (FPathViews::TryMakeChildPathRelativeTo(TempPackageName, ProjectInfo.ProjectDirectory, RelativePackageName))
			{
				TempPackageName = FPaths::Combine(TEXT("/"), FPaths::GetBaseFilename(ProjectInfo.ProjectDirectory), RelativePackageName);
				break;
			}
		}
	}

	// Finally, assign the temp variables to the member variables
	OutAssetName = TempAssetName;
	OutAssetPath = TempAssetPath;
	OutAssetType = TempAssetType;
	OutAssetTypeName = TempAssetTypeName;
	OutAssetTypeColor = TempAssetColor;
	OutPackageName = FText::FromString(TempPackageName);
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

TArray<FString> ULinBlueprintLibBPLibrary::GetChangedList()
{
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
			FString AssetPathStr;
			FString AssetTypeStr;
			FString AssetTypeNameStr;
			FText PackageName;
			FColor AssetTypeColor;
			RefreshAssetInformationInternal(Assets.IsValid() ? *Assets : NoAssets, Item->GetFilename(), AssetNameStr, AssetPathStr, AssetTypeStr, AssetTypeNameStr, PackageName, AssetTypeColor);
			ActorNames.Add(AssetNameStr);
		}

	}
	return ActorNames;
}

