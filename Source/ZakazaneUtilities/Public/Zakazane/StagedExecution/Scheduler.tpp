// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "Scheduler.h"
#include "StageState.h"
#include "Zakazane/ContinueIfMacros.h"
#include "Zakazane/Result.h"
#include "Zakazane/Variant.h"

namespace Zkz::StagedExecution
{

template <class InIdType>
TScheduler<InIdType>::FAddStageResult TScheduler<InIdType>::AddStage(
	const IdType& StageId, TArrayView<const IdType> Prerequisites, FOutputDevice* OutputDevice)
{
	TArray<FFutureStageCompletion> FuturePrerequisiteCompletions;

	const FString StageIdStr = TIdTraits<InIdType>::GetLogString(StageId);

	for (const IdType& PrerequisiteId : Prerequisites)
	{
		const FString PrerequisiteIdStr = TIdTraits<InIdType>::GetLogString(PrerequisiteId);

		TStageState<InIdType>& PrerequisiteState = FindOrAddStage(PrerequisiteId);
		FuturePrerequisiteCompletions.Emplace(StageState::AddFollowUp(PrerequisiteState, OutputDevice, StageIdStr));
	}

	if constexpr (GPerformInspections)
	{
		FAddStageResult InspectionResult = TInspectionData<IdType>::DebugAddStage(StageId, Prerequisites);
		ZKZ_RETURN_IF(InspectionResult.HasError(), InspectionResult);
	}

	return StageState::AddStage(FindOrAddStage(StageId), *this, MoveTemp(FuturePrerequisiteCompletions), OutputDevice);
}

template <class InIdType>
TScheduler<InIdType>::FAddTaskToStageResult TScheduler<InIdType>::AddTaskToStage(
	const IdType& StageId, const IdType& TaskId, FOutputDevice* const OutputDevice)
{
	FScopeLock ScopeLock{&Mutex};
	return StageState::AddTaskToStage(FindOrAddStage(StageId), TaskId, OutputDevice);
}

template <class InIdType>
void TScheduler<InIdType>::SetAllTasksAdded(const IdType& StageId, FOutputDevice* const OutputDevice)
{
	FScopeLock ScopeLock{&Mutex};
	StageState::SetAllTasksAdded(FindOrAddStage(StageId), *this, OutputDevice);
}

template <class InIdType>
TScheduler<InIdType>::FAddTaskResult TScheduler<InIdType>::AddTask(
	const IdType& TaskId, TArrayView<const IdType> Prerequisites, FOutputDevice* OutputDevice)
{
	FScopeLock ScopeLock{&Mutex};

	FAddStageResult AddStageResult = AddStage(TaskId, Prerequisites, OutputDevice);
	ZKZ_RETURN_IF(AddStageResult.HasError(), Err(MoveTemp(AddStageResult).GetError()));

	auto AddTaskToStageResult = AddTaskToStage(TaskId, TaskId, OutputDevice);
	check(AddTaskToStageResult.HasValue());

	SetAllTasksAdded(TaskId, OutputDevice);

	return MoveTemp(AddTaskToStageResult).GetValue();
}

template <class InIdType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TScheduler<InIdType>::GetDebugPrerequisiteIds(const IdType& StageId) const -> TOptional<FDebugPrerequisiteIds>
{
	FScopeLock ScopeLock{&Mutex};
	return TInspectionData<InIdType>::GetDebugPrerequisiteIds(StageId);
}

template <class InIdType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TScheduler<InIdType>::GetDebugWaitingAndExecutionTime_S(const InIdType& Id) const -> FWaitingAndExecutionTime
{
	FScopeLock ScopeLock{&Mutex};
	return TInspectionData<InIdType>::GetDebugWaitingAndExecutionTime_S(Id);
}

template <class InIdType>
void TScheduler<InIdType>::DebugNotifyChange(
	const InIdType& Id, const InspectionData::EChangeState ChangeState, const InspectionData::EChangeType ChangeType)
{
	FScopeLock ScopeLock{&Mutex};
	return TInspectionData<InIdType>::DebugNotifyChange(Id, ChangeState, ChangeType);
}

template <class InIdType>
template <class FunctionType>
auto TScheduler<InIdType>::WithStage(const IdType& StageId, FunctionType&& Func) const
{
	FScopeLock ScopeLock{&Mutex};

	const TUniquePtr<TStageState<IdType>>* const StageState = Stages.Find(StageId);

	ZKZ_RETURN_IF(StageState == nullptr, ::Invoke(Func, TStageState<IdType>{}));

	return ::Invoke(Func, **StageState);
}

template <class InIdType>
template <class FunctionType>
void TScheduler<InIdType>::ForEachStage(FunctionType&& Func) const
{
	FScopeLock ScopeLock{&Mutex};

	for (const TUniquePtr<TStageState<IdType>>& Stage : Stages)
	{
		ZKZ_CONTINUE_IF_ENSUREALWAYS(Stage == nullptr);
		Func(*Stage);
	}
}

template <class InIdType>
template <class TargetStateType, class... ArgTypes>
TargetStateType& TScheduler<InIdType>::Transition(const InIdType& IdType, ArgTypes&&... Args)
{
	return VariantEmplace_GetRef<TargetStateType>(FindOrAddStage(IdType), Forward<ArgTypes>(Args)...);
}

template <class InIdType>
const InIdType& TScheduler<InIdType>::FStagesKeyFuncs::GetSetKey(const TUniquePtr<TStageState<IdType>>& Element)
{
	check(Element != nullptr);
	return StageState::GetStageId(*Element);
}

template <class InIdType>
bool TScheduler<InIdType>::FStagesKeyFuncs::Matches(const IdType& Lhs, const IdType& Rhs)
{
	return Lhs == Rhs;
}

template <class InIdType>
uint32 TScheduler<InIdType>::FStagesKeyFuncs::GetKeyHash(const IdType& Id)
{
	return GetTypeHash(Id);
}

template <class InIdType>
TStageState<InIdType>& TScheduler<InIdType>::FindOrAddStage(const IdType& StageId)
{
	TUniquePtr<TStageState<InIdType>>* StatePtrPtr = Stages.Find(StageId);
	if (!StatePtrPtr)
	{
		const auto StagesKey =
			Stages.Emplace(MakeUnique<TStageState<InIdType>>(TInPlaceType<TStageState_Undefined<InIdType>>{}, StageId));
		return *Stages.Get(StagesKey);
	}
	return **StatePtrPtr;
}

}  // namespace Zkz::StagedExecution
