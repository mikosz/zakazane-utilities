// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ResultTypes.h"

namespace Zkz::StagedExecution
{

namespace InspectionData
{

enum class EChangeType
{
	Started,
	Finished
};

enum class EChangeState
{
	Waiting,
	Execution
};

struct FTimestamps
{
	FDateTime WaitingStartTime;
	FDateTime WaitingEndTime;
	FDateTime ExecutionStartTime;
	FDateTime ExecutionEndTime;
};

template <class InIdType>
using TPrerequisiteIds = TArray<InIdType, TInlineAllocator<8>>;

template <class InIdType>
using TPrerequisitesByStageId = TMap<InIdType, TPrerequisiteIds<InIdType>>;

template <class InIdType>
using TWaitingAndExecutionTime = TPair<TOptional<float>, TOptional<float>>;

template <class InIdType>
using TTimestampsById = TMap<InIdType, FTimestamps>;

}  // namespace InspectionData

// Inspection for staged execution is optional. It contains:
// - verifying that there are no cyclic dependencies between stages
// - reporting waiting and execution times
#ifdef NO_STAGED_EXECUTION_INSPECTION

constexpr bool GPerformInspections = false;

template <class InIdType>
struct TInspectionData
{
	using FPrerequisiteIds = InspectionData::TPrerequisiteIds<InIdType>;
	using FPrerequisitesByStageId = InspectionData::TPrerequisitesByStageId<InIdType>;
	using FWaitingAndExecutionTime = InspectionData::TWaitingAndExecutionTime<InIdType>;
	using FTimestampsById = InspectionData::TTimestampsById<InIdType>;

	TAddStageResult<InIdType> DebugAddStage(InIdType StageId, TArrayView<const InIdType> PrerequisiteIds);
	TOptional<FPrerequisiteIds> GetDebugPrerequisiteIds(const InIdType& StageId) const;
	FWaitingAndExecutionTime GetDebugWaitingAndExecutionTime_S(const InIdType& Id) const;
	void DebugNotifyChange(
		const InIdType& Id, InspectionData::EChangeState ChangeState, InspectionData::EChangeType ChangeType);
};

#else

constexpr bool GPerformInspections = true;

template <class InIdType>
struct TInspectionData
{
	using FPrerequisiteIds = InspectionData::TPrerequisiteIds<InIdType>;
	using FPrerequisitesByStageId = InspectionData::TPrerequisitesByStageId<InIdType>;
	using FWaitingAndExecutionTime = InspectionData::TWaitingAndExecutionTime<InIdType>;
	using FTimestampsById = InspectionData::TTimestampsById<InIdType>;

	FPrerequisitesByStageId PrerequisitesByStageId;
	FTimestampsById TimestampsById;

	TAddStageResult<InIdType> DebugAddStage(InIdType StageId, TArrayView<const InIdType> PrerequisiteIds);
	TOptional<FPrerequisiteIds> GetDebugPrerequisiteIds(const InIdType& StageId) const;
	FWaitingAndExecutionTime GetDebugWaitingAndExecutionTime_S(const InIdType& Id) const;
	void DebugNotifyChange(
		const InIdType& Id, InspectionData::EChangeState ChangeState, InspectionData::EChangeType ChangeType);
};

#endif

// -- template definitions

#ifdef NO_STAGED_EXECUTION_INSPECTION

template <class InIdType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TInspectionData<InIdType>::DebugAddStage(InIdType StageId, TArrayView<const InIdType> PrerequisiteIds)
	-> TAddStageResult<InIdType>
{
	return Ok();
}

template <class InIdType>
auto TInspectionData<InIdType>::GetDebugPrerequisiteIds(const InIdType& StageId) const -> TOptional<FPrerequisiteIds>
{
	return NullOpt;
}

template <class InIdType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TInspectionData<InIdType>::GetDebugWaitingAndExecutionTime_S(const InIdType& Id) const -> FWaitingAndExecutionTime
{
	return {NullOpt, NullOpt};
}

template <class InIdType>
void TInspectionData<InIdType>::DebugNotifyChange(
	const InIdType& Id, const InspectionData::EChangeState ChangeState, const InspectionData::EChangeType ChangeType)
{
}

#else

namespace Private
{

template <class InIdType>
bool CheckDependencyCycle(
	typename TStageCircularDependencyError<InIdType>::FCycle& Cycle,
	const typename TInspectionData<InIdType>::FPrerequisitesByStageId& PrerequisitesByStageId)
{
	ZKZ_RETURN_IF_ENSUREALWAYS(Cycle.IsEmpty(), false);

	const auto* const PrerequisiteIds = PrerequisitesByStageId.Find(Cycle.Last());
	ZKZ_RETURN_IF(PrerequisiteIds == nullptr, false);

	for (const InIdType& PrerequisiteId : *PrerequisiteIds)
	{
		if (Cycle.Contains(PrerequisiteId))
		{
			Cycle.Emplace(PrerequisiteId);
			return true;
		}

		Cycle.Emplace(PrerequisiteId);

		if (CheckDependencyCycle<InIdType>(Cycle, PrerequisitesByStageId))
		{
			return true;
		}

		Cycle.Pop();
	}

	return false;
}

}  // namespace Private

template <class InIdType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TInspectionData<InIdType>::DebugAddStage(InIdType StageId, TArrayView<const InIdType> PrerequisiteIds)
	-> TAddStageResult<InIdType>
{
	ZKZ_RETURN_IF(
		PrerequisitesByStageId.Contains(StageId),
		TAddStageResult<InIdType>{Unexpect, TInPlaceType<TStageAlreadyAddedError<InIdType>>{}, StageId});

	typename TStageCircularDependencyError<InIdType>::FCycle Cycle;

	Cycle.Emplace(StageId);
	for (const InIdType& PrerequisiteId : PrerequisiteIds)
	{
		Cycle.Emplace(PrerequisiteId);

		if (Private::CheckDependencyCycle<InIdType>(Cycle, PrerequisitesByStageId))
		{
			return EmplaceErr<TAddStageError<InIdType>>(
				TInPlaceType<TStageCircularDependencyError<InIdType>>{},
				StageId,
				TArray<InIdType>{PrerequisiteIds},
				Cycle);
		}

		Cycle.Pop();
	}
	Cycle.Pop();

	ensureAlways(Cycle.IsEmpty());

	PrerequisitesByStageId.Emplace(StageId, PrerequisiteIds);

	return Ok();
}

template <class InIdType>
auto TInspectionData<InIdType>::GetDebugPrerequisiteIds(const InIdType& StageId) const -> TOptional<FPrerequisiteIds>
{
	const FPrerequisiteIds* const StagePrerequisites = PrerequisitesByStageId.Find(StageId);
	ZKZ_RETURN_IF(StagePrerequisites == nullptr, NullOpt);

	return *StagePrerequisites;
}

template <class InIdType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TInspectionData<InIdType>::GetDebugWaitingAndExecutionTime_S(const InIdType& Id) const -> FWaitingAndExecutionTime
{
	const InspectionData::FTimestamps* const Timestamps = TimestampsById.Find(Id);
	ZKZ_RETURN_IF(Timestamps == nullptr, {NullOpt, NullOpt});

	const auto GetElapsedTime_S = [](const FDateTime& StartTime, const FDateTime& EndTime) -> TOptional<float>
	{
		ZKZ_RETURN_IF(StartTime.GetTicks() == 0, NullOpt);
		ZKZ_RETURN_IF(EndTime.GetTicks() == 0, (FDateTime::Now() - StartTime).GetTotalSeconds());
		return (EndTime - StartTime).GetTotalSeconds();
	};

	return {
		GetElapsedTime_S(Timestamps->WaitingStartTime, Timestamps->WaitingEndTime),
		GetElapsedTime_S(Timestamps->ExecutionStartTime, Timestamps->ExecutionEndTime)};
}

template <class InIdType>
void TInspectionData<InIdType>::DebugNotifyChange(
	const InIdType& Id, const InspectionData::EChangeState ChangeState, const InspectionData::EChangeType ChangeType)
{
	InspectionData::FTimestamps& Timestamps = TimestampsById.FindOrAdd(Id);

	if (ChangeState == InspectionData::EChangeState::Waiting)
	{
		if (ChangeType == InspectionData::EChangeType::Started)
		{
			Timestamps.WaitingStartTime = FDateTime::Now();
		}
		else if (ChangeType == InspectionData::EChangeType::Finished)
		{
			Timestamps.WaitingEndTime = FDateTime::Now();
			if (Timestamps.WaitingStartTime.GetTicks() == 0)
			{
				Timestamps.WaitingStartTime = Timestamps.WaitingEndTime;
			}
		}
	}
	else if (ChangeState == InspectionData::EChangeState::Execution)
	{
		if (ChangeType == InspectionData::EChangeType::Started)
		{
			Timestamps.ExecutionStartTime = FDateTime::Now();
		}
		else if (ChangeType == InspectionData::EChangeType::Finished)
		{
			Timestamps.ExecutionEndTime = FDateTime::Now();
			if (Timestamps.ExecutionStartTime.GetTicks() == 0)
			{
				Timestamps.ExecutionStartTime = Timestamps.ExecutionEndTime;
			}
		}
	}
}

#endif

}  // namespace Zkz::StagedExecution
