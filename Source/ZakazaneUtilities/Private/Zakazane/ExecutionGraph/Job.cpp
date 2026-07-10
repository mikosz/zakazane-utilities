// Copyright ZAKAZANE Studio. All Rights Reserved.

#include "Zakazane/ExecutionGraph/Job.h"

namespace Zkz::ExecutionGraph
{

FFutureJobCompletion FJob::AddSuccessor()
{
	auto [OptNewState, FutureJobCompletion] = Private::AddSuccessor(ActiveState);
	MaybeTransition(MoveTemp(OptNewState));
	return MoveTemp(FutureJobCompletion);
}

void FJob::DefineStage(FJobCompletionPromise StageCompletionPromise)
{
	auto OptNewState = Private::DefineStage(ActiveState, MoveTemp(StageCompletionPromise));
	MaybeTransition(MoveTemp(OptNewState));
}

void FJob::DefineTask(FTaskExecutionPromise TaskExecutionPromise, FJobCompletionPromise TaskCompletionPromise)
{
	auto OptNewState =
		Private::DefineTask(ActiveState, MoveTemp(TaskExecutionPromise), MoveTemp(TaskCompletionPromise));
	MaybeTransition(MoveTemp(OptNewState));
}

TArray<FScopedExecution> FJob::ExecuteStage()
{
	auto [OptNewState, ExecuteAtCallSite] = Private::ExecuteStage(ActiveState);
	MaybeTransition(MoveTemp(OptNewState));
	return MoveTemp(ExecuteAtCallSite);
}

FTaskExecutionPromise FJob::ExecuteTask()
{
	auto [OptNewState, TaskExecutionPromise] = Private::ExecuteTask(ActiveState);
	MaybeTransition(MoveTemp(OptNewState));
	return MoveTemp(TaskExecutionPromise);
}

TArray<FScopedExecution> FJob::OnTaskCompleted()
{
	auto [OptNewState, JobCompletionPromises] = Private::OnTaskCompleted(ActiveState);
	MaybeTransition(MoveTemp(OptNewState));
	return MoveTemp(JobCompletionPromises);
}

FFutureJobExecution FJob::EnqueueJobExecution()
{
	auto [OptNewState, FutureJobExecution] = Private::EnqueueJobExecution(ActiveState);
	MaybeTransition(MoveTemp(OptNewState));
	return MoveTemp(FutureJobExecution);
}

TResult<void, FError> FJob::OnChildJobTracked()
{
	auto [OptNewState, Result] = Private::OnChildJobTracked(ActiveState);
	MaybeTransition(MoveTemp(OptNewState));
	return MoveTemp(Result);
}

TArray<FScopedExecution> FJob::OnChildJobCompleted()
{
	auto [OptNewState, Result] = Private::OnChildJobCompleted(ActiveState);
	MaybeTransition(MoveTemp(OptNewState));
	return MoveTemp(Result);
}

TResult<TArray<FScopedExecution>, FError> FJob::CloseStage()
{
	auto [OptNewState, Result] = Private::CloseStage(ActiveState);
	MaybeTransition(MoveTemp(OptNewState));
	return MoveTemp(Result);
}

EZkzExecutionGraphJobStateId FJob::GetJobStateId() const
{
	return Visit([](const auto& JobState) { return JobState.Id; }, ActiveState);
}

TResult<void, FError> FJob::SetPayload(TUniquePtr<void> InPayload)
{
	auto [OptNewState, Result] = Private::SetPayload(ActiveState, MoveTemp(InPayload));
	MaybeTransition(MoveTemp(OptNewState));
	return MoveTemp(Result);
}

TResult<void*, FError> FJob::GetVoidPayload() const
{
	return Private::GetPayload(ActiveState);
}

void FJob::MaybeTransition(TOptional<Private::FJobState> OptNewState)
{
	if (OptNewState.IsSet())
	{
		ActiveState = MoveTemp(OptNewState.GetValue());
	}
}

}  // namespace Zkz::ExecutionGraph
