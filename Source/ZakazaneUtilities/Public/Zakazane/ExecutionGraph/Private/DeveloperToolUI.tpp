// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "Zakazane/ExecutionGraph/DeveloperToolUI.h"
#include "Zakazane/ReturnIfMacros.h"
#include "imgui.h"

namespace Zkz::ExecutionGraph
{
namespace SchedulerMonitoringDeveloperToolPrivate
{

namespace Colors
{

constexpr ImVec4 Unknown{1.0f, 0.0f, 0.0f, 1.0f};	 // red
constexpr ImVec4 Pending{1.0f, 0.0f, 1.0f, 1.0f};	 // yellow
constexpr ImVec4 Executing{0.0f, 1.0f, 0.0f, 1.0f};	 // green
constexpr ImVec4 Completed{0.6f, 0.6f, 0.6f, 1.0f};	 // grey

constexpr int32 UnknownValue = 0;
constexpr int32 PendingValue = 1;
constexpr int32 ExecutingValue = 2;
constexpr int32 CompletedValue = 3;

template <CJobIdTraits InJobIdTraitsType>
int32 GetOrderedStateValue(const typename TDebugData<InJobIdTraitsType>::FJobHistoryEntry* JobHistoryEntry)
{
	ZKZ_RETURN_IF_INVALID(JobHistoryEntry, UnknownValue);

	if (JobHistoryEntry->CompletedTime.IsSet())
	{
		return CompletedValue;
	}
	if (JobHistoryEntry->ExecutedTime.IsSet())
	{
		return ExecutingValue;
	}
	if (JobHistoryEntry->EnqueuedTime.IsSet())
	{
		return PendingValue;
	}

	return UnknownValue;
}

inline const ImVec4& GetOrderedStateColor(const int32 OrderedStateValue)
{
	switch (OrderedStateValue)
	{
		case CompletedValue:
			return Completed;
		case ExecutingValue:
			return Executing;
		case PendingValue:
			return Pending;
		case UnknownValue:
			return Unknown;
		default:
			check(false);
			return Unknown;
	}
}

template <CJobIdTraits InJobIdTraitsType>
const ImVec4& GetStateColor(const typename TDebugData<InJobIdTraitsType>::FJobHistoryEntry* const JobHistoryEntry)
{
	return GetOrderedStateColor(GetOrderedStateValue(JobHistoryEntry));
}

}  // namespace Colors

template <CJobIdTraits InJobIdTraitsType>
TPair<FAnsiString, ImVec4> GetPredecessorsString(
	TConstArrayView<typename InJobIdTraitsType::JobIdType> Predecessors, const auto& JobHistoryEntriesById)
{
	using namespace Colors;

	const auto& [NumPredecessors, Color] = [&Predecessors, &JobHistoryEntriesById]() -> TTuple<int32, ImVec4>
	{
		const int32 MinOrderedStateValue = Zkz::MinBy(
											   Predecessors,
											   [&JobHistoryEntriesById](const auto& PredecessorId)
											   {
												   auto* const JobHistoryEntry =
													   JobHistoryEntriesById.Find(PredecessorId);
												   return GetOrderedStateValue<InJobIdTraitsType>(JobHistoryEntry);
											   })
											   .Get(CompletedValue);

		return {Predecessors.Num(), GetOrderedStateColor(MinOrderedStateValue)};
	}();

	return {FAnsiString::Format("[{0} predecessor(s)]", {NumPredecessors}), Color};
}

template <CJobIdTraits InJobIdTraitsType>
[[nodiscard]] bool TickJobHeader(
	const typename InJobIdTraitsType::JobIdType& JobId, const bool bHasChildren, const auto& JobHistoryEntriesById)
{
	const auto* const JobHistoryEntry = JobHistoryEntriesById.Find(JobId);

	const auto [State, Color] = [JobHistoryEntry]() -> TTuple<const char*, const ImVec4&>
	{
		ZKZ_RETURN_IF_INVALID(JobHistoryEntry, {"Unknown", Colors::Unknown});

		if (JobHistoryEntry->CompletedTime.IsSet())
		{
			return {"Completed", Colors::Completed};
		}
		if (JobHistoryEntry->ExecutedTime.IsSet())
		{
			return {"Executing", Colors::Executing};
		}
		if (JobHistoryEntry->EnqueuedTime.IsSet())
		{
			return {"Pending", Colors::Pending};
		}

		return {"Unknown", Colors::Unknown};
	}();

	ImGui::PushStyleColor(ImGuiCol_Text, Color);
	ON_SCOPE_EXIT
	{
		ImGui::PopStyleColor();
	};

	const auto [JobLabel, bIsRoot] = [&JobId]() -> TTuple<FUtf8String, bool>
	{
		static const FUtf8String RootStageLabel{"[ Root ]"};
		ZKZ_RETURN_IF(JobIdUtilities::IsEmpty<InJobIdTraitsType>(JobId), {RootStageLabel, true});

		const auto LeafJobId = JobIdUtilities::GetLeaf<InJobIdTraitsType>(JobId);
		return {FUtf8String{InJobIdTraitsType::ToString(LeafJobId)}, false};
	}();

	const bool bIsExpandable =
		(Pointer::IsValid(JobHistoryEntry) && (!JobHistoryEntry->Predecessors.IsEmpty() || bHasChildren));

	const auto TreeNodeFlags = (bIsExpandable ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf)
							   | (bIsRoot ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None);

	const bool bIsClosed = Pointer::IsValid(JobHistoryEntry) && JobHistoryEntry->bClosed;

	const bool bTreeNodeOpen = ImGui::TreeNodeEx(
		reinterpret_cast<const char*>(*JobLabel),
		TreeNodeFlags,
		"%s [%s]%s",
		*JobLabel,
		State,
		bIsClosed ? " (closed)" : "");

	// If tree node is open we'll show the dependencies within
	ZKZ_RETURN_IF(bTreeNodeOpen, true);

	ZKZ_RETURN_IF_INVALID(JobHistoryEntry, false);

	if (const auto& [PredecessorsString, PredecessorsColor] =
			GetPredecessorsString<InJobIdTraitsType>(JobHistoryEntry->Predecessors, JobHistoryEntriesById);
		!PredecessorsString.IsEmpty())
	{
		ImGui::SameLine();
		ImGui::TextColored(PredecessorsColor, "%s", *PredecessorsString);
	}

	return bTreeNodeOpen;
}

template <CJobIdTraits InJobIdTraitsType>
void TickPredecessorNodes(const typename InJobIdTraitsType::JobIdType& JobId, const auto& JobHistoryEntriesById)
{
	const auto& JobHistoryEntry = JobHistoryEntriesById[JobId];

	if (const auto& [PredecessorsString, PredecessorsColor] =
			GetPredecessorsString<InJobIdTraitsType>(JobHistoryEntry.Predecessors, JobHistoryEntriesById);
		!PredecessorsString.IsEmpty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, PredecessorsColor);

		if (ImGui::TreeNodeEx("Predecessors", ImGuiTreeNodeFlags_None, "%s", *PredecessorsString))
		{
			for (const auto& PredecessorId : JobHistoryEntry.Predecessors)
			{
				if (TickJobHeader<InJobIdTraitsType>(PredecessorId, false, JobHistoryEntriesById))
				{
					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}

		ImGui::PopStyleColor();
	}
}

/// Draws UI for the given entry and all job history entries that are children of this entry. Returns index of the next
/// entry to be rendered (may be past-the-end)
template <CJobIdTraits InJobIdTraitsType>
[[nodiscard]] int32 TickJobHistoryEntry(
	const int32 JobIdIdx,
	const TConstArrayView<typename InJobIdTraitsType::JobIdType> OrderedJobIds,
	const auto& JobHistoryEntriesById,
	const bool bParentOpen = true)
{
	const auto& JobId = OrderedJobIds[JobIdIdx];
	const auto& JobHistoryEntry = JobHistoryEntriesById[JobId];
	bool bNodeOpen = false;

	if (bParentOpen)
	{
		const bool bHasChildren =
			OrderedJobIds.IsValidIndex(JobIdIdx + 1)
			&& JobIdUtilities::IsChild<InJobIdTraitsType>(JobHistoryEntry.JobId, OrderedJobIds[JobIdIdx + 1]);

		bNodeOpen = TickJobHeader<InJobIdTraitsType>(JobHistoryEntry.JobId, bHasChildren, JobHistoryEntriesById);

		if (bNodeOpen && !JobHistoryEntry.Predecessors.IsEmpty())
		{
			TickPredecessorNodes<InJobIdTraitsType>(JobId, JobHistoryEntriesById);
		}
	}

	int32 ChildOrNextIdx = JobIdIdx + 1;
	while (OrderedJobIds.IsValidIndex(ChildOrNextIdx)
		   && JobIdUtilities::IsChild<InJobIdTraitsType>(JobHistoryEntry.JobId, OrderedJobIds[ChildOrNextIdx]))
	{
		ChildOrNextIdx =
			TickJobHistoryEntry<InJobIdTraitsType>(ChildOrNextIdx, OrderedJobIds, JobHistoryEntriesById, bNodeOpen);
	}

	if (bNodeOpen)
	{
		ImGui::TreePop();
	}

	return ChildOrNextIdx;
}

}  // namespace SchedulerMonitoringDeveloperToolPrivate

template <CSchedulerType SchedulerType>
void TDeveloperToolUI<SchedulerType>::Tick(SchedulerType& Scheduler, const float DeltaTime, const bool bShow)
{
	// This could be a free function, but I can see this UI tool keeping state in the future.

	using namespace SchedulerMonitoringDeveloperToolPrivate;

	ZKZ_RETURN_IF(!bShow);

	const auto L = Scheduler.Lock();

	auto* const DebugData = Scheduler.GetDebugData(L);
	if (DebugData == nullptr)
	{
		ImGui::Text("No debug data available");
		return;
	}

	const auto& [JobHistoryEntriesById, OrderedJobIds] = DebugData->ResolveJobHistory();
	const int32 NumJobHistoryEntries = OrderedJobIds.Num();
	int32 Idx = 0;
	while (Idx < NumJobHistoryEntries)
	{
		Idx = TickJobHistoryEntry<SchedulerType::JobIdTraitsType>(Idx, OrderedJobIds, JobHistoryEntriesById);
	}
}

}  // namespace Zkz::ExecutionGraph
