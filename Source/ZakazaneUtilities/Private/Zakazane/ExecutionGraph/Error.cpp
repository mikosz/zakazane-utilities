#include "Zakazane/ExecutionGraph/Error.h"

#include "String/Join.h"

namespace Zkz::ExecutionGraph
{

FString FPredecessorsDontHaveSameParent::ToString()
{
	return TEXT(
		"Predecessors don't have the same parent as the dependent job. "
		"E.g. All related jobs must be part of the same stage. E.g. job Loading.SpawnActors.NeilMurray "
		"may list Loading.SpawnActors.Sheriff as a predecessor, but not Loading.Inventory.");
}

FString FAddedJobToClosedStageError::ToString()
{
	return TEXT("Attempted to add a job to a stage that has already been closed.");
}

FString FStageAlreadyClosedError::ToString()
{
	return TEXT("Attempted to close a stage that has already been closed.");
}

FString FInvalidOperationError::ToString()
{
	return TEXT("Internal error: invalid operation. Current state doesn't support this transition.");
}

FCircularDependencyError::FCircularDependencyError(FCycle InCycle)
{
	Cycle = MoveTemp(InCycle);
}

FString FCircularDependencyError::ToString() const
{
	return FString::Format(
		TEXT("Circular dependency detected: {{0}}"), {*WriteToString<128>(UE::String::Join(Cycle, TEXT(" => ")))});
}

FString FJobStateIsNotAllowedAPayload::ToString()
{
	return TEXT(
		"Invalid operation: Get or SetPayload on an invalid job state. "
		"GetPayload is only allowed on incomplete jobs. SetPayload is only allowed on incomplete, "
		"non-executing jobs.");
}

FString FPayloadAlreadySet::ToString()
{
	return TEXT("Payload for job has already been set.");
}

FString ToString(const FError& Error)
{
	return Visit([](const auto& V) { return V.ToString(); }, Error);
}

}  // namespace Zkz::ExecutionGraph
