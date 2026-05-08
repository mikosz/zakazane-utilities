// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Commandlets/Commandlet.h"

#include "ZkzOFPACleanCommandlet.generated.h"

class FPackageSourceControlHelper;

/// Commandlet for cleaning leftover components from OFPA Actors
/// Possible arguments (it's possible to use both map and folder arguments at the same time):
///		* -Map="/Game/Path/To/Map1,/Game/Path/To/Map2" - paths to maps that should be processed
///		* -Folder="/Game/Path/To/Folder,/Game/Enviro/PLI1" - paths to folders that contain maps that should be processed
///		* -Class="ZkzClassComponent,SceneComponent" - Invalid components of given classes will be removed from the Actor
UCLASS()
class ZAKAZANEVALIDATION_API UZkzOFPACleanCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UZkzOFPACleanCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	/// Builds maps to process from given map and/or folder arguments
	static void ParseMapArguments(const FString& Params, TSet<FString>& OutMapsToProcess);
	
	/// Builds classes to clean from a given classes argument
	static void ParseClassNameArguments(const FString& Params, TSet<UClass*>& OutClassesToClean);
	
	/// Prepares map and iterates through all Actors
	bool ProcessMap(const FString& MapName, const TSet<UClass*>& ClassesToClean, bool bPreviewMode) const;
	
	/// Looks for components that are considered invalid
	static void GatherComponentsFromActors(AActor* Actor, const TSet<UClass*>& ClassesToClean, TMap<AActor*, TArray<UActorComponent*>>& OutComponentsToCleanByActor);
	
	/// Prints preview of components that would be removed
	static void PrintPreviewForMap(const FString& MapName, const TMap<AActor*, TArray<UActorComponent*>>& ComponentsToCleanByActor);
	
	/// Removes invalid components from actor
	static void RemoveComponentsFromActors(TMap<AActor*, TArray<UActorComponent*>>& ComponentsToCleanByActor);
	
};
