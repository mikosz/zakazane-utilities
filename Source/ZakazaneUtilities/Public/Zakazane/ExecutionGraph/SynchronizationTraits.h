// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "Zakazane/Monostate.h"
#include "Zakazane/ScopedLock.h"

namespace Zkz::ExecutionGraph
{

template <class T>
constexpr bool IsSynchronizationTrait = false;

/// Thread safe synchronization trait adds a critical section and scoped lock handling
struct ZAKAZANEUTILITIES_API FThreadSafe
{
	using MutexType = FCriticalSection;
	using LockType = TScopedLock<MutexType>;

	static LockType Lock(MutexType& Mutex);
	static bool IsLocked(const LockType& Lock);
};

template <>
constexpr bool IsSynchronizationTrait<FThreadSafe> = true;

/// Thread unsafe synchronization trait only checks whenever a lock is requested that it's being executed
/// in the main thread.
struct FThreadUnsafe
{
	using MutexType = FMonostate;
	using LockType = FMonostate;

	static LockType Lock(MutexType& Mutex);

	static bool IsLocked(const LockType& Lock);
};

inline FThreadUnsafe::LockType FThreadUnsafe::Lock(MutexType& Mutex)
{
	checkf(
		IsInGameThread(),
		TEXT("Locking in thread unsafe mode from non-main thread - either perform actions from game thread, or use "
			 "the thread-safe synchronization trait"));

	return {};
}

inline bool FThreadUnsafe::IsLocked(const LockType& Lock)
{
	return true;
}

template <>
constexpr bool IsSynchronizationTrait<FThreadUnsafe> = true;

template <class T>
concept CSynchronizationTrait = IsSynchronizationTrait<T>;

}  // namespace Zkz::ExecutionGraph
