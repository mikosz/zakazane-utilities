// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Containers/Deque.h"

namespace Zkz::Budgeting
{

class ZAKAZANEUTILITIES_API FDebugData
{
public:
	struct FUpdateHistoryEntry
	{
		/// When was the update executed
		FDateTime Time;

		/// How many tasks were executed and finished during the update
		int32 TasksExecuted = 0;

		/// How many tasks were left over after the allotted time ran out
		int32 TasksOutstanding = 0;

		/// Huw much time of the allotted budget was left at the end of the update. If negative, the
		/// budget was exceeded.
		FTimespan TimeLeft;
	};

	void AddUpdateHistoryEntry(FDateTime Time);
	void OnQueueUpdated(int32 TasksExecuted, int32 TasksOutstanding, FTimespan TimeLeft);

	TConstArrayView<FUpdateHistoryEntry> GetUpdateHistory() const;

	void SetUpdateHistoryUnlimited();

	/// If limit set to 0, will not store history. For unlimited history, use SetUpdateHistoryUnlimited()
	void SetUpdateHistoryLimit(int32 InLimit);

private:
	/// Maximum number of frames for which we store the update history.
	TOptional<int32> UpdateHistoryLimit = 1024;

	TDeque<FUpdateHistoryEntry> UpdateHistory;

	void TrimHistory();
};

template <class T>
concept CInspections = requires(T Inspections, const T ConstInspections) {
	{ T::WithInspections } -> std::convertible_to<bool>;
	{ ConstInspections.GetDebugData() } -> std::convertible_to<const FDebugData*>;
	{ Inspections.GetDebugData() } -> std::convertible_to<FDebugData*>;
};

class ZAKAZANEUTILITIES_API FDebugInspections
{
public:
	static constexpr bool WithInspections = true;

	const FDebugData* GetDebugData() const;
	FDebugData* GetDebugData();

private:
	FDebugData DebugData;
};

class ZAKAZANEUTILITIES_API FNullInspections
{
public:
	static constexpr bool WithInspections = false;

	const FDebugData* GetDebugData() const;

	FDebugData* GetDebugData();
};

inline const FDebugData* FNullInspections::GetDebugData() const
{
	return nullptr;
}

inline FDebugData* FNullInspections::GetDebugData()
{
	return nullptr;
}

#if UE_BUILD_SHIPPING || UE_BUILD_TEST || (defined(FORCE_BUDGETING_INSPECTIONS) && !FORCE_BUDGETING_INSPECTIONS)

using FDefaultInspections = FNullInspections;

#else

using FDefaultInspections = FDebugInspections;

#endif

}  // namespace Zkz::Budgeting
