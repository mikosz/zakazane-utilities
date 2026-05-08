// Copyright ZAKAZANE Studio. All Rights Reserved.


#include "ZkzWorldActorsValidatorBase.h"

#include "ZkzAssetValidationUtils.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "WorldPartition/WorldPartition.h"
#include "Zakazane/Actor.h"
#include "Zakazane/ReturnIfMacros.h"

UZkzWorldActorsValidatorBase::UZkzWorldActorsValidatorBase()
{
	FallbackDisallowedPaths.Emplace(TEXT("/_Generated_"));
}

bool UZkzWorldActorsValidatorBase::CanValidateAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InObject,
	FDataValidationContext& InContext) const
{
	ZKZ_RETURN_IF(!Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext), false);

	return InObject->IsA<UWorld>();
}

void UZkzWorldActorsValidatorBase::ForEachWorldActor(
	const UWorld* InWorld,
	const TFunction<void(const AActor&, const FTransform&)>& InFunction)
{
	ZKZ_RETURN_IF_INVALID(InWorld);

	UWorld* World = const_cast<UWorld*>(InWorld);

	// This logic makes the world to load and initialize in commandlet
	// Without this Zkz::ForEachActorWithLoadingInWorld won't return any actors
	// This is skipped for validation in editor because it causes crashes
	if (IsRunningCommandlet() && !IsRunningCookCommandlet())
	{
		FZkzAssetValidationUtils::PrepareWorldForInitByAsset(const_cast<UWorld*>(InWorld));
	}

	ON_SCOPE_EXIT
	{
		if (IsRunningCommandlet() && !IsRunningCookCommandlet())
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
		}
	};

	TMap<const ULevel*, FTransform> LevelTransforms;
	for (const ULevel* const Level : World->GetLevels())
	{
		LevelTransforms.Add(Level, FTransform::Identity);
	}

	Zkz::ForEachActorWithLoadingInWorld(
		World,
		[&](const AActor& Actor)
		{
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

				// New Parent Transform to keep the position in World Space
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