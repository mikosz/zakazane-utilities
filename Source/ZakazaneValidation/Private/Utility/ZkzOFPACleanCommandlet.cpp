// Copyright ZAKAZANE Studio. All Rights Reserved.

#include "ZkzOFPACleanCommandlet.h"

#include "WorldPartition/WorldPartition.h"
#include "Zakazane/Actor.h"
#include "Zakazane/Asset.h"
#include "Zakazane/AssetFunctionLibrary.h"
#include "Zakazane/ContinueIfMacros.h"
#include "ZkzAssetValidationUtils.h"
#include "ZkzValidationLogCategory.h"

UZkzOFPACleanCommandlet::UZkzOFPACleanCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UZkzOFPACleanCommandlet::Main(const FString& Params)
{
	UE_LOG(LogZkzCommandlet, Display, TEXT("Starting Zkz OFPA Clean Commandlet..."));

	TSet<FString> MapsToProcess;
	ParseMapArguments(Params, MapsToProcess);

	if (MapsToProcess.IsEmpty())
	{
		UE_LOG(LogZkzCommandlet, Error, TEXT("No maps found. Use -map=/Game/Map1,/Game/Map2 or -folder=/Game/Folder"));
		return 1;
	}

	TSet<UClass*> ClassesNames;
	ParseClassNameArguments(Params, ClassesNames);
	if (ClassesNames.IsEmpty())
	{
		UE_LOG(LogZkzCommandlet, Error, TEXT("No classes found. Use -class=MyClass1,MyClass2"));
		return 1;
	}

	const bool bPreviewMode = FParse::Param(*Params, TEXT("Preview"));
	if (bPreviewMode)
	{
		UE_LOG(LogZkzCommandlet, Display, TEXT("Running in preview mode."));
	}

	for (const FString& MapName : MapsToProcess)
	{
		UE_LOG(LogZkzCommandlet, Display, TEXT("Processing Map: %s"), *MapName);
		if (ProcessMap(MapName, ClassesNames, bPreviewMode))
		{
			UE_LOG(LogZkzCommandlet, Display, TEXT("Successfully processed: %s."), *MapName);
		}
		else
		{
			UE_LOG(LogZkzCommandlet, Error, TEXT("Failed to process: %s."), *MapName);
		}
	}

	UE_LOG(LogZkzCommandlet, Display, TEXT("Commandlet finished successfully."));
	return 0;
}

void UZkzOFPACleanCommandlet::ParseMapArguments(const FString& Params, TSet<FString>& OutMapsToProcess)
{
	FString MapArgs;
	if (FParse::Value(*Params, TEXT("Map="), MapArgs, false))
	{
		TArray<FString> TempParsedMaps;
		MapArgs.ParseIntoArray(TempParsedMaps, TEXT(","), true);

		OutMapsToProcess.Append(TempParsedMaps);
	}

	FString FoldersArg;
	if (FParse::Value(*Params, TEXT("Folder="), FoldersArg))
	{
		TArray<FString> FolderPaths;
		FoldersArg.ParseIntoArray(FolderPaths, TEXT(","), true);

		for (const FString& Folder : FolderPaths)
		{
			TArray<FString> FilesInFolder;
			FPackageName::FindPackagesInDirectory(FilesInFolder, Folder);

			for (const FString& FilePath : FilesInFolder)
			{
				ZKZ_CONTINUE_IF(FPaths::GetExtension(FilePath, true) != FPackageName::GetMapPackageExtension());

				FString LongPackageName;
				ZKZ_CONTINUE_IF(!FPackageName::TryConvertFilenameToLongPackageName(FilePath, LongPackageName))

				OutMapsToProcess.Emplace(LongPackageName);
			}
		}
	}
}

void UZkzOFPACleanCommandlet::ParseClassNameArguments(const FString& Params, TSet<UClass*>& OutClassesToClean)
{
	FString ClassNames;
	TArray<FString> TempParsedClasses;
	if (FParse::Value(*Params, TEXT("Class="), ClassNames, false))
	{
		ClassNames.ParseIntoArray(TempParsedClasses, TEXT(","), true);
	}

	for (const FString& ClassName : TempParsedClasses)
	{
		UClass* Class = FindFirstObjectSafe<UClass>(*ClassName);

		if (!IsValid(Class))
		{
			UE_LOG(LogZkzCommandlet, Warning, TEXT("Class %s not found."), *ClassName);
			continue;
		}

		OutClassesToClean.Emplace(Class);
	}
}

bool UZkzOFPACleanCommandlet::ProcessMap(
	const FString& MapName, const TSet<UClass*>& ClassesToClean, const bool bPreviewMode) const
{
	using namespace Zkz::Game::Validation;

	UWorld* const World = FZkzAssetValidationUtils::PrepareWorldForInitByName(MapName);
	if (!IsValid(World))
	{
		UE_LOG(LogZkzCommandlet, Error, TEXT("Failed to load world: %s"), *MapName);
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(World))
		{
			if (UWorldPartition* const WorldPartition = World->GetWorldPartition())
			{
				if (WorldPartition->IsInitialized())
				{
					WorldPartition->Uninitialize();
				}
			}
			World->ClearWorldComponents();
			World->CleanupWorld();

			World->RemoveFromRoot();
			CollectGarbage(RF_NoFlags);
		}
	};

	TMap<AActor*, TArray<UActorComponent*>> ComponentsToCleanByActor;
	if (World->IsPartitionedWorld())
	{
		Zkz::ForEachActorWithLoadingInWorld(
			World,
			[this, &ClassesToClean, &ComponentsToCleanByActor](AActor& Actor)
			{
				GatherComponentsFromActors(&Actor, ClassesToClean, ComponentsToCleanByActor);
				return true;
			},
			false,
			true);
	}
	else
	{
		UE_LOG(LogZkzCommandlet, Display, TEXT("Map %s is not using World Partition."), *MapName);
	}

	if (ComponentsToCleanByActor.IsEmpty())
	{
		UE_LOG(LogZkzCommandlet, Display, TEXT("No invalid components found in %s."), *MapName);
		return true;
	}

	bPreviewMode ? PrintPreviewForMap(MapName, ComponentsToCleanByActor)
				 : RemoveComponentsFromActors(ComponentsToCleanByActor);

	return true;
}

void UZkzOFPACleanCommandlet::GatherComponentsFromActors(
	AActor* Actor,
	const TSet<UClass*>& ClassesToClean,
	TMap<AActor*, TArray<UActorComponent*>>& OutComponentsToCleanByActor)
{
	using namespace Zkz::Game::Validation;

	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);

	for (UActorComponent* const Comp : Components)
	{
		ZKZ_CONTINUE_IF(
			Comp->GetArchetype() != Comp->GetClass()->GetDefaultObject()
			|| Comp->CreationMethod == EComponentCreationMethod::Instance
			|| Comp->CreationMethod == EComponentCreationMethod::UserConstructionScript)

		ZKZ_CONTINUE_IF(!ClassesToClean.Contains(Comp->GetClass()))

		OutComponentsToCleanByActor.FindOrAdd(Actor).Emplace(Comp);
	}
}

void UZkzOFPACleanCommandlet::PrintPreviewForMap(
	const FString& MapName, const TMap<AActor*, TArray<UActorComponent*>>& ComponentsToCleanByActor)
{
	UE_LOG(LogZkzCommandlet, Display, TEXT("Invalid components found in %s:"), *MapName);
	for (const auto& [Actor, Components] : ComponentsToCleanByActor)
	{
		UE_LOG(LogZkzCommandlet, Display, TEXT("\tActor - %s:"), *Actor->GetActorLabel());
		for (const UActorComponent* const Comp : Components)
		{
			UE_LOG(LogZkzCommandlet, Display, TEXT("\t\tComponent: %s"), *Comp->GetReadableName());
		}
	}
}

void UZkzOFPACleanCommandlet::RemoveComponentsFromActors(
	TMap<AActor*, TArray<UActorComponent*>>& ComponentsToCleanByActor)
{
	for (const auto& [Actor, Components] : ComponentsToCleanByActor)
	{
		for (UActorComponent* const Comp : Components)
		{
			UE_LOG(
				LogZkzCommandlet,
				Display,
				TEXT("Deleted %s from %s"),
				*Comp->GetReadableName(),
				*Actor->GetActorLabel());

			Comp->DestroyComponent();
		}

		UZkzAssetFunctionLibrary::CheckoutAndSaveAsset(Actor);
	}
}
