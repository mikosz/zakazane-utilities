// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Error.h"
#include "Private/JobState.h"
#include "ResultTypes.h"

namespace Zkz::ExecutionGraph
{

/// Represents a job in the execution graph.
class ZAKAZANEUTILITIES_API FJob
{
public:
	FJob() = default;
	FJob(FJob&& Other) = default;
	FJob& operator=(FJob&& Other) = default;

	/// Tells a job that it has a successor job that has to be triggered upon completion.
	FFutureJobCompletion AddSuccessor();

	/// Defines a job as a stage with known requirements, ready to start execution when they are fulfilled.
	void DefineStage(FJobCompletionPromise StageCompletionPromise);

	/// Defines a job as a task with known requirements, ready to start execution when they are fulfilled.
	void DefineTask(FTaskExecutionPromise TaskExecutionPromise, FJobCompletionPromise TaskCompletionPromise);

	/// Tells a stage that all its requirements are met and it can start execution of its tasks.
	TArray<FJobExecutionPromise> ExecuteStage();

	/// Tells a task that all its requirements are met and the stage is executing, so it can start execution.
	FTaskExecutionPromise ExecuteTask();

	/// Tells a task that it's now completed.
	FJobCompletionPromises OnTaskCompleted();

	/// Called on a stage to create an execution promise to be fulfilled when it starts execution.
	/// This is only done for jobs that have no predecessors, so execution can start immediately.
	FFutureJobExecution EnqueueJobExecution();

	/// Called on a stage to increment its internal job counter. This way the stage tracks how many jobs need to
	/// complete before the stage is completed.
	TResult<void, FError> OnChildJobTracked();

	/// Called on a stage to decrement its internal job counter. This way the stage tracks how many jobs need to
	/// complete before the stage is completed.
	/// Callee must fulfill all returned promises.
	FJobCompletionPromises OnChildJobCompleted();

	/// Tells a stage that it's now closed and can no longer accept new jobs.
	TResult<FJobCompletionPromises, FError> CloseStage();

	/// @returns A job state id identifying the state the job is in.
	EZkzExecutionGraphJobStateId GetJobStateId() const;

private:
	Private::FJobState ActiveState{TInPlaceType<Private::FJobState_Default>{}};

	void MaybeTransition(TOptional<Private::FJobState> OptNewState);
};

}  // namespace Zkz::ExecutionGraph
