// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Inspection.h"
#include "ResultTypes.h"
#include "StageState.h"

namespace Zkz::StagedExecution
{

/// Allows scheduling tasks within stages. Stages support dependencies. Thread safe.
template <class InIdType>
class TScheduler : TInspectionData<InIdType>
{
public:
	using IdType = InIdType;
	using FAddTaskToStageResult = TAddTaskToStageResult<IdType>;
	using FAddTaskResult = TAddTaskResult<IdType>;
	using FAddStageResult = TAddStageResult<IdType>;
	using FDebugPrerequisiteIds = TInspectionData<InIdType>::FPrerequisiteIds;
	using FWaitingAndExecutionTime = TInspectionData<InIdType>::FWaitingAndExecutionTime;

	TScheduler() = default;

	TScheduler(const TScheduler&) = delete;
	TScheduler& operator=(const TScheduler&) = delete;
	TScheduler(TScheduler&&) = default;
	TScheduler& operator=(TScheduler&&) = default;

	/// Defines an execution stage, dependent on other stages / tasks. Tasks should be added to this stage by
	/// calling AddTaskToStage. SetAllTasksAdded needs to be called at some point, allowing the stage to become
	/// completed when all running tasks finish and enabling to trigger dependant stages. Note that AddStage may be
	/// called after AddTaskToStage or SetAllTasksAdded, i.e. it's fine to add tasks to a stage that hasn't been defined
	/// yet.
	FAddStageResult AddStage(
		const IdType& StageId, TArrayView<const IdType> Prerequisites, FOutputDevice* OutputDevice = nullptr);

	/// Adds a task to the given stage. The stage doesn't have to be added at this point, the only requirement is that
	/// SetAllTasksAdded has not been called for it.
	/// Tasks are defined by a nested future / promise pair. AddTaskToStage returns a future task execution - that is a future
	/// that will yield a promise when that task is executed. The promise is the task completion promise - i.e. the
	/// promise the task must fulfil when the task finishes execution. So the way to implement a task is to call:
	/// <pre>
	///		FAddTaskToStageResult AddTaskResult = Scheduler.AddTaskToStage("spawn actors", "policeman Tom");
	///		if (AddTaskResult.HasError())
	///		{
	///		    // handle error...
	///		}
	///		else
	///		{
	///		    IfNotCanceled(
	///		        MoveTemp(AddTaskResult).GetValue(),
	///		        [](FTaskCompletionPromise CompletionPromise) {
	///		            // perform task actions...
	///
	///		            CompletionPromise.EmplaceValue(); // notify the scheduler task is finished
	///		        });
	///		}
	/// </pre>
	/// @returns a future task execution or error if AllTasksCollected has already been called
	FAddTaskToStageResult AddTaskToStage(
		const IdType& StageId, const IdType& TaskId, FOutputDevice* OutputDevice = nullptr);

	/// Sets the given stage as all tasks added. This stage will not accept any more tasks. When all  tasks finish work,
	/// the stage completion promise will be fulfilled, potentially triggering execution of dependent stages.
	void SetAllTasksAdded(const IdType& StageId, FOutputDevice* OutputDevice = nullptr);

	/// Adds a single task with dependencies.
	/// Under the hood this creates a single-task stage with the same id as the given TaskId.
	FAddTaskResult AddTask(
		const IdType& TaskId, TArrayView<const IdType> Prerequisites, FOutputDevice* OutputDevice = nullptr);

	/// Returns prerequisite ids for a given stage. Prerequisite ids may not be available in your build, in which
	/// case this will return NullOpt.
	TOptional<FDebugPrerequisiteIds> GetDebugPrerequisiteIds(const IdType& StageId) const;

	FWaitingAndExecutionTime GetDebugWaitingAndExecutionTime_S(const InIdType& Id) const;

	/// Internal use only! Used by stage state functions to notify debug instrumentation about changes in states.
	void DebugNotifyChange(
		const InIdType& Id,
		const InspectionData::EChangeState ChangeState,
		const InspectionData::EChangeType ChangeType);

	template <class FunctionType>
	auto WithStage(const IdType& StageId, FunctionType&& Func) const;

	template <class FunctionType>
	void ForEachStage(FunctionType&& Func) const;

	template <class TargetStateType, class... ArgTypes>
	TargetStateType& Transition(const InIdType& IdType, ArgTypes&&... Args);

private:
	struct FStagesKeyFuncs : BaseKeyFuncs<TUniquePtr<TStageState<IdType>>, IdType>
	{
		static const IdType& GetSetKey(const TUniquePtr<TStageState<IdType>>& Element);
		static bool Matches(const IdType& Lhs, const IdType& Rhs);
		static uint32 GetKeyHash(const IdType& Id);
	};

	mutable FCriticalSection Mutex;

	TSet<TUniquePtr<TStageState<IdType>>, FStagesKeyFuncs> Stages;

	TStageState<IdType>& FindOrAddStage(const IdType& StageId);
};

}  // namespace Zkz::StagedExecution

#include "Scheduler.tpp"
