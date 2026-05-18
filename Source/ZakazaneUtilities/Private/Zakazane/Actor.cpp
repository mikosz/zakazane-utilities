#include "Zakazane/Actor.h"

#include "UObject/GCObjectScopeGuard.h"

#if WITH_EDITOR
#include "EngineUtils.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionHelpers.h"
#include "Zakazane/ContinueIfMacros.h"
#include "Zakazane/ReturnIfMacros.h"
#endif

#if WITH_EDITOR
void Zkz::ForEachActorWithLoadingInWorld(
	const UWorld* InWorld,
	const TFunction<bool(AActor&)>& InCallback,
	const bool bInRecursive,
	const bool bInKeepReferences)
{
	ZKZ_RETURN_IF_INVALID(InWorld);

	TArray<TSoftObjectPtr<const ALevelInstance>> LevelInstancesToVisit;

	const auto VisitActorFunc = [bInRecursive, &InCallback, &LevelInstancesToVisit](AActor& Actor) -> bool
	{
		if (bInRecursive && Actor.IsA<ALevelInstance>())
		{
			const ALevelInstance* const LevelInstance = Cast<ALevelInstance>(&Actor);
			ZKZ_RETURN_IF_INVALID(LevelInstance, true);

			LevelInstancesToVisit.Push(TSoftObjectPtr<const ALevelInstance>{LevelInstance});
		}

		const bool bContinue = Invoke(InCallback, Actor);
		return bContinue;
	};

	const auto VisitWorldFunc = [bInKeepReferences, &VisitActorFunc](const UWorld* World)
	{
		ZKZ_RETURN_IF_INVALID(World);

		if (UWorldPartition* const WorldPartition = World->GetWorldPartition(); IsValid(WorldPartition))
		{
			TGCObjectScopeGuard WorldPartitionGuard(WorldPartition);

			FWorldPartitionHelpers::FForEachActorWithLoadingParams Params;
			Params.bKeepReferences = bInKeepReferences;

			FWorldPartitionHelpers::ForEachActorWithLoading(
				WorldPartition,
				[&VisitActorFunc](const FWorldPartitionActorDescInstance* ActorDescInstance)
				{
					AActor* const LoadedActor = ActorDescInstance->GetActor();
					ZKZ_RETURN_IF_INVALID(LoadedActor, true);

					const bool bContinue = Invoke(VisitActorFunc, *LoadedActor);
					return bContinue;
				},
				Params);
		}
		else
		{
			for (TActorIterator<AActor> It{World}; It; ++It)
			{
				AActor* const Actor = *It;
				ZKZ_CONTINUE_IF_INVALID(Actor);

				const bool bContinue = Invoke(VisitActorFunc, *Actor);
				if (!bContinue)
				{
					break;
				}
			}
		}
	};

	Invoke(VisitWorldFunc, InWorld);

	while (!LevelInstancesToVisit.IsEmpty())
	{
		ALevelInstance* const LevelInstance =
			const_cast<ALevelInstance*>(LevelInstancesToVisit.Pop().LoadSynchronous());
		ZKZ_CONTINUE_IF_INVALID(LevelInstance);
		TGCObjectScopeGuard LevelInstanceGuard(LevelInstance);

		const TSoftObjectPtr<UWorld>& WorldAsset = LevelInstance->GetWorldAsset();
		ZKZ_CONTINUE_IF(WorldAsset.IsNull());

		UWorld* const SubLevelWorld = Cast<UWorld>(WorldAsset.LoadSynchronous());
		ZKZ_CONTINUE_IF_INVALID(SubLevelWorld);
		TGCObjectScopeGuard SubWorldGuard(SubLevelWorld);

		Invoke(VisitWorldFunc, SubLevelWorld);
	}
}
#endif
