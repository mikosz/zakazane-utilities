#include "Zakazane/Budgeting/Inspections.h"

#include "Zakazane/ReturnIfMacros.h"

namespace Zkz::Budgeting
{

void FDebugData::AddUpdateHistoryEntry(FDateTime Time)
{
	ZKZ_RETURN_IF(UpdateHistoryLimit == 0);
	UpdateHistory.EmplaceLast(MoveTemp(Time));
	TrimHistory();
}

void FDebugData::OnQueueUpdated(const int32 TasksExecuted, const int32 TasksOutstanding, const FTimespan TimeLeft)
{
	ZKZ_RETURN_IF(UpdateHistory.IsEmpty());

	auto& CurrentEntry = UpdateHistory.Last();
	CurrentEntry.TasksExecuted = TasksExecuted;
	CurrentEntry.TasksOutstanding = TasksOutstanding;
	CurrentEntry.TimeLeft = TimeLeft;
}

TConstArrayView<FDebugData::FUpdateHistoryEntry> FDebugData::GetUpdateHistory() const
{
	return TConstArrayView<FUpdateHistoryEntry>{&UpdateHistory.First(), UpdateHistory.Num()};
}

void FDebugData::SetUpdateHistoryUnlimited()
{
	UpdateHistoryLimit = NullOpt;
}

void FDebugData::SetUpdateHistoryLimit(int32 InLimit)
{
	UpdateHistoryLimit = MoveTemp(InLimit);
	TrimHistory();
}

void FDebugData::TrimHistory()
{
	ZKZ_RETURN_IF(!UpdateHistoryLimit.IsSet());

	while (UpdateHistory.Num() > UpdateHistoryLimit.GetValue())
	{
		UpdateHistory.PopFirst();
	}
}

const FDebugData* FDebugInspections::GetDebugData() const
{
	return &DebugData;
}

FDebugData* FDebugInspections::GetDebugData()
{
	return &DebugData;
}

}  // namespace Zkz::Budgeting
