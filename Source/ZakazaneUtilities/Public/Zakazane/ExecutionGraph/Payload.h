// Copyright Mikolaj Radwan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Zkz::ExecutionGraph
{

class FPayloadDeleter
{
public:
	using FDestroyFunc = void (*)(void*);

	FPayloadDeleter() = default;

	explicit FPayloadDeleter(FDestroyFunc InDestroy) : Destroy(InDestroy)
	{
	}

	void operator()(void* Ptr) const
	{
		if (Ptr != nullptr)
		{
			check(Destroy != nullptr);
			Destroy(Ptr);
		}
	}

private:
	FDestroyFunc Destroy = nullptr;
};

using FPayload = TUniquePtr<void, FPayloadDeleter>;

template <class T, class... ArgTypes>
FPayload MakePayload(ArgTypes&&... Args)
{
	// Note: cannot use TUniquePtr's pointer constructors - they are constrained on
	// UE::CPointerConvertibleTo<U, void>, which fails to compile in UE 5.8.
	// Reset() and GetDeleter() are unconstrained, so we go through those.
	FPayload Result;
	Result.GetDeleter() = FPayloadDeleter{[](void* Ptr) { delete static_cast<T*>(Ptr); }};
	Result.Reset(new T{Forward<ArgTypes>(Args)...});
	return Result;
}

}  // namespace Zkz::ExecutionGraph
