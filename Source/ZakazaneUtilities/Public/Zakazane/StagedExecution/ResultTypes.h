// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "IdTraits.h"
#include "Zakazane/Future.h"
#include "Zakazane/Result.h"

namespace Zkz::StagedExecution
{

using FStageCompletionPromise = TScopedPromise<void>;
using FFutureStageCompletion = TCancelableFuture<void>;

using FTaskCompletionPromise = TScopedPromise<void>;
using FFutureTaskCompletion = TCancelableFuture<void>;

using FTaskExecutionPromise = TScopedPromise<FTaskCompletionPromise>;
using FFutureTaskExecution = TCancelableFuture<FTaskCompletionPromise>;

template <class InIdType>
struct TStageAlreadyAddedError
{
	using IdType = InIdType;

	IdType StageId;

	explicit TStageAlreadyAddedError(IdType InStageId);
};

template <class InIdType>
struct TAllTasksCollectedError
{
	using IdType = InIdType;

	IdType StageId;
	IdType TaskId;

	TAllTasksCollectedError(IdType InStageId, IdType InTaskId)
		: StageId{MoveTemp(InStageId)}, TaskId{MoveTemp(InTaskId)}
	{
	}
};

template <class InIdType>
using TAddTaskToStageResult = TResult<FFutureTaskExecution, TAllTasksCollectedError<InIdType>>;

template <class InIdType>
struct TStageCircularDependencyError
{
	using IdType = InIdType;
	using FCycle = TArray<IdType, TInlineAllocator<8>>;

	IdType StageId;

	TArray<IdType> PrerequisiteIds;

	FCycle Cycle;

	explicit TStageCircularDependencyError(IdType InStageId, TArray<IdType> InPrerequisiteIds, FCycle InCycle);
};

template <class InIdType>
using TAddStageError = TVariant<TStageAlreadyAddedError<InIdType>, TStageCircularDependencyError<InIdType>>;

template <class InIdType>
FString ToString(const TStageAlreadyAddedError<InIdType>& StageAlreadyAddedError);
template <class InIdType>
FString ToString(const TStageCircularDependencyError<InIdType>& StageCircularDependencyError);

template <class InIdType>
using TAddStageResult = TResult<void, TAddStageError<InIdType>>;

template <class InIdType>
using TAddTaskResult = TResult<FFutureTaskExecution, TAddStageError<InIdType>>;

template <class InIdType>
FString ToString(const TAddStageError<InIdType>& AddStageError);

// -- template definitions

template <class InIdType>
TStageAlreadyAddedError<InIdType>::TStageAlreadyAddedError(IdType InStageId) : StageId{MoveTemp(InStageId)}
{
}

template <class InIdType>
TStageCircularDependencyError<InIdType>::TStageCircularDependencyError(
	IdType InStageId, TArray<IdType> InPrerequisiteIds, FCycle InCycle)
	: StageId{MoveTemp(InStageId)}, PrerequisiteIds{MoveTemp(InPrerequisiteIds)}, Cycle{MoveTemp(InCycle)}
{
}

template <class InIdType>
FString ToString(const TAddStageError<InIdType>& AddStageError)
{
	return Visit([](const auto& V) { return ToString<InIdType>(V); }, AddStageError);
}

template <class InIdType>
FString ToString(const TStageAlreadyAddedError<InIdType>& StageAlreadyAddedError)
{
	return FString::Format(
		TEXT(R"(Stage "{0}" has already been added. Aborting operation.)"),
		{TIdTraits<InIdType>::GetLogString(StageAlreadyAddedError.StageId)});
}

template <class InIdType>
FString ToString(const TStageCircularDependencyError<InIdType>& StageCircularDependencyError)
{
	FStringBuilderBase Result;

	Result.Append("Adding stage \"")
		.Append(TIdTraits<InIdType>::GetLogString(StageCircularDependencyError.StageId))
		.Append("\" with prerequisite(s) {");

	{
		bool bFirst = true;
		for (const InIdType& PrerequisiteId : StageCircularDependencyError.PrerequisiteIds)
		{
			if (!bFirst)
			{
				Result.Append(", ");
			}

			Result.Append("\"").Append(TIdTraits<InIdType>::GetLogString(PrerequisiteId)).Append("\"");

			bFirst = false;
		}
	}

	Result.Append("} would introduce cycle ");

	{
		bool bFirst = true;
		for (const InIdType& CycleId : StageCircularDependencyError.Cycle)
		{
			if (!bFirst)
			{
				Result.Append(" -> ");
			}

			Result.Append("\"").Append(TIdTraits<InIdType>::GetLogString(CycleId)).Append("\"");
			bFirst = false;
		}
	}

	Result.Append(". Aborting operation.");

	return Result.ToString();
}

}  // namespace Zkz::StagedExecution
