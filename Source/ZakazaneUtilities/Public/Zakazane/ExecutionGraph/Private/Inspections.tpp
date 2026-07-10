// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "Zakazane/Algo.h"
#include "Zakazane/ExecutionGraph/Inspections.h"
#include "Zakazane/ReturnIfMacros.h"
#include "Zakazane/Variant.h"

namespace Zkz::ExecutionGraph
{

template <CJobIdTraits InJobIdTraitsType>
TDebugData<InJobIdTraitsType>::FJobHistoryEntry::FJobHistoryEntry(JobIdType InJobId) : JobId(MoveTemp(InJobId))
{
}

template <CJobIdTraits InJobIdTraitsType>
void TDebugData<InJobIdTraitsType>::OnJobCreated(JobIdReferenceType JobId)
{
	using namespace Private;

	FJobHistoryQueueEntry JobEntry = MakeJobHistoryEntry(JobId);
	JobEntry.Event.template Emplace<FJobEvent_JobCreated>();

	JobHistoryQueue.EmplaceLast(MoveTemp(JobEntry));
	TrimHistoryQueue();
}

template <CJobIdTraits InJobIdTraitsType>
void TDebugData<InJobIdTraitsType>::OnJobEnqueued(
	JobIdReferenceType JobId, JobIdReferenceType Parent, TConstArrayView<JobIdReferenceType> Predecessors)
{
	using namespace Private;

	FJobHistoryQueueEntry JobEntry = MakeJobHistoryEntry(JobId);

	auto& Event = VariantEmplace_GetRef<FJobEvent_JobEnqueued>(JobEntry.Event);
	Algo::Transform(Predecessors, Event.Predecessors, &JobIdTraitsType::FromReference);

	JobHistoryQueue.EmplaceLast(MoveTemp(JobEntry));
	TrimHistoryQueue();
}

template <CJobIdTraits InJobIdTraitsType>
void TDebugData<InJobIdTraitsType>::OnJobExecuted(JobIdReferenceType JobId)
{
	using namespace Private;

	FJobHistoryQueueEntry JobEntry = MakeJobHistoryEntry(JobId);
	JobEntry.Event.template Emplace<FJobEvent_JobExecuted>();

	JobHistoryQueue.EmplaceLast(MoveTemp(JobEntry));
	TrimHistoryQueue();
}

template <CJobIdTraits InJobIdTraitsType>
void TDebugData<InJobIdTraitsType>::OnStageClosed(JobIdReferenceType JobId)
{
	using namespace Private;

	FJobHistoryQueueEntry JobEntry = MakeJobHistoryEntry(JobId);
	JobEntry.Event.template Emplace<FJobEvent_StageClosed>();

	JobHistoryQueue.EmplaceLast(MoveTemp(JobEntry));
	TrimHistoryQueue();
}

template <CJobIdTraits InJobIdTraitsType>
void TDebugData<InJobIdTraitsType>::OnJobCompleted(JobIdReferenceType JobId)
{
	using namespace Private;

	FJobHistoryQueueEntry JobEntry = MakeJobHistoryEntry(JobId);
	JobEntry.Event.template Emplace<FJobEvent_JobCompleted>();

	JobHistoryQueue.EmplaceLast(MoveTemp(JobEntry));
	TrimHistoryQueue();
}

template <CJobIdTraits InJobIdTraitsType>
TOptional<int32> TDebugData<InJobIdTraitsType>::GetJobHistoryQueueLimit() const
{
	return JobHistoryQueueLimit;
}

template <CJobIdTraits InJobIdTraitsType>
void TDebugData<InJobIdTraitsType>::SetJobHistoryQueueLimit(TOptional<int32> InLimit)
{
	JobHistoryQueueLimit = MoveTemp(InLimit);
	TrimHistoryQueue();
}

template <CJobIdTraits InJobIdTraitsType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TDebugData<InJobIdTraitsType>::ResolveJobHistory()
	-> TTuple<const FJobHistoryEntriesById&, TConstArrayView<JobIdType>>
{
	for (FJobHistoryQueueEntry& QueueEntry : JobHistoryQueue)
	{
		auto& Entry = [this, &QueueEntry]() -> FJobHistoryEntry&
		{
			if (auto* const ExistingEntry = JobHistoryEntriesById.Find(QueueEntry.JobId); ExistingEntry != nullptr)
			{
				return *ExistingEntry;
			}

			OrderedJobIds.EmplaceAt(
				Algo::LowerBound(
					OrderedJobIds,
					QueueEntry.JobId,
					[](const auto& Lhs, const auto& Rhs) { return InJobIdTraitsType::Less(Lhs, Rhs); }),
				QueueEntry.JobId);
			return JobHistoryEntriesById.Emplace(QueueEntry.JobId, QueueEntry.JobId);
		}();

		UpdateJobHistoryEntry(Entry, MoveTemp(QueueEntry));
	}

	JobHistoryQueue.Reset();

	return TTuple<const FJobHistoryEntriesById&, TConstArrayView<JobIdType>>{JobHistoryEntriesById, OrderedJobIds};
}

template <CJobIdTraits InJobIdTraitsType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TDebugData<InJobIdTraitsType>::MakeJobHistoryEntry(JobIdReferenceType JobId) -> FJobHistoryQueueEntry
{
	FJobHistoryQueueEntry JobEntry;
	JobEntry.Time = FDateTime::Now();
	JobEntry.JobId = InJobIdTraitsType::FromReference(JobId);
	return JobEntry;
}

template <CJobIdTraits InJobIdTraitsType>
void TDebugData<InJobIdTraitsType>::TrimHistoryQueue()
{
	ZKZ_RETURN_IF(!JobHistoryQueueLimit.IsSet());

	while (JobHistoryQueue.Num() > JobHistoryQueueLimit.GetValue())
	{
		JobHistoryQueue.PopFirst();
	}
}

template <CJobIdTraits InJobIdTraitsType>
void TDebugData<InJobIdTraitsType>::UpdateJobHistoryEntry(
	FJobHistoryEntry& JobHistoryEntry, FJobHistoryQueueEntry QueueEntry)
{
	Visit(
		[&JobHistoryEntry, &QueueEntry]<class EventType>(EventType&& V)
		{ UpdateJobHistoryEntry(JobHistoryEntry, QueueEntry, MoveTemp(V)); },
		MoveTemp(QueueEntry.Event));
}

template <CJobIdTraits InJobIdTraitsType>
void TDebugData<InJobIdTraitsType>::UpdateJobHistoryEntry(
	FJobHistoryEntry& JobHistoryEntry, FJobHistoryQueueEntry QueueEntry, FJobEvent_JobCreated Event)
{
}

template <CJobIdTraits InJobIdTraitsType>
void TDebugData<InJobIdTraitsType>::UpdateJobHistoryEntry(
	FJobHistoryEntry& JobHistoryEntry, FJobHistoryQueueEntry QueueEntry, FJobEvent_JobEnqueued Event)
{
	JobHistoryEntry.Predecessors = MoveTemp(Event.Predecessors);
	JobHistoryEntry.EnqueuedTime = MoveTemp(QueueEntry.Time);
}

template <CJobIdTraits InJobIdTraitsType>
void TDebugData<InJobIdTraitsType>::UpdateJobHistoryEntry(
	FJobHistoryEntry& JobHistoryEntry, FJobHistoryQueueEntry QueueEntry, FJobEvent_JobExecuted Event)
{
	JobHistoryEntry.ExecutedTime = MoveTemp(QueueEntry.Time);
}

template <CJobIdTraits InJobIdTraitsType>
void TDebugData<InJobIdTraitsType>::UpdateJobHistoryEntry(
	FJobHistoryEntry& JobHistoryEntry, FJobHistoryQueueEntry QueueEntry, FJobEvent_StageClosed Event)
{
	JobHistoryEntry.bClosed = true;
}

template <CJobIdTraits InJobIdTraitsType>
void TDebugData<InJobIdTraitsType>::UpdateJobHistoryEntry(
	FJobHistoryEntry& JobHistoryEntry, FJobHistoryQueueEntry QueueEntry, FJobEvent_JobCompleted Event)
{
	JobHistoryEntry.CompletedTime = MoveTemp(QueueEntry.Time);
}

template <CJobIdTraits InJobIdTraitsType>
const TDebugData<InJobIdTraitsType>* TDebugInspections<InJobIdTraitsType>::GetDebugData() const
{
	return &DebugData;
}

template <CJobIdTraits InJobIdTraitsType>
TDebugData<InJobIdTraitsType>* TDebugInspections<InJobIdTraitsType>::GetDebugData()
{
	return &DebugData;
}

template <CJobIdTraits InJobIdTraitsType>
TResult<void, FError> TDebugInspections<InJobIdTraitsType>::TryAddDependency(
	JobIdReferenceType InJobId, TConstArrayView<JobIdReferenceType> InPredecessorIds)
{
	using namespace Private;

	CycleCandidateType CycleCandidate;

	CycleCandidate.Emplace(InJobIdTraitsType::FromReference(InJobId));

	for (JobIdReferenceType PredecessorId : InPredecessorIds)
	{
		CycleCandidate.Emplace(InJobIdTraitsType::FromReference(PredecessorId));

		if (const bool bHasCycle = CheckDependencyCycle(CycleCandidate); bHasCycle)
		{
			return Err(
				MakeError<FCircularDependencyError>(Zkz::Transform(CycleCandidate, &InJobIdTraitsType::ToString)));
		}

		CycleCandidate.Pop();
	}

	CycleCandidate.Pop();

	ensureAlways(CycleCandidate.IsEmpty());

	PredecessorsByJobId
		.Emplace(InJobId, Zkz::TransformTo<PredecessorsType>(InPredecessorIds, &InJobIdTraitsType::FromReference));

	return Ok();
}

template <CJobIdTraits InJobIdTraitsType>
bool TDebugInspections<InJobIdTraitsType>::CheckDependencyCycle(CycleCandidateType& CycleCandidate) const
{
	ZKZ_RETURN_IF_ENSUREALWAYS(CycleCandidate.IsEmpty(), false);

	const auto* const PrerequisiteIds = PredecessorsByJobId.Find(CycleCandidate.Last());
	ZKZ_RETURN_IF(PrerequisiteIds == nullptr, false);

	for (const JobIdType& PrerequisiteId : *PrerequisiteIds)
	{
		if (CycleCandidate.Contains(PrerequisiteId))
		{
			CycleCandidate.Emplace(PrerequisiteId);
			return true;
		}

		CycleCandidate.Emplace(PrerequisiteId);

		if (CheckDependencyCycle(CycleCandidate))
		{
			return true;
		}

		CycleCandidate.Pop();
	}

	return false;
}

}  // namespace Zkz::ExecutionGraph
