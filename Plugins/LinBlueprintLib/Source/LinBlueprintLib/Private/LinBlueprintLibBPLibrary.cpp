// Copyright Epic Games, Inc. All Rights Reserved.

#include "LinBlueprintLibBPLibrary.h"
#include "LinBlueprintLib.h"
#include "DataTableEditorUtils.h"

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
