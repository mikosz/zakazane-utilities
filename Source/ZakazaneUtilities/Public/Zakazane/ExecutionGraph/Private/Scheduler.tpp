// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "Algo/AllOf.h"
#include "Algo/Compare.h"
#include "Zakazane/Algo.h"
#include "Zakazane/ExecutionGraph/Scheduler.h"

namespace Zkz::ExecutionGraph
{

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
auto TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::MakeJobIdFromString(
	const FStringView String) -> JobIdType
{
	return JobIdTraitsType::FromString(String);
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::AppendJobId(
	JobIdReferenceType Parent, JobIdReferenceType Child) -> JobIdType
{
	return JobIdTraitsType::Append(Parent, Child);
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::TScheduler(
	const LogCategoryType& InLogCategory)
	: LogCategory{InLogCategory}
{
	auto L = Lock();

	FJobCompletionPromise RootJobCompletionPromise;
	FutureRootJobCompletion = RootJobCompletionPromise.GetFuture();

	FindOrAddJob({}, L).DefineStage(MoveTemp(RootJobCompletionPromise));
	ExecuteStage({}, L);

	if constexpr (WithInspections)
	{
		if (auto* const DebugData = GetDebugData(L); DebugData != nullptr)
		{
			DebugData->OnJobExecuted({});
		}
	}
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
auto TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::Lock() const
	-> ScopedLockType
{
	return SynchronizationTraits::Lock(Mutex);
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
auto TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::MakeUnique(JobIdType JobId)
	-> JobIdType
{
	return MakeUnique(MoveTemp(JobId), Lock());
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::MakeUnique(
	JobIdType JobId, const ScopedLockType& L) -> JobIdType
{
	check(!JobIdUtilities::IsEmpty<JobIdTraitsType>(JobId));
	check(SynchronizationTraits::IsLocked(L));

	JobIdTraitsType::MakeIncrementable(JobId);

	while (Jobs.Contains(JobId))
	{
		JobIdTraitsType::Increment(JobId);
	}

	Jobs.Emplace(JobId);

	return JobId;
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
auto TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::MakeUniqueJobIdFromString(
	FStringView JobId) -> JobIdType
{
	auto L = Lock();
	return MakeUniqueJobIdFromString(MoveTemp(JobId), L);
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
auto TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::MakeUniqueJobIdFromString(
	FStringView JobId, const ScopedLockType& L) -> JobIdType
{
	check(SynchronizationTraits::IsLocked(L));

	return MakeUnique(MakeJobIdFromString(MoveTemp(JobId)), L);
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
bool TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::HasJob(
	JobIdType JobId) const
{
	return HasJob(MoveTemp(JobId), Lock());
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
bool TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::HasJob(
	JobIdType JobId, const ScopedLockType& L) const
{
	check(SynchronizationTraits::IsLocked(L));

	return Jobs.Contains(JobId);
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
FFutureJobCompletion TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::
	WhenCompleted(JobIdReferenceType JobId)
{
	auto L = Lock();
	return WhenCompleted(MoveTemp(JobId), L);
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
FFutureJobCompletion TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::
	WhenCompleted(JobIdReferenceType JobId, const ScopedLockType& L)
{
	return FindOrAddJob(JobId, L).AddSuccessor();
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
TResult<void, FError> TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::
	EnqueueStage(JobIdType JobId, TConstArrayView<JobIdReferenceType> Predecessors)
{
	return EnqueueStage(MoveTemp(JobId), Predecessors, Lock());
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
TResult<void, FError> TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::
	EnqueueStage(JobIdType JobId, TConstArrayView<JobIdReferenceType> Predecessors, const ScopedLockType& L)
{
	check(!JobIdUtilities::IsEmpty<JobIdTraitsType>(JobId));
	check(SynchronizationTraits::IsLocked(L));

	ZKZ_PROPAGATE_IF_ERROR(this->TryAddDependency(JobId, Predecessors));

	const JobIdReferenceType ParentId = JobIdUtilities::GetParent<JobIdTraitsType>(JobId);

	UE_LOG(
		LogCategory,
		Log,
		TEXT("Enqueueing stage %s with predecessor(s): {%s}"),
		*JobIdTraitsType::ToString(JobId),
		*JobIdUtilities::PredecessorsToString<JobIdTraitsType>(Predecessors));

	{
		FJobCompletionPromise StageCompletionPromise;
		FFutureJobCompletion FutureStageCompletion = StageCompletionPromise.GetFuture();

		TResult<void, FError> TrackJobCompletionResult =
			TrackChildJobCompletion(ParentId, MoveTemp(FutureStageCompletion), L);
		ZKZ_PROPAGATE_IF_ERROR(TrackJobCompletionResult);

		FindOrAddJob(JobId, L).DefineStage(MoveTemp(StageCompletionPromise));
	}

	ZKZ_PROPAGATE_IF_ERROR(EnqueueJobCommon(JobId, Predecessors, &TScheduler::ExecuteStage, L));

	return Ok();
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
TResult<FFutureTaskExecution, FError> TScheduler<
	LogCategoryType,
	InJobIdTraitsType,
	SynchronizationTraits,
	InspectionsType>::EnqueueTask(JobIdType JobId, TConstArrayView<JobIdReferenceType> Predecessors)
{
	return EnqueueTask(MoveTemp(JobId), MoveTemp(Predecessors), Lock());
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
TResult<FFutureTaskExecution, FError> TScheduler<
	LogCategoryType,
	InJobIdTraitsType,
	SynchronizationTraits,
	InspectionsType>::
	EnqueueTask(JobIdType JobId, TConstArrayView<JobIdReferenceType> Predecessors, const ScopedLockType& L)
{
	check(!JobIdUtilities::IsEmpty<JobIdTraitsType>(JobId));
	check(SynchronizationTraits::IsLocked(L));

	ZKZ_PROPAGATE_IF_ERROR(this->TryAddDependency(JobId, Predecessors));

	const JobIdReferenceType ParentId = JobIdUtilities::GetParent<JobIdTraitsType>(JobId);

	FTaskExecutionPromise TaskExecutionPromise;
	FFutureTaskExecution FutureTaskExecution = TaskExecutionPromise.GetFuture();

	UE_LOG(
		LogCategory,
		Log,
		TEXT("Enqueueing task %s with predecessor(s): {%s}"),
		*JobIdTraitsType::ToString(JobId),
		*JobIdUtilities::PredecessorsToString<JobIdTraitsType>(Predecessors));

	{
		FJobCompletionPromise TaskCompletionPromise;
		FFutureJobCompletion FutureTaskCompletion = TaskCompletionPromise.GetFuture();

		TResult<void, FError> TrackJobCompletionResult =
			TrackChildJobCompletion(ParentId, MoveTemp(FutureTaskCompletion), L);
		ZKZ_PROPAGATE_IF_ERROR(TrackJobCompletionResult);

		FindOrAddJob(JobId, L).DefineTask(MoveTemp(TaskExecutionPromise), MoveTemp(TaskCompletionPromise));
	}

	ZKZ_PROPAGATE_IF_ERROR(EnqueueJobCommon(JobId, Predecessors, &TScheduler::ExecuteTask, L));

	return Ok(MoveTemp(FutureTaskExecution));
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
TResult<void, FError> TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::
	CloseStage(JobIdReferenceType StageId)
{
	return CloseStage(MoveTemp(StageId), Lock());
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
TResult<void, FError> TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::
	CloseStage(JobIdReferenceType StageId, const ScopedLockType& L)
{
	check(SynchronizationTraits::IsLocked(L));

	UE_LOG(LogCategory, Log, TEXT("Closing stage %s"), *JobIdTraitsType::ToString(StageId));

	if constexpr (WithInspections)
	{
		if (auto* const DebugData = GetDebugData(L); DebugData != nullptr)
		{
			DebugData->OnStageClosed(StageId);
		}
	}

	auto CloseStageResult = FindOrAddJob(StageId, L).CloseStage();

	ZKZ_PROPAGATE_IF_ERROR(CloseStageResult);

	for (auto& JobCompletionPromise : CloseStageResult.GetValue())
	{
		JobCompletionPromise.EmplaceValue();
	}

	return Ok();
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
auto TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::GetDebugData(
	const ScopedLockType& L) const -> const DebugDataType*
{
	return const_cast<TScheduler&>(*this).GetDebugData(L);
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
auto TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::GetDebugData(
	const ScopedLockType& L) -> DebugDataType*
{
	check(SynchronizationTraits::IsLocked(L));

	return InspectionsType<InJobIdTraitsType>::GetDebugData();
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::FindOrAddJob(
	JobIdReferenceType JobId, const ScopedLockType& L) -> FJob&
{
	check(SynchronizationTraits::IsLocked(L));

	FJob* const FoundJob = Jobs.FindByHash(JobIdTraitsType::GetTypeHash(JobId), JobId);
	ZKZ_RETURN_IF(FoundJob != nullptr, *FoundJob);

	return Jobs.Emplace(JobId);
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
TResult<void, FError> TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::
	EnqueueJobCommon(
		JobIdType JobId,
		TConstArrayView<JobIdReferenceType> Predecessors,
		ExecuteJobFuncType ExecuteJobFunc,
		const ScopedLockType& L)
{
	const JobIdReferenceType ParentId = JobIdUtilities::GetParent<JobIdTraitsType>(JobId);

	// Make sure all predecessors belong to the same parent as this job
	ZKZ_RETURN_IF(
		!Algo::AllOf(
			Predecessors,
			[ParentId](const JobIdReferenceType PredecessorId)
			{
				const JobIdReferenceType PredecessorParentId =
					JobIdUtilities::GetParent<JobIdTraitsType>(PredecessorId);
				return Algo::Compare(ParentId, PredecessorParentId);
			}),
		Err(FError{TInPlaceType<FPredecessorsDontHaveSameParent>{}}));

	if constexpr (WithInspections)
	{
		if (auto* const DebugData = GetDebugData(L); DebugData != nullptr)
		{
			DebugData->OnJobEnqueued(JobId, JobIdUtilities::GetParent<JobIdTraitsType>(JobId), Predecessors);
		}
	}

	auto ExecuteJob = [this, JobId, ExecuteJobFunc = MoveTemp(ExecuteJobFunc)]
	{
		UE_LOG(LogCategory, Log, TEXT("Executing job %s"), *JobIdTraitsType::ToString(JobId));

		auto L = Lock();

		if constexpr (WithInspections)
		{
			if (auto* const DebugData = GetDebugData(L); DebugData != nullptr)
			{
				DebugData->OnJobExecuted(JobId);
			}
		}

		::Invoke(ExecuteJobFunc, this, JobId, L);
	};

	// If any predecessors exist, sign up to each of them to wait for their completion and then execute
	if (!Predecessors.IsEmpty())
	{
		auto FuturePredecessorCompletions = Zkz::TransformTo<TArray<FFutureJobCompletion, TInlineAllocator<1>>>(
			Predecessors,
			[this, &L](const JobIdReferenceType& PredecessorId) { return AddSuccessor(PredecessorId, L); });

		// When all predecessors are completed, execute the job
		IfNotCanceled(AggregateFutureResults(MoveTemp(FuturePredecessorCompletions)), ExecuteJob);
	}
	else
	{
		// No predecessors - retrieve future execution from parent stage
		FFutureJobExecution FutureJobExecution =
			EnqueueJobExecution(JobIdUtilities::GetParent<JobIdTraitsType>(JobId), L);
		IfNotCanceled(MoveTemp(FutureJobExecution), ExecuteJob);
	}

	return Ok();
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
FFutureJobCompletion TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::
	AddSuccessor(JobIdReferenceType JobId, const ScopedLockType& L)
{
	check(SynchronizationTraits::IsLocked(L));
	return FindOrAddJob(JobId, L).AddSuccessor();
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
FFutureJobExecution TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::
	EnqueueJobExecution(JobIdReferenceType ParentStageId, const ScopedLockType& L)
{
	check(SynchronizationTraits::IsLocked(L));
	return FindOrAddJob(ParentStageId, L).EnqueueJobExecution();
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
void TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::ExecuteStage(
	JobIdReferenceType JobId)
{
	auto L = Lock();
	return ExecuteStage(MoveTemp(JobId), L);
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
void TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::ExecuteStage(
	JobIdReferenceType JobId, const ScopedLockType& L)
{
	check(SynchronizationTraits::IsLocked(L));

	if constexpr (WithInspections)
	{
		if (auto* const DebugData = GetDebugData(L); DebugData != nullptr)
		{
			IfNotCanceled(
				FindOrAddJob(JobId, L).AddSuccessor(),
				[DebugData, JobId = JobIdType{JobId}] { DebugData->OnJobCompleted(JobId); });
		}
	}

	TArray<FJobExecutionPromise> JobExecutionPromises = FindOrAddJob(JobId, L).ExecuteStage();

	for (FJobExecutionPromise& JobExecutionPromise : JobExecutionPromises)
	{
		JobExecutionPromise.EmplaceValue();
	}
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
void TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::ExecuteTask(
	JobIdReferenceType JobId)
{
	auto L = Lock();
	return ExecuteTask(MoveTemp(JobId), L);
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
void TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::ExecuteTask(
	JobIdReferenceType JobId, const ScopedLockType& L)
{
	check(SynchronizationTraits::IsLocked(L));

	FTaskExecutionPromise TaskExecutionPromise = FindOrAddJob(JobId, L).ExecuteTask();

	FJobCompletionPromise JobCompletionPromise;

	IfNotCanceled(
		JobCompletionPromise.GetFuture(),
		[this, JobId = JobIdType{JobId}]
		{
			auto L = Lock();

			if constexpr (WithInspections)
			{
				if (auto* const DebugData = GetDebugData(L); DebugData != nullptr)
				{
					DebugData->OnJobCompleted(JobId);
				}
			}

			OnTaskCompleted(JobId, L);
		});

	TaskExecutionPromise.EmplaceValue(MoveTemp(JobCompletionPromise));
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
TResult<void, FError> TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::
	TrackChildJobCompletion(JobIdReferenceType ParentJobId, FFutureJobCompletion FutureJobCompletion)
{
	auto L = Lock();
	return TrackChildJobCompletion(ParentJobId, MoveTemp(FutureJobCompletion), L);
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
TResult<void, FError> TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::
	TrackChildJobCompletion(
		JobIdReferenceType ParentJobId, FFutureJobCompletion FutureJobCompletion, const ScopedLockType& L)
{
	check(SynchronizationTraits::IsLocked(L));

	auto Result = FindOrAddJob(ParentJobId, L).OnChildJobTracked();
	ZKZ_PROPAGATE_IF_ERROR(Result);

	IfNotCanceled(
		MoveTemp(FutureJobCompletion),
		[this, ParentJobId = JobIdType{ParentJobId}]
		{
			auto JobCompletionPromises = FindOrAddJob(ParentJobId, Lock()).OnChildJobCompleted();

			for (auto& JobCompletionPromise : JobCompletionPromises)
			{
				JobCompletionPromise.EmplaceValue();
			}
		});

	return Ok();
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
void TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::OnTaskCompleted(
	JobIdReferenceType JobId)
{
	auto L = Lock();
	OnTaskCompleted(MoveTemp(JobId), L);
}

template <
	CLogCategory LogCategoryType,
	CJobIdTraits InJobIdTraitsType = TDefaultSchedulerJobIdTraits<>,
	CSynchronizationTrait SynchronizationTraits,
	template <class>
	class InspectionsType>
	requires CInspections<InspectionsType<InJobIdTraitsType>, InJobIdTraitsType>
void TScheduler<LogCategoryType, InJobIdTraitsType, SynchronizationTraits, InspectionsType>::OnTaskCompleted(
	JobIdReferenceType JobId, const ScopedLockType& L)
{
	check(SynchronizationTraits::IsLocked(L));

	UE_LOG(LogCategory, Log, TEXT("Task %s completed"), *JobIdTraitsType::ToString(JobId));

	auto JobCompletionPromises = FindOrAddJob(JobId, L).OnTaskCompleted();

	for (auto& JobCompletionPromise : JobCompletionPromises)
	{
		JobCompletionPromise.EmplaceValue();
	}
}

}  // namespace Zkz::ExecutionGraph
