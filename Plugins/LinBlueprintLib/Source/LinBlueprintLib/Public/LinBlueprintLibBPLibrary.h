// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "LinBlueprintLibBPLibrary.generated.h"

/* 
*	Function library class.
*	Each function in it is expected to be static and represents blueprint node that can be called in any blueprint.
*
*	When declaring function you can define metadata for the node. Key function specifiers will be BlueprintPure and BlueprintCallable.
*	BlueprintPure - means the function does not affect the owning object in any way and thus creates a node without Exec pins.
*	BlueprintCallable - makes a function which can be executed in Blueprints - Thus it has Exec pins.
*	DisplayName - full name of the node, shown when you mouse over the node and in the blueprint drop down menu.
*				Its lets you name the node using characters not allowed in C++ function names.
*	CompactNodeTitle - the word(s) that appear on the node.
*	Keywords -	the list of keywords that helps you to find node when you search for it using Blueprint drop-down menu. 
*				Good example is "Print String" node which you can find also by using keyword "log".
*	Category -	the category your node will be under in the Blueprint drop-down menu.
*
*	For more info on custom blueprint nodes visit documentation:
*	https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation
*/
UCLASS()
class ULinBlueprintLibBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Execute Sample function", Keywords = "LinBlueprintLib sample test testing"), Category = "LinBlueprintLibTesting")
	static float LinBlueprintLibSampleFunction(float Param);
	

 	UFUNCTION(BlueprintCallable, CustomThunk, meta = (DisplayName = "Add Datatable Row", DefaultToSelf = "Object",
 		CustomStructureParam = "CustomStruct", AdvancedDisplay = "Object"), Category = "DataTable")
	static bool AddRowDT(int32 CustomStruct, UDataTable* DT, FName NewRowName);
 	static bool Generic_AddRowDT(void* Target, FProperty* Pro, UDataTable* DT, FName NewRowName);
 	DECLARE_FUNCTION(execAddRowDT)
 	{
 		Stack.MostRecentProperty = nullptr;
 		Stack.StepCompiledIn<FProperty>(NULL);
 		void* AAddr = Stack.MostRecentPropertyAddress;
 		FProperty* Property = CastField<FProperty>(Stack.MostRecentProperty);
 		P_GET_OBJECT(UDataTable, DT);
 		P_GET_PROPERTY(FNameProperty, NewRowName);
 		P_FINISH;
 		P_NATIVE_BEGIN;
 		*(bool*)Z_Param__Result = Generic_AddRowDT(AAddr, Property, DT, NewRowName);
 		P_NATIVE_END;
 	}
	
	UFUNCTION(BlueprintCallable, Category = "WorldTools")
	static TArray<FString> GetChangedList();
};
