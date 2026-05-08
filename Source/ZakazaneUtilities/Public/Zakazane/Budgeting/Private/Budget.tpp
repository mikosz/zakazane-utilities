// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Zakazane/Budgeting/Budget.h"
#include "Zakazane/RAII.h"
#include "Zakazane/ReturnIfMacros.h"

namespace Zkz::Budgeting
{

template <CLogCategory LogCategoryType, CInspections InspectionsType>
TBudget<LogCategoryType, InspectionsType>::TBudget(FTimespan InPerFrameBudget, const LogCategoryType& InLogCategory)
	: PerFrameBudget{MoveTemp(InPerFrameBudget)}, LogCategory{InLogCategory}
{
}

template <CLogCategory LogCategoryType, CInspections InspectionsType>
void TBudget<LogCategoryType, InspectionsType>::Tick()
{
	BudgetLeftThisFrame = PerFrameBudget;
	UpdateQueue(true);
}

template <CLogCategory LogCategoryType, CInspections InspectionsType>
void TBudget<LogCategoryType, InspectionsType>::EnqueueTask(TaskType InTask)
{
	Tasks.EmplaceLast(MoveTemp(InTask));
	UpdateQueue(false);
}

template <CLogCategory LogCategoryType, CInspections InspectionsType>
void TBudget<LogCategoryType, InspectionsType>::SetPerFrameBudget(FTimespan InPerFrameBudget)
{
	PerFrameBudget = MoveTemp(InPerFrameBudget);
}

template <CLogCategory LogCategoryType, CInspections InspectionsType>
const FDebugData* TBudget<LogCategoryType, InspectionsType>::GetDebugData() const
{
	return InspectionsType::GetDebugData();
}

template <CLogCategory LogCategoryType, CInspections InspectionsType>
void TBudget<LogCategoryType, InspectionsType>::UpdateQueue(const bool bNewFrame)
{
	// Budgeting is currently only implemented to be executed in the game thread. If it's ever necessary to
	// create budgeting for other threads, we can store the thread id in the constructor and verify here that
	// we're in that thread (or execute a task on that thread)
	check(IsInGameThread());

	// Note that new tasks may be added as a consequence of the tasks being executed - we mark that update is currently
	// run and don't update if it's already marked as being updated.
	ZKZ_RETURN_IF(bIsUpdating);
	TScopedAssignment OverrideIsUpdating{bIsUpdating, true};

	UE_LOG(
		LogCategory,
		Verbose,
		TEXT("Updating queue with %d tasks and %f ms budget"),
		Tasks.Num(),
		FMath::Max(BudgetLeftThisFrame.GetTotalMilliseconds(), 0.0f));

	const FDateTime UpdateStartTime = WithInspections ? FDateTime::Now() : FDateTime{};
	int32 ExecutedTasks = 0;

	auto DoUpdate = [this, &ExecutedTasks](TDeque<TaskType> TasksToUpdate)
	{
		ZKZ_RETURN_IF(BudgetLeftThisFrame <= FTimespan::Zero(), TasksToUpdate);

		ZKZ_RETURN_IF(TasksToUpdate.IsEmpty(), TasksToUpdate);

		const auto StartTime = FDateTime::Now();

		for (;;)
		{
			auto FirstTask = MoveTemp(TasksToUpdate.First());
			TasksToUpdate.PopFirst();

			FirstTask();

			++ExecutedTasks;

			// Instead of calculating each task running time calculate time passed from start. This will include
			// the overhead of this function.
			const auto Now = FDateTime::Now();

			const auto ExecutionTime = Now - StartTime;

			if (ExecutionTime > BudgetLeftThisFrame || TasksToUpdate.IsEmpty())
			{
				BudgetLeftThisFrame -= ExecutionTime;
				break;
			}
		}

		return TasksToUpdate;
	};

	// Keep executing the tasks for as long as:
	// * a single DoUpdate call returns no remaining tasks (meaning that some allotted time is potentially left)
	// * AND the Tasks queue after the call is not empty (meaning that new tasks were added while executing)
	auto RemainingTasks = DoUpdate(MoveTemp(Tasks));
	while (RemainingTasks.IsEmpty() && !Tasks.IsEmpty())
	{
		RemainingTasks = DoUpdate(MoveTemp(Tasks));
	}

	if (!RemainingTasks.IsEmpty())
	{
		if (Tasks.IsEmpty())
		{
			Tasks = MoveTemp(RemainingTasks);
		}
		else
		{
			auto NewTasks = MoveTemp(Tasks);
			Tasks = MoveTemp(RemainingTasks);
			for (auto& NewTask : NewTasks)
			{
				Tasks.EmplaceLast(MoveTemp(NewTask));
			}
		}
	}

	if (!Tasks.IsEmpty())
	{
		UE_LOG(
			LogCategory,
			Log,
			TEXT("Finished updating queue with %d executed and %d outstanding task(s)"),
			ExecutedTasks,
			Tasks.Num());
	}
	else
	{
		UE_LOG(
			LogCategory,
			Verbose,
			TEXT("Finished updating queue with %d executed tasks and %f ms of allotted time left"),
			ExecutedTasks,
			BudgetLeftThisFrame.GetTotalMilliseconds());
	}

	if constexpr (WithInspections)
	{
		if (FDebugData* const DebugData = InspectionsType::GetDebugData(); DebugData != nullptr)
		{
			if (bNewFrame)
			{
				DebugData->AddUpdateHistoryEntry(UpdateStartTime);
			}

			DebugData->OnQueueUpdated(ExecutedTasks, Tasks.Num(), BudgetLeftThisFrame);
		}
	}
}

}  // namespace Zkz::Budgeting
