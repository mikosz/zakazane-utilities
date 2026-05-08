// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Containers/Deque.h"
#include "Inspections.h"
#include "Zakazane/Logging.h"

namespace Zkz::Budgeting
{

// #TODO #Budgeting: The current update "heuristic" assumes that if there's any time left this frame,
// another task can be executed. This should potentially be replaced by at least an average execution time
// calculation.

using FBudgetTask = TFunction<void()>;

/// Implements simple budgeting for user tasks. User defines a per-frame amount of time which can be used for
/// task execution.
/// The task queue is updated whenever Tick is called and whenever a new task is added.
template <CLogCategory LogCategoryType, CInspections InspectionsType = FDefaultInspections>
class TBudget final : InspectionsType
{
public:
	using TaskType = FBudgetTask;

	using InspectionsType::WithInspections;

	explicit TBudget(FTimespan InPerFrameBudget, const LogCategoryType& InLogCategory);

	/// Call every frame to reset the per-frame time budget and update the task queue (to run all leftover tasks
	/// from previous frames).
	void Tick();

	// #TODO #Budgeting: consider taking a category argument - then gather stats of avg execution
	// time per category, to estimate if have enough time to complete.

	/// Adds a new task to the queue. If any budget is available for this tick, the task is immediately executed.
	void EnqueueTask(TaskType InTask);

	void SetPerFrameBudget(FTimespan InPerFrameBudget);

	const FDebugData* GetDebugData() const;

private:
	bool bIsUpdating = false;

	FTimespan PerFrameBudget;

	TLogCategoryRef<LogCategoryType> LogCategory;

	FTimespan BudgetLeftThisFrame;

	TDeque<TaskType> Tasks;

	void UpdateQueue(bool bNewFrame);
};

template <class InspectionsType = FDefaultInspections, class LogCategoryType>
auto MakeBudget(FTimespan InPerFrameBudget, const LogCategoryType& InLogCategory UE_LIFETIMEBOUND)
{
	return TBudget<LogCategoryType, InspectionsType>(MoveTemp(InPerFrameBudget), InLogCategory);
}

template <class T>
constexpr bool IsBudgetType = false;

template <class LogCategoryType, class InspectionsType>
constexpr bool IsBudgetType<TBudget<LogCategoryType, InspectionsType>> = true;

template <class T>
concept CBudgetType = IsBudgetType<T>;

}  // namespace Zkz::Budgeting

#include "Private/Budget.tpp"
