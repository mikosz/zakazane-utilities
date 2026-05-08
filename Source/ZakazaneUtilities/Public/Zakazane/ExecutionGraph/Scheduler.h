// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Inspections.h"
#include "Job.h"
#include "JobIdTraits.h"
#include "Scheduler.h"
#include "SynchronizationTraits.h"
#include "Zakazane/Logging.h"
#include "Zakazane/TypeTraits.h"

#include <concepts>

namespace Zkz::ExecutionGraph
{

/// Scheduler for executing tasks within a hierarchical structure, allowing to define dependencies between jobs.
///
/// Two important definitions:
/// - task - this is the leaf node in the job tree, a task executed be user code
/// - stage - this is an internal node in the job tree, a directory for other jobs
///
/// All jobs added to the scheduler are contained without the unnamed "root stage". This stage can be closed
/// the same way as other stages, by providing an empty job id.
///
/// Jobs are identified by an id. E.g. "outer_stage.inner_stage.task" is a task within two named stages (actually three,
/// counting the root stage).
///
/// Order of definitions is not important, so if you first enqueue task "stage.task" and then enqueue
/// the stage "stage", everything will work the same way as if you enqueued "stage" first and then "stage.task".
///
/// Dependencies between jobs are defined by the Predecessors parameter. Note that you can only create
/// dependencies within the same stage, so "stage.task" can have a dependency on "stage.predecessor", but not
/// on "other_stage" or "other_stage.task".
template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits = FThreadUnsafe,
	template <class> class InspectionsType = TDefaultInspections>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
class TScheduler final : InspectionsType<InJobIdTraitsType>
{
public:
	using JobIdTraitsType = InJobIdTraitsType;
	using JobIdType = JobIdTraitsType::JobIdType;
	using JobIdReferenceType = JobIdTraitsType::JobIdReferenceType;
	using ScopedLockType = SynchronizationTraits::LockType;
	using DebugDataType = TDebugData<InJobIdTraitsType>;

	using InspectionsType<InJobIdTraitsType>::WithInspections;

	/// @returns a job id created by parsing the given string. JobIds are hierarchical and parsing is done
	///		by splitting the string by '.'. JobId "grandparent.parent.child" denotes a task "child" in stage
	///		"parent", that itself is in the stage "grandparent".
	static JobIdType MakeJobIdFromString(FStringView String);

	/// @returns a job id created by appending the child job id to the parent job id. E.g., appending "child" to "parent"
	///		yields a job id for the task "child" belonging to the stage "parent".
	static JobIdType AppendJobId(JobIdReferenceType Parent, JobIdReferenceType Child);

	explicit TScheduler(const LogCategoryType& InLogCategory);

	TScheduler(TScheduler&&) = default;
	TScheduler& operator=(TScheduler&&) = default;

	/// Locks the critical section for this scheduler. If scheduler is not thread-safe, checks that execution
	/// is performed under the game thread, and otherwise it is a no-op.
	ScopedLockType Lock() const;

	/// Makes the job id unique, that is, if the job id has already been used in another context, creates a new, unique
	/// job id, and returns it.
	JobIdType MakeUnique(JobIdType JobId);
	JobIdType MakeUnique(JobIdType JobId, const ScopedLockType& L);

	/// Helper function combining MakeUnique and MakeJobIdFromString.
	JobIdType MakeUniqueJobIdFromString(FStringView JobId);
	JobIdType MakeUniqueJobIdFromString(FStringView JobId, const ScopedLockType& L);

	/// @returns Whether a job with the given ID exists. The job might not have been defined, but stubbed because
	///		of being mentioned by another job.
	bool HasJob(JobIdType JobId) const;
	bool HasJob(JobIdType JobId, const ScopedLockType& L) const;

	/// Call the provided function on a job with the given ID. Note that this function asserts that the job by the
	/// given ID exists. If you're not sure whether it exists, first call HasJob.
	template <class FunctionType, class... AdditionalArgTypes>
		requires CInvokable<FunctionType, const FJob&>
	auto WithJob(JobIdType JobId, FunctionType&& Func, AdditionalArgTypes&&... AdditionalArgs) const
	{
		auto L = Lock();
		return WithJob(MoveTemp(JobId), L, Forward<FunctionType>(Func), Forward<AdditionalArgTypes>(AdditionalArgs)...);
	}

	template <class FunctionType, class... AdditionalArgTypes>
		requires CInvokable<FunctionType, const FJob&>
	auto WithJob(
		JobIdType JobId, const ScopedLockType& L, FunctionType&& Func, AdditionalArgTypes&&... AdditionalArgs) const
	{
		check(SynchronizationTraits::IsLocked(L));

		const FJob* const JobPtr = Jobs.Find(JobId);
		check(JobPtr != nullptr);

		return ::Invoke(Func, *JobPtr, Forward<AdditionalArgTypes>(AdditionalArgs)...);
	}

	/// Returns a future that will be completed when the job with the specified id completes execution.
	FFutureJobCompletion WhenCompleted(JobIdReferenceType JobId);
	FFutureJobCompletion WhenCompleted(JobIdReferenceType JobId, const ScopedLockType& L);

	/// Defines an execution stage and its dependencies on other stages / tasks.
	/// CloseStage needs to be called at some point, allowing the stage to become completed when all running tasks
	/// finish and enabling to trigger dependent stages.
	TResult<void, FError> EnqueueStage(JobIdType JobId, TConstArrayView<JobIdReferenceType> Predecessors);
	TResult<void, FError> EnqueueStage(
		JobIdType JobId, TConstArrayView<JobIdReferenceType> Predecessors, const ScopedLockType& L);

	/// Defines a task and its dependencies on other stages / tasks. The stage doesn't have to be defined at this point;
	/// the only requirement is that CloseStage has not been called for it.
	/// @returns FFutureTaskExecution - that is a future that will yield a promise when that task is executed. The
	/// promise is the task completion promise - i.e., the promise the task must fulfil when the task finishes execution.
	/// So the way to implement a task is to call:
	/// <pre>
	///		TResult<FFutureTaskExecution, FError> Result = Scheduler.EnqueueTask(JobId, Predecessors);
	///		if (Result.HasError())
	///		{
	///		    // handle error...
	///		}
	///		else
	///		{
	///		    IfNotCanceled(
	///		        MoveTemp(Result).GetValue(),
	///		        [](FJobCompletionPromise CompletionPromise) {
	///		            // perform task actions...
	///
	///		            CompletionPromise.EmplaceValue(); // notify the scheduler task is finished
	///		        });
	///		}
	/// </pre>
	TResult<FFutureTaskExecution, FError> EnqueueTask(
		JobIdType JobId, TConstArrayView<JobIdReferenceType> Predecessors);
	TResult<FFutureTaskExecution, FError> EnqueueTask(
		JobIdType JobId, TConstArrayView<JobIdReferenceType> Predecessors, const ScopedLockType& L);

	/// Closes a stage, preventing any further tasks from being added to it.
	/// The current implementation prohibits enqueueing jobs even if they have been stubbed prior to closing.
	/// This is explained by a comment in SchedulerTest.cpp marked #TODO #Scheduler.
	TResult<void, FError> CloseStage(JobIdReferenceType StageId);
	TResult<void, FError> CloseStage(JobIdReferenceType StageId, const ScopedLockType& L);

	/// Returns the debug data for this scheduler. May be null if WithInspections is false.
	const DebugDataType* GetDebugData(const ScopedLockType& L) const;
	DebugDataType* GetDebugData(const ScopedLockType& L);

private:
	using MutexType = SynchronizationTraits::MutexType;

	// Not very elegant, but no better ideas
	using ExecuteJobFuncType = void (TScheduler::*)(JobIdReferenceType, const ScopedLockType&);

	mutable MutexType Mutex;

	TLogCategoryRef<LogCategoryType> LogCategory;

	TMap<JobIdType, FJob> Jobs;

	FFutureJobCompletion FutureRootJobCompletion;

	FJob& FindOrAddJob(JobIdReferenceType JobId, const ScopedLockType& L);

	TResult<void, FError> EnqueueJobCommon(
		JobIdType JobId,
		TConstArrayView<JobIdReferenceType> Predecessors,
		ExecuteJobFuncType ExecuteJobFunc,
		const ScopedLockType& L);

	FFutureJobCompletion AddSuccessor(JobIdReferenceType JobId, const ScopedLockType& L);

	FFutureJobExecution EnqueueJobExecution(JobIdReferenceType ParentStageId, const ScopedLockType& L);

	void ExecuteStage(JobIdReferenceType JobId);
	void ExecuteStage(JobIdReferenceType JobId, const ScopedLockType& L);

	void ExecuteTask(JobIdReferenceType JobId);
	void ExecuteTask(JobIdReferenceType JobId, const ScopedLockType& L);

	TResult<void, FError> TrackChildJobCompletion(
		JobIdReferenceType ParentJobId, FFutureJobCompletion FutureJobCompletion);
	TResult<void, FError> TrackChildJobCompletion(
		JobIdReferenceType ParentJobId, FFutureJobCompletion FutureJobCompletion, const ScopedLockType& L);

	void OnTaskCompleted(JobIdReferenceType JobId);
	void OnTaskCompleted(JobIdReferenceType JobId, const ScopedLockType& L);
};

template <class T>
constexpr bool IsSchedulerType = false;

template <class InJobIdTraitsType, class SynchronizationTraits>
constexpr bool IsSchedulerType<TScheduler<InJobIdTraitsType, SynchronizationTraits>> = true;

template <class T>
concept CSchedulerType = IsSchedulerType<T>;

}  // namespace Zkz::ExecutionGraph

#include "Private/Scheduler.tpp"
