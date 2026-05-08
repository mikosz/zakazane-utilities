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

TArray<FJobExecutionPromise> FJob::ExecuteStage()
{
	auto [OptNewState, JobExecutionPromises] = Private::ExecuteStage(ActiveState);
	MaybeTransition(MoveTemp(OptNewState));
	return MoveTemp(JobExecutionPromises);
}

FTaskExecutionPromise FJob::ExecuteTask()
{
	auto [OptNewState, TaskExecutionPromise] = Private::ExecuteTask(ActiveState);
	MaybeTransition(MoveTemp(OptNewState));
	return MoveTemp(TaskExecutionPromise);
}

FJobCompletionPromises FJob::OnTaskCompleted()
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

FJobCompletionPromises FJob::OnChildJobCompleted()
{
	auto [OptNewState, Result] = Private::OnChildJobCompleted(ActiveState);
	MaybeTransition(MoveTemp(OptNewState));
	return MoveTemp(Result);
}

TResult<FJobCompletionPromises, FError> FJob::CloseStage()
{
	auto [OptNewState, Result] = Private::CloseStage(ActiveState);
	MaybeTransition(MoveTemp(OptNewState));
	return MoveTemp(Result);
}

EZkzExecutionGraphJobStateId FJob::GetJobStateId() const
{
	return Visit([](const auto& JobState) { return JobState.Id; }, ActiveState);
}

void FJob::MaybeTransition(TOptional<Private::FJobState> OptNewState)
{
	if (OptNewState.IsSet())
	{
		ActiveState = MoveTemp(OptNewState.GetValue());
	}
}

}  // namespace Zkz::ExecutionGraph
