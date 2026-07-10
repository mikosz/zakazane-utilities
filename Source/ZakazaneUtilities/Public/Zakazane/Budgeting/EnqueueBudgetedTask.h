// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Zakazane/Budgeting/Budget.h"
#include "Zakazane/ExecutionGraph/Scheduler.h"
#include "Zakazane/Pointer.h"

namespace Zkz::Budgeting
{

/// Enqueues a task in the given scheduler. When the task is executed, it's run under the provided budget.
/// Note that budget is passed as a lifetime tracking pointer, because we need to make sure its
/// lifetime extends to the time of task execution.
template <CLifetimeTrackingPtr BudgetPtrType, ExecutionGraph::CSchedulerType SchedulerType>
	requires(CBudgetType<std::decay_t<decltype(*std::declval<BudgetPtrType>())>>)
TResult<void, ExecutionGraph::FError> EnqueueBudgetedTask(
	SchedulerType& Scheduler,
	typename SchedulerType::JobIdType JobId,
	TConstArrayView<typename SchedulerType::JobIdReferenceType> Predecessors,
	BudgetPtrType BudgetPtr,
	FBudgetTask Task)
{
	auto EnqueueTaskResult = Scheduler.EnqueueTask(MoveTemp(JobId), MoveTemp(Predecessors));
	ZKZ_PROPAGATE_IF_ERROR(EnqueueTaskResult);

	IfNotCanceled(
		MoveTemp(EnqueueTaskResult).GetValue(),
		[BudgetPtr = MoveTemp(BudgetPtr), Task = MoveTemp(Task)](ExecutionGraph::FTaskArgs TaskArgs) mutable
		{
			auto* const BudgetRawPtr = Pointer::Get(BudgetPtr);

			ZKZ_RETURN_IF(BudgetRawPtr == nullptr);

			// TFunction is required to be copyable, so need to make promise shared, unfortunately.
			auto SharedJobCompletionPromise =
				MakeShared<ExecutionGraph::FJobCompletionPromise>(MoveTemp(TaskArgs.CompletionPromise));
			ensureAlwaysMsgf(TaskArgs.Payload == nullptr, TEXT("Budget does not support tasks with payload"));

			BudgetRawPtr->EnqueueTask(
				[SharedJobCompletionPromise = MoveTemp(SharedJobCompletionPromise), Task = MoveTemp(Task)]() mutable
				{
					Task();
					SharedJobCompletionPromise->EmplaceValue();
				});
		});

	return Ok();
}

}  // namespace Zkz::Budgeting
