// Copyright ZAKAZANE Studio. All Rights Reserved.

#include "ZkzWorldActorsValidatorBase.h"

#include "EditorWorldUtils.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "UObject/GCObjectScopeGuard.h"
#include "Utility/ZkzValidationLogCategory.h"
#include "WorldPartition/WorldPartition.h"
#include "Zakazane/Actor.h"
#include "Zakazane/ReturnIfMacros.h"

// ReSharper disable CppTooWideScope

UZkzWorldActorsValidatorBase::UZkzWorldActorsValidatorBase()
{
	FallbackDisallowedPaths.Emplace(TEXT("/_Generated_"));
}

bool UZkzWorldActorsValidatorBase::CanValidateAsset_Implementation(
	const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	ZKZ_RETURN_IF(!Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext), false);

	return InObject->IsA<UWorld>();
}

/// #TODO #Validation Validators that need to load the world should have their logic moved to separate Tasks.
/// Currently World Lifecycle is repeated for each Validator. Once Loaded Actors should be reused by each *ValidatorTask*
/// @note When validating outside the Editor with bSkipTransientActors = false, some Actor might be reinitialized
/// without being destroyed after the previous validator finished its job.
/// Changing Validator logic to load and run ValidatorTasks on loaded data will fix that problem.
/// Since these Managers are not validated, duplicates shouldn't cause any problems during the process.
void UZkzWorldActorsValidatorBase::ForEachWorldActor(
	const UWorld* InWorld,
	const TFunction<void(const AActor&, const FTransform&)>& InFunction,
	const bool bSkipTransientActors)
{
	ZKZ_RETURN_IF_INVALID(InWorld);

	UWorld::InitializationValues WorldInitializationValues;
	WorldInitializationValues.InitializeScenes(false);
	WorldInitializationValues.AllowAudioPlayback(false);
	WorldInitializationValues.RequiresHitProxies(false);
	WorldInitializationValues.CreatePhysicsScene(false);
	WorldInitializationValues.CreateNavigation(false);
	WorldInitializationValues.CreateAISystem(false);
	WorldInitializationValues.ShouldSimulatePhysics(false);
	WorldInitializationValues.EnableTraceCollision(false);
	WorldInitializationValues.SetTransactional(false);
	WorldInitializationValues.CreateFXSystem(false);
	WorldInitializationValues.CreateWorldPartition(true);
	WorldInitializationValues.EnableWorldPartitionStreaming(false);

	TOptional<FScopedEditorWorld> ScopedEditorWorld;
	TOptional<TGCObjectScopeGuard<UWorldPartition>> WorldPartitionGCGuard;

	if (IsRunningCommandlet() && !IsRunningCookCommandlet() && !InWorld->IsInitialized())
	{
		// const_cast is necessary for ScopedEditorWorld (TSoftObjectPtr deprecation)
		UWorld* const MutableWorld = const_cast<UWorld*>(InWorld);
		ScopedEditorWorld.Emplace(MutableWorld, WorldInitializationValues);

		if (UWorldPartition* const WorldPartition = MutableWorld->GetWorldPartition())
		{
			WorldPartitionGCGuard.Emplace(WorldPartition);
		}
	}

	TMap<const ULevel*, FTransform> LevelTransforms;
	for (const ULevel* const Level : InWorld->GetLevels())
	{
		LevelTransforms.Add(Level, FTransform::Identity);
	}

	TArray<TUniquePtr<TGCObjectScopeGuard<UWorldPartition>>> SubWorldPartitionGCGuards;
	TArray<TUniquePtr<TGCObjectScopeGuard<UWorld>>> SubWorldGCGuards;

	// FScopedEditorWorld calls DestroyWorld() on destruction, which corrupts LevelInstance's references.
	// Manual cleanup is needed for manually initialized SubLevels
	TArray<TFunction<void()>> SafeSubLevelCleanups;
	ON_SCOPE_EXIT
	{
		for (const TFunction<void()>& CleanupSubLevel : SafeSubLevelCleanups)
		{
			CleanupSubLevel();
		}
	};

	Zkz::ForEachActorWithLoadingInWorld(
		InWorld,
		[&](const AActor& Actor)
		{
			ZKZ_RETURN_IF(bSkipTransientActors && Actor.HasAnyFlags(RF_Transient), true);

			const ULevel* const ActorLevel = Actor.GetLevel();
			const FTransform* const CurrentTransformPtr =
				IsValid(ActorLevel) ? LevelTransforms.Find(ActorLevel) : nullptr;
			const FTransform CurrentTransform =
				CurrentTransformPtr != nullptr ? *CurrentTransformPtr : FTransform::Identity;

			InFunction(Actor, CurrentTransform);

			if (Actor.IsA<ALevelInstance>())
			{
				const ALevelInstance* const LevelInstance = Cast<ALevelInstance>(&Actor);
				ZKZ_RETURN_IF_INVALID(LevelInstance, true);

				const TSoftObjectPtr<UWorld>& WorldAsset = LevelInstance->GetWorldAsset();
				ZKZ_RETURN_IF(WorldAsset.IsNull(), true);

				const UWorld* const SubLevelWorld = Cast<UWorld>(WorldAsset.LoadSynchronous());
				ZKZ_RETURN_IF_INVALID(SubLevelWorld, true);

				UWorld* const MutableSubLevelWorld = const_cast<UWorld*>(SubLevelWorld);
				SubWorldGCGuards.Add(MakeUnique<TGCObjectScopeGuard<UWorld>>(MutableSubLevelWorld));

				// Prevent GC from cleaning SubLevelWorldPartition
				if (UWorldPartition* const SubLevelWorldPartition = MutableSubLevelWorld->GetWorldPartition())
				{
					SubWorldPartitionGCGuards.Add(
						MakeUnique<TGCObjectScopeGuard<UWorldPartition>>(SubLevelWorldPartition));
				}

				// Manual World Lifecycle (Initialize and Queue Cleanup)
				// FScopedEditorWorld can't be used on SubLevels cause it calls
				// DestroyWorld, which corrupts LevelInstances inside the outer world
				if (IsRunningCommandlet() && !IsRunningCookCommandlet() && !MutableSubLevelWorld->bIsWorldInitialized)
				{
					MutableSubLevelWorld->WorldType = EWorldType::Editor;
					MutableSubLevelWorld->InitWorld(WorldInitializationValues);
					MutableSubLevelWorld->UpdateWorldComponents(true, false);

					SafeSubLevelCleanups.Add(
						[MutableSubLevelWorld]()
						{
							ZKZ_RETURN_IF_INVALID(MutableSubLevelWorld);
							MutableSubLevelWorld->CleanupWorld();
						});
				}

				const FTransform NewAccumulatedTransform = LevelInstance->GetActorTransform() * CurrentTransform;
				for (const ULevel* const SubLevel : SubLevelWorld->GetLevels())
				{
					LevelTransforms.Add(SubLevel, NewAccumulatedTransform);
				}
			}

			return true;
		},
		true,
		true);
}