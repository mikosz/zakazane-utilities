// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Zakazane/ExecutionGraph/Error.h"
#include "Zakazane/ExecutionGraph/ResultTypes.h"
#include "Zakazane/ExecutionGraph/ZkzExecutionGraphJobStateId.h"

namespace Zkz::ExecutionGraph::Private
{

using EJobStateId = EZkzExecutionGraphJobStateId;

/// JOB STATES
/// - FJobState_Base
///		- FJobState_Default (concrete)
/// 	- FJobState_Incomplete_Base
/// 		- FJobState_Stub (concrete)
/// 		- FJobState_DefinedTask (concrete)
/// 		- FJobState_IncompleteStage_Base
/// 			- FJobState_PendingStage_Base
/// 				- FJobState_StageStub (concrete)
/// 				- FJobState_DefinedStage (concrete)
/// 			- FJobState_ExecutingStage (concrete)
/// 			- FJobState_ExecutingTask (concrete)
/// - FJobState_Completed (concrete)

/// NOTE:
/// State structs may contain promises, but they don't contain futures. This is because, for thread safety (and
/// simplicity), states may never initiate state transitions. These are always performed by the scheduler. Therefore
/// futures are only used on the scheduler side, where they are retrieved and acted upon.

struct FJobState_Base
{
protected:
	FJobState_Base() = default;
};

/// Default job state for the job state machine (CONCRETE).
/// TRANSITIONS:
/// - add successor: transitions to Stub
/// - on child job tracked: transitions to StageStub
/// - close: transitions to StageStub (may occur if it's an empty stage)
/// - define stage: transitions to DefinedStage
/// - define task: transitions to DefinedTask
struct ZAKAZANEUTILITIES_API FJobState_Default final : FJobState_Base
{
	static constexpr EJobStateId Id = EJobStateId::Default;
};

/// Base for all incomplete job states - "incomplete" means that the job has either not started, or not
/// finished its execution.
struct ZAKAZANEUTILITIES_API FJobState_Incomplete_Base : FJobState_Base
{
	/// Promises that this job will complete, to be fulfilled when task finishes or all stage tasks finish and stage
	/// is closed.
	FJobCompletionPromises JobCompletionPromises;

	FJobState_Incomplete_Base() = default;
	FJobState_Incomplete_Base(FJobState_Incomplete_Base&& Other) = default;
	FJobState_Incomplete_Base& operator=(FJobState_Incomplete_Base&& Other) = default;
};

/// Stub job state (CONCRETE)
/// Represents a job that has been introduced by dependent jobs. This may be a task or a stage.
/// TRANSITIONS:
/// - on child job tracked: transitions to StageStub
/// - close: transitions to StageStub (may occur if it's an empty stage)
/// - define stage: transitions to DefinedStage
/// - define task: transitions to DefinedTask
struct ZAKAZANEUTILITIES_API FJobState_Stub : FJobState_Incomplete_Base
{
	static constexpr EJobStateId Id = EJobStateId::Stub;

	using FJobState_Incomplete_Base::FJobState_Incomplete_Base;
};

/// Base for incomplete stages. See doc of FJobState_Incomplete_Base for a definition of "incomplete".
struct ZAKAZANEUTILITIES_API FJobState_IncompleteStage_Base : FJobState_Incomplete_Base
{
	bool bClosed = false;

	int32 NumTrackedJobs = 0;

	explicit FJobState_IncompleteStage_Base() = default;
	explicit FJobState_IncompleteStage_Base(FJobState_Incomplete_Base&& Other);

	bool IsCompleted() const;
};

/// Base for all pending stage states - "pending" in this case means that the job is not being executed. The concrete
/// state may be a stub or a defined stage that awaits for its predecessors to be completed.
struct ZAKAZANEUTILITIES_API FJobState_PendingStage_Base : FJobState_IncompleteStage_Base
{
	/// Note: internal jobs can have predecessors within the stage: e.g., you can have a "prepare" stage
	/// containing a "load assets" job, which is a predecessor of a "load saved game" job.
	/// The JobExecutionPromises array only contains internal jobs that don't have other predecessors because they
	/// are the jobs that should be launched immediately after the stage is executed.
	TArray<FJobExecutionPromise> JobExecutionPromises;

	explicit FJobState_PendingStage_Base() = default;
	explicit FJobState_PendingStage_Base(FJobState_Incomplete_Base&& Other);

	FJobState_PendingStage_Base(FJobState_PendingStage_Base&&) = default;
	FJobState_PendingStage_Base& operator=(FJobState_PendingStage_Base&&) = default;
	FJobState_PendingStage_Base(const FJobState_PendingStage_Base&) = delete;
	FJobState_PendingStage_Base& operator=(const FJobState_PendingStage_Base&) = delete;
};

/// Stub job state for stages (CONCRETE).
/// Represents a stage that has been introduced by jobs within that stage.
/// TRANSITIONS:
/// - define stage: transitions to DefinedStage
struct ZAKAZANEUTILITIES_API FJobState_StageStub final : FJobState_PendingStage_Base
{
	static constexpr EJobStateId Id = EJobStateId::StageStub;

	using FJobState_PendingStage_Base::FJobState_PendingStage_Base;
};

/// Defined job state for stages (CONCRETE).
/// Represents a stage that has had its predecessors defined. Internal jobs may have been partially or completely added.
/// TRANSITIONS:
/// - execute stage: transitions to ExecutingStage
struct ZAKAZANEUTILITIES_API FJobState_DefinedStage final : FJobState_PendingStage_Base
{
	static constexpr EJobStateId Id = EJobStateId::DefinedStage;

	FJobState_DefinedStage(FJobCompletionPromise StageCompletionPromise);
	FJobState_DefinedStage(FJobState_Incomplete_Base&& Other, FJobCompletionPromise StageCompletionPromise);
	FJobState_DefinedStage(FJobState_PendingStage_Base&& Other, FJobCompletionPromise StageCompletionPromise);
};

/// Defined job for tasks (CONCRETE).
/// Represents a task that has its predecessors, as well as the work to perform, defined.
/// TRANSITIONS:
/// - execute task: transitions to ExecutingTask
struct ZAKAZANEUTILITIES_API FJobState_DefinedTask final : FJobState_Incomplete_Base
{
	static constexpr EJobStateId Id = EJobStateId::DefinedTask;

	FTaskExecutionPromise ExecutionPromise;

	FJobState_DefinedTask(FTaskExecutionPromise InTaskExecutionPromise, FJobCompletionPromise InTaskCompletionPromise);
	FJobState_DefinedTask(
		FJobState_Incomplete_Base&& Other,
		FTaskExecutionPromise InTaskExecutionPromise,
		FJobCompletionPromise InTaskCompletionPromise);
};

/// Executing job state for stages (CONCRETE).
/// Represents a stage which all predecessors have completed execution. Internal jobs may have been partially or
/// completely added.
/// TRANSITIONS:
/// - job completed: if all internal jobs have completed execution and stage is closed, transitions to Completed
/// - closed: if all internal jobs have completed, transitions to Completed
struct ZAKAZANEUTILITIES_API FJobState_ExecutingStage final : FJobState_IncompleteStage_Base
{
	static constexpr EJobStateId Id = EJobStateId::ExecutingStage;

	explicit FJobState_ExecutingStage(FJobState_PendingStage_Base&& Other);
};

/// Executing job state for tasks (CONCRETE).
/// Represents a task which all predecessors have completed execution. The underlying work has begun execution.
/// TRANSITIONS:
/// - job completed - transitions to Completed
struct ZAKAZANEUTILITIES_API FJobState_ExecutingTask final : FJobState_Incomplete_Base
{
	static constexpr EJobStateId Id = EJobStateId::ExecutingTask;

	explicit FJobState_ExecutingTask(FJobState_Incomplete_Base&& Other);
};

/// Completed job state (CONCRETE).
/// Represents a stage or a job that has finished its execution.
struct ZAKAZANEUTILITIES_API FJobState_Completed final : FJobState_Base
{
	static constexpr EJobStateId Id = EJobStateId::Completed;
};

using FJobState = TVariant<
	FJobState_Default,
	FJobState_Stub,
	FJobState_StageStub,
	FJobState_DefinedStage,
	FJobState_DefinedTask,
	FJobState_ExecutingStage,
	FJobState_ExecutingTask,
	FJobState_Completed>;

template <class VariantType>
FJobState MakeJobState(VariantType Variant)
{
	return FJobState{TInPlaceType<VariantType>{}, MoveTemp(Variant)};
}

// JobState event API: each event returns an optional new state the state machine should transition to, as well
// as a result of the event, if applicable.

ZAKAZANEUTILITIES_API TPair<TOptional<FJobState>, FFutureJobCompletion> AddSuccessor(FJobState& JobState);

ZAKAZANEUTILITIES_API TOptional<FJobState> DefineStage(
	FJobState& JobState, FJobCompletionPromise StageCompletionPromise);

ZAKAZANEUTILITIES_API TOptional<FJobState> DefineTask(
	FJobState& JobState, FTaskExecutionPromise TaskExecutionPromise, FJobCompletionPromise TaskCompletionPromise);

ZAKAZANEUTILITIES_API TPair<TOptional<FJobState>, TArray<FJobExecutionPromise>> ExecuteStage(FJobState& JobState);

ZAKAZANEUTILITIES_API TPair<TOptional<FJobState>, FTaskExecutionPromise> ExecuteTask(FJobState& JobState);

ZAKAZANEUTILITIES_API TPair<TOptional<FJobState>, FJobCompletionPromises> OnTaskCompleted(FJobState& JobState);

ZAKAZANEUTILITIES_API TPair<TOptional<FJobState>, TResult<void, FError>> OnChildJobTracked(FJobState& JobState);

ZAKAZANEUTILITIES_API TPair<TOptional<FJobState>, FJobCompletionPromises> OnChildJobCompleted(FJobState& JobState);

ZAKAZANEUTILITIES_API TPair<TOptional<FJobState>, FFutureJobExecution> EnqueueJobExecution(FJobState& JobState);

ZAKAZANEUTILITIES_API TPair<TOptional<FJobState>, TResult<FJobCompletionPromises, FError>> CloseStage(
	FJobState& JobState);

}  // namespace Zkz::ExecutionGraph::Private
