// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ResultTypes.h"
// ReSharper disable once CppUnusedIncludeDirective : include used
#include "Scheduler.h"
#include "Zakazane/Future.h"
#include "Zakazane/Result.h"
#include "ZkzStagedExecutionStageStateId.h"

namespace Zkz::StagedExecution
{

template <class InIdType>
class TScheduler;

// Staged execution stage state machine:
// 1 - "Undefined": prerequisites of the stage (names of other stages that need to be completed before this gets executed)
//	are yet unknown.
// 2 - "Defined": prerequisites have been defined.
// 3 - "Executing": prerequisites have all completed, tasks are being executed.
// 4 - "Completed": all tasks have been collected and finished execution.
//
// New tasks are accepted until "all tasks collected" is called in states 1, 2 and 3.

using EStageStateId = EZkzStagedExecutionStageStateId;

/// Default stage state for the stage state variant. Should never be used.
struct FStageState_Unknown
{
	static constexpr EStageStateId Id = EStageStateId::Unknown;
};

template <class InIdType>
struct TStageState_Base
{
	using IdType = InIdType;

	InIdType StageId;

	explicit TStageState_Base(InIdType InStageId);
};

template <class InIdType>
struct TStageState_Pending : TStageState_Base<InIdType>
{
	struct FTaskEntry
	{
		InIdType Id;
		FTaskExecutionPromise ExecutionPromise;
	};

	bool bAllTasksCollected = false;

	TArray<FTaskEntry> Tasks;

	TArray<FStageCompletionPromise> StageCompletionPromises;

	explicit TStageState_Pending(InIdType InStageId);
	TStageState_Pending(TStageState_Pending&&) = default;
	TStageState_Pending(const TStageState_Pending&) = delete;
	TStageState_Pending& operator=(TStageState_Pending&&) = default;
	TStageState_Pending& operator=(const TStageState_Pending&) = delete;
};

template <class InIdType>
struct TStageState_Undefined : TStageState_Pending<InIdType>
{
	static constexpr EStageStateId Id = EStageStateId::Undefined;

	explicit TStageState_Undefined(InIdType InStageId);
};

template <class InIdType>
struct TStageState_Defined : TStageState_Pending<InIdType>
{
	static constexpr EStageStateId Id = EStageStateId::Defined;

	TArray<FFutureStageCompletion> FuturePrerequisiteCompletions;

	explicit TStageState_Defined(
		TStageState_Undefined<InIdType> StageState_Undefined,
		TArray<FFutureStageCompletion> InFuturePrerequisiteCompletions);
};

template <class InIdType>
struct TStageState_Executing : TStageState_Base<InIdType>
{
	static constexpr EStageStateId Id = EStageStateId::Executing;

	struct FTaskEntry
	{
		InIdType Id;
		FFutureTaskCompletion FutureCompletion;
	};

	bool bAllTasksCollected = false;

	TArray<FTaskEntry> Tasks;

	TArray<FStageCompletionPromise> StageCompletionPromises;

	// Note: Scheduler passed in optionally to add debug notifications
	explicit TStageState_Executing(
		TStageState_Defined<InIdType> StageState_Defined, TScheduler<InIdType>* DebugScheduler = nullptr);

private:
	static TArray<FTaskEntry> ExecuteAllTasks(
		const TArrayView<typename TStageState_Pending<InIdType>::FTaskEntry> PendingTasks,
		TScheduler<InIdType>* DebugScheduler = nullptr);
};

template <class InIdType>
struct TStageState_Completed : TStageState_Base<InIdType>
{
	static constexpr EStageStateId Id = EStageStateId::Completed;

	explicit TStageState_Completed(InIdType InStageId);
};

template <class InIdType>
using TStageState = TVariant<
	FStageState_Unknown,
	TStageState_Undefined<InIdType>,
	TStageState_Defined<InIdType>,
	TStageState_Executing<InIdType>,
	TStageState_Completed<InIdType>>;

// -- stage state api

namespace StageState
{

template <class InIdType>
const InIdType& GetStageId(const TStageState<InIdType>& State);

template <class InIdType>
TAddStageResult<InIdType> AddStage(
	TStageState<InIdType>& State,
	TScheduler<InIdType>& Scheduler,
	TArray<FFutureStageCompletion> FuturePrerequisiteCompletions,
	FOutputDevice* const OutputDevice);

template <class InIdType>
FFutureStageCompletion AddFollowUp(
	TStageState<InIdType>& State,
	FOutputDevice* const OutputDevice,
	FStringView FromStageName,
	FStringView ToStageName);

template <class InIdType>
TResult<FFutureTaskExecution, TAllTasksCollectedError<InIdType>> AddTaskToStage(
	TStageState<InIdType>& State, InIdType TaskId);

template <class InIdType>
void SetAllTasksAdded(
	TStageState<InIdType>& State, TScheduler<InIdType>& Scheduler, FOutputDevice* OutputDevice, FStringView StageName);

template <class InIdType>
EStageStateId GetId(const TStageState<InIdType>& State);

}  // namespace StageState

}  // namespace Zkz::StagedExecution

#include "StageState.tpp"
