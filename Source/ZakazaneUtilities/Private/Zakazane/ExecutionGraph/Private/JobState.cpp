// Copyright ZAKAZANE Studio. All Rights Reserved.

#include "Zakazane/ExecutionGraph/Private/JobState.h"

namespace Zkz::ExecutionGraph::Private
{
namespace JobStateImpl
{

// -- AddSuccessor

TPair<TOptional<FJobState>, FFutureJobCompletion> AddSuccessor(FJobState_Incomplete_Base& JobState_Incomplete)
{
	FJobCompletionPromise& CompletionPromise = JobState_Incomplete.JobCompletionPromises.Emplace_GetRef();
	return {NullOpt, CompletionPromise.GetFuture()};
}

TPair<TOptional<FJobState>, FFutureJobCompletion> AddSuccessor(FJobState_Default& JobState_Default)
{
	FJobState_Stub JobState_Stub;
	FJobCompletionPromise& CompletionPromise = JobState_Stub.JobCompletionPromises.Emplace_GetRef();

	return {MakeJobState(MoveTemp(JobState_Stub)), CompletionPromise.GetFuture()};
}

TPair<TOptional<FJobState>, FFutureJobCompletion> AddSuccessor(FJobState_Completed& JobState_Completed)
{
	return {NullOpt, MakeImmediateFuture<void>()};
}

// DefineStage

TOptional<FJobState> DefineStage(const FJobState_Base& JobState, FJobCompletionPromise StageCompletionPromise)
{
	checkf(false, TEXT("Invalid state transition"));
	return NullOpt;
}

TOptional<FJobState> DefineStage(FJobState_Default& JobState_Default, FJobCompletionPromise StageCompletionPromise)
{
	FJobState_DefinedStage JobState_DefinedStage{MoveTemp(StageCompletionPromise)};

	return MakeJobState(MoveTemp(JobState_DefinedStage));
}

TOptional<FJobState> DefineStage(
	FJobState_Incomplete_Base& JobState_Incomplete, FJobCompletionPromise StageCompletionPromise)
{
	FJobState_DefinedStage JobState_DefinedStage{MoveTemp(JobState_Incomplete), MoveTemp(StageCompletionPromise)};

	return MakeJobState(MoveTemp(JobState_DefinedStage));
}

TOptional<FJobState> DefineStage(FJobState_StageStub& JobState_StageStub, FJobCompletionPromise StageCompletionPromise)
{
	FJobState_DefinedStage JobState_DefinedStage{MoveTemp(JobState_StageStub), MoveTemp(StageCompletionPromise)};

	return MakeJobState(MoveTemp(JobState_DefinedStage));
}

// DefineTask

TOptional<FJobState> DefineTask(
	const FJobState_Base& JobState,
	FTaskExecutionPromise TaskExecutionPromise,
	FJobCompletionPromise TaskCompletionPromise)
{
	checkf(false, TEXT("Invalid state transition"));
	return NullOpt;
}

TOptional<FJobState> DefineTask(
	FJobState_Default& JobState_Default,
	FTaskExecutionPromise TaskExecutionPromise,
	FJobCompletionPromise TaskCompletionPromise)
{
	FJobState_DefinedTask JobState_DefinedTask{MoveTemp(TaskExecutionPromise), MoveTemp(TaskCompletionPromise)};

	return MakeJobState(MoveTemp(JobState_DefinedTask));
}

TOptional<FJobState> DefineTask(
	FJobState_Stub& JobState_Stub,
	FTaskExecutionPromise TaskExecutionPromise,
	FJobCompletionPromise TaskCompletionPromise)
{
	FJobState_DefinedTask
		JobState_DefinedTask{MoveTemp(JobState_Stub), MoveTemp(TaskExecutionPromise), MoveTemp(TaskCompletionPromise)};

	return MakeJobState(MoveTemp(JobState_DefinedTask));
}

// ExecuteStage

TPair<TOptional<FJobState>, TArray<FJobExecutionPromise>> ExecuteStage(const FJobState_Base& JobState)
{
	checkf(false, TEXT("Invalid state transition"));
	return {NullOpt, TArray<FJobExecutionPromise>{}};
}

TPair<TOptional<FJobState>, TArray<FJobExecutionPromise>> ExecuteStage(FJobState_DefinedStage& JobState_DefinedStage)
{
	TArray<FJobExecutionPromise> JobExecutionPromises = MoveTemp(JobState_DefinedStage.JobExecutionPromises);

	if (!JobState_DefinedStage.IsCompleted())
	{
		FJobState_ExecutingStage JobState_ExecutingStage{MoveTemp(JobState_DefinedStage)};
		return {MakeJobState(MoveTemp(JobState_ExecutingStage)), MoveTemp(JobExecutionPromises)};
	}
	else
	{
		FJobState_Completed JobState_CompletedStage;
		return {MakeJobState(MoveTemp(JobState_CompletedStage)), MoveTemp(JobExecutionPromises)};
	}
}

// ExecuteTask

TPair<TOptional<FJobState>, FTaskExecutionPromise> ExecuteTask(const FJobState_Base& JobState)
{
	checkf(false, TEXT("Invalid state transition"));
	return {NullOpt, FTaskExecutionPromise{}};
}

TPair<TOptional<FJobState>, FTaskExecutionPromise> ExecuteTask(FJobState_DefinedTask& JobState_DefinedTask)
{
	FTaskExecutionPromise TaskExecutionPromise = MoveTemp(JobState_DefinedTask.ExecutionPromise);

	FJobState_ExecutingTask JobState_ExecutingTask{MoveTemp(JobState_DefinedTask)};

	return {MakeJobState(MoveTemp(JobState_ExecutingTask)), MoveTemp(TaskExecutionPromise)};
}

// OnTaskCompleted

TPair<TOptional<FJobState>, FJobCompletionPromises> OnTaskCompleted(const FJobState_Base& JobState)
{
	checkf(false, TEXT("Invalid state transition"));
	return {NullOpt, FJobCompletionPromises{}};
}

TPair<TOptional<FJobState>, FJobCompletionPromises> OnTaskCompleted(FJobState_ExecutingTask& JobState_ExecutingTask)
{
	auto JobCompletionPromises = MoveTemp(JobState_ExecutingTask.JobCompletionPromises);

	FJobState_Completed JobState_Completed;

	return {MakeJobState(MoveTemp(JobState_Completed)), MoveTemp(JobCompletionPromises)};
}

// OnChildJobTracked

TPair<TOptional<FJobState>, TResult<void, FError>> OnChildJobTracked(const FJobState_Base& JobState)
{
	checkf(false, TEXT("Invalid state transition"));
	return {NullOpt, Err(FError{TInPlaceType<FInvalidOperationError>{}})};
}

TPair<TOptional<FJobState>, TResult<void, FError>> OnChildJobTracked(FJobState_Default& JobState_Default)
{
	FJobState_StageStub JobState_StageStub;

	++JobState_StageStub.NumTrackedJobs;

	return {MakeJobState(MoveTemp(JobState_StageStub)), Ok()};
}

TPair<TOptional<FJobState>, TResult<void, FError>> OnChildJobTracked(FJobState_Stub& JobState_Stub)
{
	FJobState_StageStub JobState_StageStub{MoveTemp(JobState_Stub)};

	++JobState_StageStub.NumTrackedJobs;

	return {MakeJobState(MoveTemp(JobState_StageStub)), Ok()};
}

TPair<TOptional<FJobState>, TResult<void, FError>> OnChildJobTracked(FJobState_PendingStage_Base& JobState_PendingStage)
{
	ZKZ_RETURN_IF(JobState_PendingStage.bClosed, {NullOpt, Err(FError{TInPlaceType<FStageAlreadyClosedError>{}})});

	++JobState_PendingStage.NumTrackedJobs;

	return {NullOpt, Ok()};
}

TPair<TOptional<FJobState>, TResult<void, FError>> OnChildJobTracked(FJobState_ExecutingStage& JobState_ExecutingStage)
{
	ZKZ_RETURN_IF(JobState_ExecutingStage.bClosed, {NullOpt, Err(FError{TInPlaceType<FAddedJobToClosedStageError>{}})});

	++JobState_ExecutingStage.NumTrackedJobs;

	return {NullOpt, Ok()};
}

TPair<TOptional<FJobState>, TResult<void, FError>> OnChildJobTracked(FJobState_Completed& JobState_Completed)
{
	return {NullOpt, Err(FError{TInPlaceType<FAddedJobToClosedStageError>{}})};
}

// OnChildJobCompleted

TPair<TOptional<FJobState>, FJobCompletionPromises> OnChildJobCompleted(const FJobState_Base& JobState)
{
	checkf(false, TEXT("Invalid state transition"));
	return {NullOpt, FJobCompletionPromises{}};
}

TPair<TOptional<FJobState>, FJobCompletionPromises> OnChildJobCompleted(
	FJobState_ExecutingStage& JobState_ExecutingStage)
{
	checkf(JobState_ExecutingStage.NumTrackedJobs > 0, TEXT("Internal error"));
	--JobState_ExecutingStage.NumTrackedJobs;

	ZKZ_RETURN_IF(!JobState_ExecutingStage.IsCompleted(), {NullOpt, FJobCompletionPromises{}});

	auto JobCompletionPromises = MoveTemp(JobState_ExecutingStage.JobCompletionPromises);

	FJobState_Completed JobState_Completed;

	return {MakeJobState(MoveTemp(JobState_Completed)), MoveTemp(JobCompletionPromises)};
}

// EnqueueJobExecution
//
// This function is always executed after OnChildJobTracked, so the state should be at least a stage stub.
// It also can't be a closed stage, since OnChildJobTracked would have returned an error.

TPair<TOptional<FJobState>, FFutureJobExecution> EnqueueJobExecution(const FJobState_Base& JobState)
{
	checkf(false, TEXT("Invalid state transition"));
	return {NullOpt, FFutureJobExecution{}};
}

TPair<TOptional<FJobState>, FFutureJobExecution> EnqueueJobExecution(FJobState_PendingStage_Base& JobState_PendingStage)
{
	checkf(!JobState_PendingStage.bClosed, TEXT("Stage is already closed"));
	FFutureJobExecution FutureJobExecution = JobState_PendingStage.JobExecutionPromises.Emplace_GetRef().GetFuture();
	return {NullOpt, MoveTemp(FutureJobExecution)};
}

TPair<TOptional<FJobState>, FFutureJobExecution> EnqueueJobExecution(
	const FJobState_ExecutingStage& JobState_ExecutingStage)
{
	checkf(!JobState_ExecutingStage.bClosed, TEXT("Stage is already closed"));
	return {NullOpt, MakeImmediateFuture<void>()};
}

// CloseStage

TPair<TOptional<FJobState>, TResult<FJobCompletionPromises, FError>> CloseStage(const FJobState_Base& JobState)
{
	checkf(false, TEXT("Invalid state transition"));
	return {NullOpt, Err(FError{TInPlaceType<FInvalidOperationError>{}})};
}

TPair<TOptional<FJobState>, TResult<FJobCompletionPromises, FError>> CloseStage(
	FJobState_PendingStage_Base& JobState_PendingStage)
{
	ZKZ_RETURN_IF(JobState_PendingStage.bClosed, {NullOpt, Err(FError{TInPlaceType<FStageAlreadyClosedError>{}})});

	JobState_PendingStage.bClosed = true;
	return {NullOpt, Ok(FJobCompletionPromises{})};
}

TPair<TOptional<FJobState>, TResult<FJobCompletionPromises, FError>> CloseStage(
	FJobState_ExecutingStage& JobState_ExecutingStage)
{
	ZKZ_RETURN_IF(JobState_ExecutingStage.bClosed, {NullOpt, Err(FError{TInPlaceType<FStageAlreadyClosedError>{}})});

	JobState_ExecutingStage.bClosed = true;

	if (JobState_ExecutingStage.NumTrackedJobs == 0)
	{
		auto JobCompletionPromises = MoveTemp(JobState_ExecutingStage.JobCompletionPromises);

		FJobState_Completed JobState_Completed;
		return {MakeJobState(MoveTemp(JobState_Completed)), Ok(MoveTemp(JobCompletionPromises))};
	}

	return {NullOpt, Ok(FJobCompletionPromises{})};
}

TPair<TOptional<FJobState>, TResult<FJobCompletionPromises, FError>> CloseStage(FJobState_Completed& JobState_Completed)
{
	return {NullOpt, Err(FError{TInPlaceType<FStageAlreadyClosedError>{}})};
}

}  // namespace JobStateImpl

FJobState_IncompleteStage_Base::FJobState_IncompleteStage_Base(FJobState_Incomplete_Base&& Other)
	: FJobState_Incomplete_Base{MoveTemp(Other)}
{
}

bool FJobState_IncompleteStage_Base::IsCompleted() const
{
	return NumTrackedJobs == 0 && bClosed;
}

FJobState_PendingStage_Base::FJobState_PendingStage_Base(FJobState_Incomplete_Base&& Other)
	: FJobState_IncompleteStage_Base{MoveTemp(Other)}
{
}

FJobState_DefinedStage::FJobState_DefinedStage(FJobCompletionPromise StageCompletionPromise)
{
	this->JobCompletionPromises.Emplace(MoveTemp(StageCompletionPromise));
}

FJobState_DefinedStage::FJobState_DefinedStage(
	FJobState_Incomplete_Base&& Other, FJobCompletionPromise StageCompletionPromise)
	: FJobState_PendingStage_Base(MoveTemp(Other))
{
	this->JobCompletionPromises.Emplace(MoveTemp(StageCompletionPromise));
}

FJobState_DefinedStage::FJobState_DefinedStage(
	FJobState_PendingStage_Base&& Other, FJobCompletionPromise StageCompletionPromise)
	: FJobState_PendingStage_Base(MoveTemp(Other))
{
	this->JobCompletionPromises.Emplace(MoveTemp(StageCompletionPromise));
}

FJobState_DefinedTask::FJobState_DefinedTask(
	FTaskExecutionPromise InTaskExecutionPromise, FJobCompletionPromise InTaskCompletionPromise)
	: ExecutionPromise{MoveTemp(InTaskExecutionPromise)}
{
	this->JobCompletionPromises.Emplace(MoveTemp(InTaskCompletionPromise));
}

FJobState_DefinedTask::FJobState_DefinedTask(
	FJobState_Incomplete_Base&& Other,
	FTaskExecutionPromise InTaskExecutionPromise,
	FJobCompletionPromise InTaskCompletionPromise)
	: FJobState_Incomplete_Base(MoveTemp(Other)), ExecutionPromise{MoveTemp(InTaskExecutionPromise)}
{
	this->JobCompletionPromises.Emplace(MoveTemp(InTaskCompletionPromise));
}

FJobState_ExecutingStage::FJobState_ExecutingStage(FJobState_PendingStage_Base&& Other)
	: FJobState_IncompleteStage_Base(MoveTemp(Other))
{
}

FJobState_ExecutingTask::FJobState_ExecutingTask(FJobState_Incomplete_Base&& Other)
	: FJobState_Incomplete_Base(MoveTemp(Other))
{
}

TPair<TOptional<FJobState>, FFutureJobCompletion> AddSuccessor(FJobState& JobState)
{
	return Visit([](auto& V) { return JobStateImpl::AddSuccessor(V); }, JobState);
}

TOptional<FJobState> DefineStage(FJobState& JobState, FJobCompletionPromise StageCompletionPromise)
{
	return Visit(
		[&](auto& V) mutable { return JobStateImpl::DefineStage(V, MoveTemp(StageCompletionPromise)); }, JobState);
}

TOptional<FJobState> DefineTask(
	FJobState& JobState, FTaskExecutionPromise TaskExecutionPromise, FJobCompletionPromise TaskCompletionPromise)
{
	return Visit(
		[&](auto& V) mutable
		{ return JobStateImpl::DefineTask(V, MoveTemp(TaskExecutionPromise), MoveTemp(TaskCompletionPromise)); },
		JobState);
}

TPair<TOptional<FJobState>, TArray<FJobExecutionPromise>> ExecuteStage(FJobState& JobState)
{
	return Visit([&](auto& V) mutable { return JobStateImpl::ExecuteStage(V); }, JobState);
}

TPair<TOptional<FJobState>, FTaskExecutionPromise> ExecuteTask(FJobState& JobState)
{
	return Visit([&](auto& V) mutable { return JobStateImpl::ExecuteTask(V); }, JobState);
}

TPair<TOptional<FJobState>, FJobCompletionPromises> OnTaskCompleted(FJobState& JobState)
{
	return Visit([&](auto& V) mutable { return JobStateImpl::OnTaskCompleted(V); }, JobState);
}

TPair<TOptional<FJobState>, TResult<void, FError>> OnChildJobTracked(FJobState& JobState)
{
	return Visit([&](auto& V) mutable { return JobStateImpl::OnChildJobTracked(V); }, JobState);
}

TPair<TOptional<FJobState>, FJobCompletionPromises> OnChildJobCompleted(FJobState& JobState)
{
	return Visit([&](auto& V) mutable { return JobStateImpl::OnChildJobCompleted(V); }, JobState);
}

TPair<TOptional<FJobState>, FFutureJobExecution> EnqueueJobExecution(FJobState& JobState)
{
	return Visit([&](auto& V) mutable { return JobStateImpl::EnqueueJobExecution(V); }, JobState);
}

TPair<TOptional<FJobState>, TResult<FJobCompletionPromises, FError>> CloseStage(FJobState& JobState)
{
	return Visit([](auto& V) { return JobStateImpl::CloseStage(V); }, JobState);
}

}  // namespace Zkz::ExecutionGraph::Private
