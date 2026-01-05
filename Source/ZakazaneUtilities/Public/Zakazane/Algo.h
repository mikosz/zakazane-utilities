// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Containers/ArrayView.h"
#include "Templates/IdentityFunctor.h"
#include "Templates/Invoke.h"
#include "Templates/Less.h"
#include "Templates/UnrealTemplate.h"

#include <type_traits>

namespace Zkz
{

namespace AlgoImpl
{

template <class RangeType, class ProjectionType, class ComparatorType>
auto MinBy(RangeType&& Range, ProjectionType Projection, ComparatorType Comparator)
{
	using ValueType = std::decay_t<decltype(::Invoke(Projection, *std::begin(Range)))>;
	TOptional<ValueType> Result;

	for (auto& Element : Range)
	{
		auto ProjectedElement = ::Invoke(Projection, Element);
		if (!Result || ::Invoke(Comparator, ProjectedElement, *Result))
		{
			Result = MoveTemp(ProjectedElement);
		}
	}

	return Result;
}

}  // namespace AlgoImpl

/// Returns the array view index of an array element provided as a pointer. Useful to get indices of elements found by
/// using Algo functions. Note that the pointer is not checked for nullness or for a valid index.
template <class T, class U UE_REQUIRES(std::is_same_v<std::remove_cv_t<T>, std::remove_cv_t<U>>)>
int32 PointerToIndex(const TArrayView<U> Array, const T* const Ptr)
{
	return Ptr - Array.GetData();
}

/// Returns the array view index of an array element provided as a pointer. Useful to get indices of elements found by
/// using Algo functions. Note that the pointer is not checked for nullness or for a valid index.
template <class T, class U UE_REQUIRES(std::is_same_v<std::remove_cv_t<T>, std::remove_cv_t<U>>)>
int32 PointerToIndex(const TArray<U>& Array, const T* const Ptr)
{
	return PointerToIndex(MakeArrayView(Array), Ptr);
}

/// Similar to Algo::MinElement, but returns the underlying value or NullOpt if Range was empty.
template <class RangeType>
[[nodiscard]] auto Min(RangeType&& Range)
{
	return AlgoImpl::MinBy(Range, FIdentityFunctor{}, TLess<>{});
}

/// Similar to Algo::MinElement, but returns the underlying value or NullOpt if Range was empty.
template <class RangeType, class ComparatorType>
[[nodiscard]] auto Min(RangeType&& Range, ComparatorType Comparator)
{
	return AlgoImpl::MinBy(Range, FIdentityFunctor{}, Comparator);
}

/// Similar to Algo::MinElementBy, but returns the underlying value or NullOpt if Range was empty.
template <class RangeType, class ProjectionType>
[[nodiscard]] auto MinBy(RangeType&& Range, ProjectionType Projection)
{
	return AlgoImpl::MinBy(Range, Projection, TLess<>{});
}

/// Similar to Algo::MinElementBy, but returns the underlying value or NullOpt if Range was empty.
template <class RangeType, class ProjectionType, class ComparatorType>
[[nodiscard]] auto MinBy(RangeType&& Range, ProjectionType Projection, ComparatorType Comparator)
{
	return AlgoImpl::MinBy(Range, Projection, Comparator);
}

}  // namespace Zkz
