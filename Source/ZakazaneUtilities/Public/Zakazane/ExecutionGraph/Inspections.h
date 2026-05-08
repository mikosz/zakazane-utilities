// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Containers/Deque.h"
#include "Error.h"
#include "JobIdTraits.h"
#include "Zakazane/Result.h"

namespace Zkz::ExecutionGraph
{

template <CJobIdTraits InJobIdTraitsType>
class TDebugData
{
public:
	static constexpr int32 DefaultJobHistoryQueueLimit = 1024;

	using JobIdTraitsType = InJobIdTraitsType;
	using JobIdType = JobIdTraitsType::JobIdType;
	using JobIdReferenceType = JobIdTraitsType::JobIdReferenceType;

	struct FJobHistoryEntry
	{
		JobIdType JobId;

		TArray<JobIdType, TInlineAllocator<1>> Predecessors;

		TOptional<FDateTime> EnqueuedTime;
		TOptional<FDateTime> ExecutedTime;
		TOptional<FDateTime> CompletedTime;

		bool bClosed = false;

		explicit FJobHistoryEntry(JobIdType InJobId);
	};

	using FJobHistoryEntriesById = TMap<JobIdType, FJobHistoryEntry>;

	void OnJobEnqueued(
		JobIdReferenceType JobId, JobIdReferenceType Parent, TConstArrayView<JobIdReferenceType> Predecessors);

	void OnJobExecuted(JobIdReferenceType JobId);

	void OnStageClosed(JobIdReferenceType JobId);

	void OnJobCompleted(JobIdReferenceType JobId);

	TOptional<int32> GetJobHistoryQueueLimit() const;

	/// If limit set to 0, will not store history. If set to null opt it will be unlimited.
	void SetJobHistoryQueueLimit(TOptional<int32> InLimit);

	/// Updates the job history based on the current job history queue. Returns the updated job history and
	/// an ordered list of job ids.
	/// The job id list is ordered alphabetically, where all the tasks within a stage are listed immediately
	/// after that stage.
	/// NOTE: This must be executed under a lock to ensure thread safety.
	TTuple<const FJobHistoryEntriesById&, TConstArrayView<JobIdType>> ResolveJobHistory();

private:
	struct FJobEvent_JobEnqueued
	{
		JobIdType Parent;
		TArray<JobIdType, TInlineAllocator<1>> Predecessors;
	};

	struct FJobEvent_JobExecuted
	{
	};

	struct FJobEvent_StageClosed
	{
	};

	struct FJobEvent_JobCompleted
	{
	};

	using FJobEvent =
		TVariant<FJobEvent_JobEnqueued, FJobEvent_JobExecuted, FJobEvent_StageClosed, FJobEvent_JobCompleted>;

	struct FJobHistoryQueueEntry
	{
		FDateTime Time;

		JobIdType JobId;

		FJobEvent Event;
	};

	/// Maximum number of frames for which we store the job history queue.
	TOptional<int32> JobHistoryQueueLimit = DefaultJobHistoryQueueLimit;

	TDeque<FJobHistoryQueueEntry> JobHistoryQueue;

	FJobHistoryEntriesById JobHistoryEntriesById;

	TArray<JobIdType> OrderedJobIds;

	static FJobHistoryQueueEntry MakeJobHistoryEntry(JobIdReferenceType JobId);

	void TrimHistoryQueue();

	static void UpdateJobHistoryEntry(FJobHistoryEntry& JobHistoryEntry, FJobHistoryQueueEntry QueueEntry);
	static void UpdateJobHistoryEntry(
		FJobHistoryEntry& JobHistoryEntry, FJobHistoryQueueEntry QueueEntry, FJobEvent_JobEnqueued Event);
	static void UpdateJobHistoryEntry(
		FJobHistoryEntry& JobHistoryEntry, FJobHistoryQueueEntry QueueEntry, FJobEvent_JobExecuted Event);
	static void UpdateJobHistoryEntry(
		FJobHistoryEntry& JobHistoryEntry, FJobHistoryQueueEntry QueueEntry, FJobEvent_StageClosed Event);
	static void UpdateJobHistoryEntry(
		FJobHistoryEntry& JobHistoryEntry, FJobHistoryQueueEntry QueueEntry, FJobEvent_JobCompleted Event);
};

template <class T, class InJobIdTraitsType>
concept CInspections = requires(T Inspections, const T ConstInspections) {
	CJobIdTraits<InJobIdTraitsType>;

	{ T::WithInspections } -> std::convertible_to<bool>;
	{ ConstInspections.GetDebugData() } -> std::convertible_to<const TDebugData<InJobIdTraitsType>*>;
	{ Inspections.GetDebugData() } -> std::convertible_to<TDebugData<InJobIdTraitsType>*>;
	{
		Inspections.TryAddDependency(
			std::declval<typename InJobIdTraitsType::JobIdReferenceType>(),
			std::declval<TConstArrayView<typename InJobIdTraitsType::JobIdReferenceType>>())
	} -> std::convertible_to<TResult<void, FError>>;
};

template <CJobIdTraits InJobIdTraitsType>
class TDebugInspections
{
public:
	using JobIdReferenceType = InJobIdTraitsType::JobIdReferenceType;

	static constexpr bool WithInspections = true;

	const TDebugData<InJobIdTraitsType>* GetDebugData() const;
	TDebugData<InJobIdTraitsType>* GetDebugData();

	TResult<void, FError> TryAddDependency(
		JobIdReferenceType InJobId, TConstArrayView<JobIdReferenceType> InPredecessorIds);

private:
	using JobIdType = InJobIdTraitsType::JobIdType;
	using PredecessorsType = TArray<JobIdType, TInlineAllocator<1>>;
	using PredecessorsByJobIdType = TMap<JobIdType, PredecessorsType>;
	using CycleCandidateType = TArray<JobIdType, TInlineAllocator<8>>;

	TDebugData<InJobIdTraitsType> DebugData;

	PredecessorsByJobIdType PredecessorsByJobId;

	bool CheckDependencyCycle(CycleCandidateType& CycleCandidate) const;
};

template <CJobIdTraits InJobIdTraitsType>
class TNullInspections
{
public:
	using JobIdReferenceType = InJobIdTraitsType::JobIdReferenceType;

	static constexpr bool WithInspections = false;

	const TDebugData<InJobIdTraitsType>* GetDebugData() const
	{
		return nullptr;
	}

	TDebugData<InJobIdTraitsType>* GetDebugData()
	{
		return nullptr;
	}

	// ReSharper disable once CppMemberFunctionMayBeStatic
	TResult<void, FError> TryAddDependency(
		JobIdReferenceType InJobId, TConstArrayView<JobIdReferenceType> InPredecessorIds)
	{
		return Ok();
	}
};

#if UE_BUILD_SHIPPING || UE_BUILD_TEST || (defined(FORCE_BUDGETING_INSPECTIONS) && !FORCE_BUDGETING_INSPECTIONS)

template <CJobIdTraits InJobIdTraitsType>
using TDefaultInspections = TNullInspections<InJobIdTraitsType>;

#else

template <CJobIdTraits InJobIdTraitsType>
using TDefaultInspections = TDebugInspections<InJobIdTraitsType>;

#endif

}  // namespace Zkz::ExecutionGraph

#include "Private/Inspections.tpp"
