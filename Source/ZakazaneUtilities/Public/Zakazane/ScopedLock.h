// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ReturnIfMacros.h"

namespace Zkz
{

/// Copy of UE::TScopeLock, but move constructible.
template <class MutexType>
class TScopedLock
{
public:
	TScopedLock() = default;

	[[nodiscard]] explicit TScopedLock(MutexType& InMutex) : Mutex{&InMutex}
	{
		Mutex->Lock();
	}

	~TScopedLock()
	{
		Unlock();
	}

	TScopedLock(const TScopedLock& Other) = delete;
	const TScopedLock& operator=(const TScopedLock& Other) = delete;

	TScopedLock(TScopedLock&& Other)
	{
		Swap(*this, Other);
	}

	TScopedLock& operator=(TScopedLock&& Other)
	{
		ZKZ_RETURN_IF(this == &Other, *this);
		TScopedLock ConsumedOther = MoveTemp(Other);
		Swap(*this, ConsumedOther);
		return *this;
	}

	explicit operator bool() const
	{
		return Mutex != nullptr;
	}

	void Unlock()
	{
		if (Mutex)
		{
			Mutex->Unlock();
			Mutex = nullptr;
		}
	}

private:
	MutexType* Mutex = nullptr;

	friend void Swap(TScopedLock& Lhs, TScopedLock& Rhs)
	{
		using ::Swap;
		Swap(Lhs.Mutex, Rhs.Mutex);
	}
};

using FScopedLock = TScopedLock<FCriticalSection>;

}  // namespace Zkz
