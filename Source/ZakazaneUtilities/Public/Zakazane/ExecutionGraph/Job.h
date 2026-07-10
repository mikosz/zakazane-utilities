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
	/// @returns objects to be triggered at call site
	TArray<FScopedExecution> ExecuteStage();

	/// Tells a task that all its requirements are met and the stage is executing, so it can start execution.
	FTaskExecutionPromise ExecuteTask();

	/// Tells a task that it's now completed.
	TArray<FScopedExecution> OnTaskCompleted();

	/// Called on a stage to create an execution promise to be fulfilled when it starts execution.
	/// This is only done for jobs that have no predecessors, so execution can start immediately.
	FFutureJobExecution EnqueueJobExecution();

	/// Called on a stage to increment its internal job counter. This way the stage tracks how many jobs need to
	/// complete before the stage is completed.
	TResult<void, FError> OnChildJobTracked();

	/// Called on a stage to decrement its internal job counter. This way the stage tracks how many jobs need to
	/// complete before the stage is completed.
	/// Callee must fulfill all returned promises.
	TArray<FScopedExecution> OnChildJobCompleted();

	/// Tells a stage that it's now closed and can no longer accept new jobs.
	TResult<TArray<FScopedExecution>, FError> CloseStage();

	/// @returns A job state id identifying the state the job is in.
	EZkzExecutionGraphJobStateId GetJobStateId() const;

	/// Sets the payload for the job. Note that completed jobs may not have a payload,
	/// and that if a payload is already set, this function will fail. Setting the
	/// payload on an executing job is also prohibited.
	TResult<void, FError> SetPayload(TUniquePtr<void> InPayload);

	template <class T>
	TResult<T*, FError> GetPayload() const
	{
		auto Payload = GetVoidPayload();
		ZKZ_PROPAGATE_IF_ERROR(Payload);
		return Ok(static_cast<T*>(Payload.GetValue()));
	}

private:
	Private::FJobState ActiveState{TInPlaceType<Private::FJobState_Default>{}};

	void MaybeTransition(TOptional<Private::FJobState> OptNewState);

	TResult<void*, FError> GetVoidPayload() const;
};

}  // namespace Zkz::ExecutionGraph
