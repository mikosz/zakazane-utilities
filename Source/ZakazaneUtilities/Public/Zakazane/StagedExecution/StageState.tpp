#pragma once

#include "IdTraits.h"
// ReSharper disable once CppUnusedIncludeDirective : include used
#include "Scheduler.h"
#include "StageState.h"

namespace Zkz::StagedExecution
{

template <class InIdType>
TStageState_Base<InIdType>::TStageState_Base(InIdType InStageId) : StageId{MoveTemp(InStageId)}
{
}

template <class InIdType>
TStageState_Pending<InIdType>::TStageState_Pending(InIdType InStageId) : TStageState_Base<InIdType>{MoveTemp(InStageId)}
{
}

template <class InIdType>
TStageState_Undefined<InIdType>::TStageState_Undefined(InIdType InStageId)
	: TStageState_Pending<InIdType>{MoveTemp(InStageId)}
{
}

template <class InIdType>
TStageState_Defined<InIdType>::TStageState_Defined(
	TStageState_Undefined<InIdType> StageState_Undefined,
	TArray<FFutureStageCompletion> InFuturePrerequisiteCompletions)
	: TStageState_Pending<InIdType>{MoveTemp(StageState_Undefined)}
	, FuturePrerequisiteCompletions{MoveTemp(InFuturePrerequisiteCompletions)}
{
}

template <class InIdType>
TStageState_Executing<InIdType>::TStageState_Executing(
	TStageState_Defined<InIdType> StageState_Defined, TScheduler<InIdType>* const DebugScheduler)
	: TStageState_Base<InIdType>(MoveTemp(StageState_Defined.StageId))
	, bAllTasksCollected{StageState_Defined.bAllTasksCollected}
	, Tasks{ExecuteAllTasks(StageState_Defined.Tasks, DebugScheduler)}
	, StageCompletionPromises(MoveTemp(StageState_Defined.StageCompletionPromises))
{
}

template <class InIdType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TStageState_Executing<InIdType>::ExecuteAllTasks(
	const TArrayView<typename TStageState_Pending<InIdType>::FTaskEntry> PendingTasks,
	TScheduler<InIdType>* const DebugScheduler) -> TArray<FTaskEntry>
{
	TArray<FTaskEntry> Tasks;

	for (typename TStageState_Pending<InIdType>::FTaskEntry& PendingTask : PendingTasks)
	{
		if (DebugScheduler != nullptr)
		{
			DebugScheduler->DebugNotifyChange(
				PendingTask.Id, InspectionData::EChangeState::Execution, InspectionData::EChangeType::Started);
		}

		FTaskCompletionPromise TaskCompletionPromise;
		FFutureTaskCompletion FutureTaskCompletion;

		if (DebugScheduler != nullptr)
		{
			FTaskCompletionPromise PostNotifyCompletionPromise;
			FutureTaskCompletion = PostNotifyCompletionPromise.GetFuture();

			IfNotCanceled(
				TaskCompletionPromise.GetFuture(),
				[TaskId = PendingTask.Id,
				 PostNotifyCompletionPromise = MoveTemp(PostNotifyCompletionPromise),
				 DebugScheduler]() mutable
				{
					PostNotifyCompletionPromise.EmplaceValue();
					DebugScheduler->DebugNotifyChange(
						TaskId, InspectionData::EChangeState::Execution, InspectionData::EChangeType::Finished);
				});
		}
		else
		{
			FutureTaskCompletion = TaskCompletionPromise.GetFuture();
		}

		Tasks.Emplace(PendingTask.Id, MoveTemp(FutureTaskCompletion));
		PendingTask.ExecutionPromise.SetValue(MoveTemp(TaskCompletionPromise));
	}

	return Tasks;
}

template <class InIdType>
TStageState_Completed<InIdType>::TStageState_Completed(InIdType InStageId)
	: TStageState_Base<InIdType>(MoveTemp(InStageId))
{
}

namespace StageState
{

namespace Private
{

template <class InIdType>
void TransitionToStageState_Completed(
	TScheduler<InIdType>& Scheduler,
	TStageState_Executing<InIdType> StageState_Executing,
	FOutputDevice* const OutputDevice)
{
	if (OutputDevice != nullptr)
	{
		OutputDevice->Logf(
			ELogVerbosity::Log,
			TEXT("Stage %s: completed, notifying %d dependent stage(s)"),
			*TIdTraits<InIdType>::GetLogString(StageState_Executing.StageId),
			StageState_Executing.StageCompletionPromises.Num());
	}

	Scheduler.DebugNotifyChange(
		StageState_Executing.StageId, InspectionData::EChangeState::Execution, InspectionData::EChangeType::Finished);

	for (FStageCompletionPromise& StageCompletionPromise : StageState_Executing.StageCompletionPromises)
	{
		StageCompletionPromise.EmplaceValue();
	}

	Scheduler.template Transition<TStageState_Completed<InIdType>>(
		StageState_Executing.StageId, StageState_Executing.StageId);
}

template <class InIdType>
void CompleteWhenTasksFinished(
	TStageState_Executing<InIdType>& StageState_Executing,
	TScheduler<InIdType>& Scheduler,
	FOutputDevice* const OutputDevice)
{
	if (StageState_Executing.Tasks.IsEmpty())
	{
		if (StageState_Executing.bAllTasksCollected)
		{
			TransitionToStageState_Completed(Scheduler, MoveTemp(StageState_Executing), OutputDevice);
		}
		else if (OutputDevice != nullptr)
		{
			OutputDevice->Logf(
				ELogVerbosity::Log,
				TEXT("Stage %s: no more tasks, waiting for all tasks collected"),
				*TIdTraits<InIdType>::GetLogString(StageState_Executing.StageId));
		}
	}
	else
	{
		if (OutputDevice != nullptr)
		{
			OutputDevice->Logf(
				ELogVerbosity::Log,
				TEXT("Stage %s: %d task(s) remaining"),
				*TIdTraits<InIdType>::GetLogString(StageState_Executing.StageId),
				StageState_Executing.Tasks.Num());
		}

		IfNotCanceled(
			StageState_Executing.Tasks.Pop(EAllowShrinking::No).FutureCompletion,
			[&StageState_Executing, &Scheduler, OutputDevice]
			{ CompleteWhenTasksFinished(StageState_Executing, Scheduler, OutputDevice); });
	}
}

template <class InIdType>
void TransitionToStageState_Executing(
	TScheduler<InIdType>& Scheduler,
	TStageState_Defined<InIdType> StageState_Defined,
	FOutputDevice* const OutputDevice)
{
	InIdType StageId = StageState_Defined.StageId;
	TStageState_Executing<InIdType>& StageState_Executing =
		Scheduler.template Transition<TStageState_Executing<InIdType>>(
			MoveTemp(StageId), MoveTemp(StageState_Defined), GPerformInspections ? &Scheduler : nullptr);

	if constexpr (GPerformInspections)
	{
		Scheduler
			.DebugNotifyChange(StageId, InspectionData::EChangeState::Waiting, InspectionData::EChangeType::Finished);
		Scheduler
			.DebugNotifyChange(StageId, InspectionData::EChangeState::Execution, InspectionData::EChangeType::Started);
	}

	if (StageState_Executing.bAllTasksCollected)
	{
		CompleteWhenTasksFinished(StageState_Executing, Scheduler, OutputDevice);
	}
}

template <class InIdType>
void StartExecutingWhenPrerequisitesComplete(
	TStageState_Defined<InIdType>& StageState_Defined,
	TScheduler<InIdType>& Scheduler,
	FOutputDevice* const OutputDevice)
{
	if (StageState_Defined.FuturePrerequisiteCompletions.IsEmpty())
	{
		TransitionToStageState_Executing(Scheduler, MoveTemp(StageState_Defined), OutputDevice);
	}
	else
	{
		IfNotCanceled(
			StageState_Defined.FuturePrerequisiteCompletions.Pop(EAllowShrinking::No),
			[&StageState_Defined, &Scheduler, OutputDevice]
			{ StartExecutingWhenPrerequisitesComplete(StageState_Defined, Scheduler, OutputDevice); });
	}
}

template <class InIdType>
void TransitionToStageState_Defined(
	TScheduler<InIdType>& Scheduler,
	TStageState_Undefined<InIdType> StageState_Undefined,
	TArray<FFutureStageCompletion> FuturePrerequisiteCompletions,
	FOutputDevice* const OutputDevice)
{
	InIdType StageId = StageState_Undefined.StageId;
	TStageState_Defined<InIdType>& StageState_Defined = Scheduler.template Transition<TStageState_Defined<InIdType>>(
		StageId, MoveTemp(StageState_Undefined), MoveTemp(FuturePrerequisiteCompletions));

	Scheduler.DebugNotifyChange(StageId, InspectionData::EChangeState::Waiting, InspectionData::EChangeType::Started);

	StartExecutingWhenPrerequisitesComplete(StageState_Defined, Scheduler, OutputDevice);
}

template <class InIdType>
const InIdType& GetStageId(const FStageState_Unknown& UnknownState)
{
	static InIdType InvalidId;
	// Undefined state, should never be called!
	check(false);

	return InvalidId;
}

template <class InIdType>
const InIdType& GetStageId(const TStageState_Base<InIdType>& State)
{
	return State.StageId;
}

template <class InIdType>
TAddStageResult<InIdType> AddStage(
	FStageState_Unknown& StageState_Unknown,
	TScheduler<InIdType>& Scheduler,
	TArray<FFutureStageCompletion> FuturePrerequisiteCompletions,
	FOutputDevice* const OutputDevice)
{
	// Undefined state, should never be called!
	check(false);
	return {};
}

template <class InIdType>
TAddStageResult<InIdType> AddStage(
	TStageState_Base<InIdType>& StageState_Base,
	TScheduler<InIdType>& Scheduler,
	TArray<FFutureStageCompletion> FuturePrerequisiteCompletions,
	FOutputDevice* const OutputDevice)
{
	if (OutputDevice != nullptr)
	{
		OutputDevice->Logf(
			ELogVerbosity::Warning,
			TEXT("Stage %s: attempted to re-add an already added stage. Ignoring."),
			*TIdTraits<InIdType>::GetLogString(StageState_Base.StageId));
	}

	return Err(TAddStageError<InIdType>{TInPlaceType<TStageAlreadyAddedError<InIdType>>{}, StageState_Base.StageId});
}

template <class InIdType>
TAddStageResult<InIdType> AddStage(
	TStageState_Undefined<InIdType>& StageState_Undefined,
	TScheduler<InIdType>& Scheduler,
	TArray<FFutureStageCompletion> FuturePrerequisiteCompletions,
	FOutputDevice* const OutputDevice)
{
	TransitionToStageState_Defined(
		Scheduler, MoveTemp(StageState_Undefined), MoveTemp(FuturePrerequisiteCompletions), OutputDevice);

	return Ok();
}

inline FFutureStageCompletion AddFollowUp(
	FStageState_Unknown& UnknownState, FOutputDevice* const OutputDevice, FStringView DependentStageName)
{
	// Undefined state, should never be called!
	check(false);
	return {};
}

template <class InIdType>
FFutureStageCompletion AddFollowUp(
	TStageState_Pending<InIdType>& StageState_Pending,
	FOutputDevice* const OutputDevice,
	FStringView DependentStageName)
{
	FStageCompletionPromise& StageCompletionPromise = StageState_Pending.StageCompletionPromises.Emplace_GetRef();

	if (OutputDevice != nullptr)
	{
		OutputDevice->Log(
			ELogVerbosity::Log,
			FString::Format(
				TEXT("Stage {0}: added dependent stage - {1}"),
				{TIdTraits<InIdType>::GetLogString(StageState_Pending.StageId), DependentStageName}));
	}

	return StageCompletionPromise.GetFuture();
}

template <class InIdType>
FFutureStageCompletion AddFollowUp(
	TStageState_Executing<InIdType>& StageState_Executing,
	FOutputDevice* const OutputDevice,
	FStringView DependentStageName)
{
	FStageCompletionPromise& StageCompletionPromise = StageState_Executing.StageCompletionPromises.Emplace_GetRef();

	if (OutputDevice != nullptr)
	{
		OutputDevice->Log(
			ELogVerbosity::Log,
			FString::Format(
				TEXT("Stage {0}: added dependent stage - {1}"),
				{TIdTraits<InIdType>::GetLogString(StageState_Executing.StageId), DependentStageName}));
	}

	return StageCompletionPromise.GetFuture();
}

template <class InIdType>
FFutureStageCompletion AddFollowUp(
	TStageState_Completed<InIdType>& StageState_Complete,
	FOutputDevice* const OutputDevice,
	FStringView DependentStageName)
{
	if (OutputDevice != nullptr)
	{
		OutputDevice->Log(
			ELogVerbosity::Log,
			FString::Format(
				TEXT("Stage {0}: added dependent stage - {1}, stage complete, notifying immediately"),
				{TIdTraits<InIdType>::GetLogString(StageState_Complete.StageId), DependentStageName}));
	}

	FStageCompletionPromise StageCompletionPromise;
	StageCompletionPromise.EmplaceValue();
	return StageCompletionPromise.GetFuture();
}

template <class InIdType>
TResult<FFutureTaskExecution, TAllTasksCollectedError<InIdType>> AddTaskToStage(
	FStageState_Unknown& StageState_Unknown, InIdType TaskId, FOutputDevice* const OutputDevice)
{
	// Undefined state, should never be called!
	check(false);
	return {};
}

template <class InIdType>
TResult<FFutureTaskExecution, TAllTasksCollectedError<InIdType>> AddTaskToStage(
	TStageState_Pending<InIdType>& StageState_Pending, InIdType TaskId, FOutputDevice* const OutputDevice)
{
	ZKZ_RETURN_IF(
		StageState_Pending.bAllTasksCollected,
		Err(TAllTasksCollectedError<InIdType>{StageState_Pending.StageId, MoveTemp(TaskId)}));

	typename TStageState_Pending<InIdType>::FTaskEntry& Task = StageState_Pending.Tasks.Emplace_GetRef();
	Task.Id = MoveTemp(TaskId);

	if (OutputDevice != nullptr)
	{
		OutputDevice->Log(
			ELogVerbosity::Log,
			FString::Format(
				TEXT("Stage {0}: added task - {1}, waiting for prerequisites"),
				{TIdTraits<InIdType>::GetLogString(StageState_Pending.StageId),
				 TIdTraits<InIdType>::GetLogString(Task.Id)}));
	}

	return Ok(Task.ExecutionPromise.GetFuture());
}

template <class InIdType>
TResult<FFutureTaskExecution, TAllTasksCollectedError<InIdType>> AddTaskToStage(
	TStageState_Executing<InIdType>& StageState_Executing, InIdType TaskId, FOutputDevice* const OutputDevice)
{
	ZKZ_RETURN_IF(
		StageState_Executing.bAllTasksCollected,
		Err(TAllTasksCollectedError<InIdType>{StageState_Executing.StageId, MoveTemp(TaskId)}));

	FTaskExecutionPromise TaskExecutionPromise;
	FTaskCompletionPromise TaskCompletionPromise;
	typename TStageState_Executing<InIdType>::FTaskEntry& Task = StageState_Executing.Tasks.Emplace_GetRef();
	Task.Id = MoveTemp(TaskId);
	Task.FutureCompletion = TaskCompletionPromise.GetFuture();

	if (OutputDevice != nullptr)
	{
		OutputDevice->Log(
			ELogVerbosity::Log,
			FString::Format(
				TEXT("Stage {0}: added task - {1}, started execution"),
				{TIdTraits<InIdType>::GetLogString(StageState_Executing.StageId),
				 TIdTraits<InIdType>::GetLogString(Task.Id)}));
	}

	TaskExecutionPromise.SetValue(MoveTemp(TaskCompletionPromise));

	return Ok(TaskExecutionPromise.GetFuture());
}

template <class InIdType>
TResult<FFutureTaskExecution, TAllTasksCollectedError<InIdType>> AddTaskToStage(
	const TStageState_Completed<InIdType>& StageState_Completed, InIdType TaskId, FOutputDevice* const OutputDevice)
{
	if (OutputDevice != nullptr)
	{
		OutputDevice->Log(
			ELogVerbosity::Warning,
			FString::Format(
				TEXT("Stage {0}: attempted to add task - {1} - to a completed stage, ignored"),
				{TIdTraits<InIdType>::GetLogString(StageState_Completed.StageId),
				 TIdTraits<InIdType>::GetLogString(TaskId)}));
	}

	return Err(TAllTasksCollectedError<InIdType>{StageState_Completed.StageId, MoveTemp(TaskId)});
}

template <class InIdType>
void SetAllTasksAdded(
	FStageState_Unknown& StageState_Unknown, TScheduler<InIdType>& Scheduler, FOutputDevice* const OutputDevice)
{
	// Undefined state, should never be called!
	check(false);
}

template <class InIdType>
void SetAllTasksAdded(
	TStageState_Pending<InIdType>& StageState_Pending,
	TScheduler<InIdType>& Scheduler,
	FOutputDevice* const OutputDevice)
{
	ZKZ_RETURN_IF(StageState_Pending.bAllTasksCollected);

	StageState_Pending.bAllTasksCollected = true;

	if (OutputDevice != nullptr)
	{
		OutputDevice->Log(
			ELogVerbosity::Log,
			FString::Format(
				TEXT("Stage {0}: all tasks added, waiting for prerequisites"),
				{TIdTraits<InIdType>::GetLogString(StageState_Pending.StageId)}));
	}
}

template <class InIdType>
void SetAllTasksAdded(
	TStageState_Executing<InIdType>& StageState_Executing,
	TScheduler<InIdType>& Scheduler,
	FOutputDevice* const OutputDevice)
{
	ZKZ_RETURN_IF(StageState_Executing.bAllTasksCollected);

	StageState_Executing.bAllTasksCollected = true;

	if (OutputDevice != nullptr)
	{
		OutputDevice->Log(
			ELogVerbosity::Log,
			FString::Format(
				TEXT("Stage {0}: all tasks added, waiting for task completion"),
				{TIdTraits<InIdType>::GetLogString(StageState_Executing.StageId)}));
	}

	CompleteWhenTasksFinished(StageState_Executing, Scheduler, OutputDevice);
}

template <class InIdType>
void SetAllTasksAdded(
	TStageState_Completed<InIdType>& StageState_Complete,
	TScheduler<InIdType>& Scheduler,
	FOutputDevice* const OutputDevice)
{
}

}  // namespace Private

template <class InIdType>
const InIdType& GetStageId(const TStageState<InIdType>& State)
{
	return Visit([](auto& Variant) -> const InIdType& { return Private::GetStageId<InIdType>(Variant); }, State);
}

template <class InIdType>
TAddStageResult<InIdType> AddStage(
	TStageState<InIdType>& State,
	TScheduler<InIdType>& Scheduler,
	TArray<FFutureStageCompletion> FuturePrerequisiteCompletions,
	FOutputDevice* const OutputDevice)
{
	return Visit(
		[&FuturePrerequisiteCompletions, OutputDevice, &Scheduler](auto& Variant)
		{ return Private::AddStage(Variant, Scheduler, MoveTemp(FuturePrerequisiteCompletions), OutputDevice); },
		State);
}

template <class InIdType>
FFutureStageCompletion AddFollowUp(
	TStageState<InIdType>& State, FOutputDevice* const OutputDevice, const FStringView DependentStageName)
{
	return Visit(
		[OutputDevice, DependentStageName](auto& Variant)
		{ return Private::AddFollowUp(Variant, OutputDevice, DependentStageName); },
		State);
}

template <class InIdType>
TResult<FFutureTaskExecution, TAllTasksCollectedError<InIdType>> AddTaskToStage(
	TStageState<InIdType>& State, InIdType TaskId, FOutputDevice* const OutputDevice)
{
	return Visit(
		[&TaskId, OutputDevice](auto& Variant)
		{ return Private::AddTaskToStage(Variant, MoveTemp(TaskId), OutputDevice); },
		State);
}

template <class InIdType>
void SetAllTasksAdded(TStageState<InIdType>& State, TScheduler<InIdType>& Scheduler, FOutputDevice* const OutputDevice)
{
	Visit(
		[&Scheduler, OutputDevice](auto& Variant)
		{ return Private::SetAllTasksAdded(Variant, Scheduler, OutputDevice); },
		State);
}

template <class InIdType>
EStageStateId GetId(const TStageState<InIdType>& State)
{
	return Visit([]<class StateVariantType>(const StateVariantType& Variant) { return StateVariantType::Id; }, State);
}

}  // namespace StageState

}  // namespace Zkz::StagedExecution