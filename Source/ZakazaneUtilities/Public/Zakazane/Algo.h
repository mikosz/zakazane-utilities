// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Containers/ArrayView.h"
#include "Templates/IdentityFunctor.h"
#include "Templates/Invoke.h"
#include "Templates/Less.h"
#include "Templates/UnrealTemplate.h"
#include "TypeTraits.h"

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

template <class RangeType>
concept CHasNum = requires(RangeType& Range) {
	{ Range.Num() } -> std::convertible_to<int32>;
};

template <class RangeType>
concept CHasReserve = requires(RangeType& Range) { Range.Reserve(); };

}  // namespace AlgoImpl

/// Does what Algo::Transform does, but returns a new container instead of taking an output parameter.
/// @tparam OutContainerType - result container type
template <class OutContainerType, class InContainerType, class FunctionType, class... AdditionalArgTypes>
	requires(CInvokable<FunctionType, const typename InContainerType::ElementType&, AdditionalArgTypes...>)
OutContainerType TransformTo(
	const InContainerType& InContainer, FunctionType&& F, AdditionalArgTypes&&... AdditionalArgs)
{
	OutContainerType Result;

	if constexpr (AlgoImpl::CHasReserve<OutContainerType> && AlgoImpl::CHasNum<InContainerType>)
	{
		Result.Reserve(InContainer.Num());
	}

	for (const auto& Value : InContainer)
	{
		Result.Emplace(::Invoke(F, Value, Forward<AdditionalArgTypes>(AdditionalArgs)...));
	}

	return Result;
}

/// Does what Algo::Transform does, but returns a new array instead of taking an output parameter.
template <class InContainerType, class FunctionType, class... AdditionalArgTypes>
	requires(CInvokable<FunctionType, const typename InContainerType::ElementType&, AdditionalArgTypes...>)
auto Transform(const InContainerType& InContainer, FunctionType&& F, AdditionalArgTypes&&... AdditionalArgs)
{
	using TransformedType = decltype(::Invoke(
		F, std::declval<typename InContainerType::ElementType>(), std::declval<AdditionalArgTypes>()...));

	return TransformTo<TArray<TransformedType>>(
		InContainer, Forward<FunctionType>(F), Forward<AdditionalArgTypes>(AdditionalArgs)...);
}

/// Does what Algo::TransformIf does, but returns a new container instead of taking an output parameter.
/// @tparam OutContainerType - result container type
template <
	class OutContainerType,
	class InContainerType,
	class PredicateType,
	class FunctionType,
	class... AdditionalArgTypes>
	requires(CInvokable<FunctionType, const typename InContainerType::ElementType&, AdditionalArgTypes...>)
OutContainerType TransformToIf(
	const InContainerType& InContainer, PredicateType&& P, FunctionType&& F, AdditionalArgTypes&&... AdditionalArgs)
{
	OutContainerType Result;

	if constexpr (AlgoImpl::CHasReserve<OutContainerType> && AlgoImpl::CHasNum<InContainerType>)
	{
		Result.Reserve(InContainer.Num());
	}

	// Re-implements Algo::TransformIf adding support for additional arguments
	for (const auto& Value : InContainer)
	{
		if (::Invoke(P, Value))
		{
			Result.Emplace(::Invoke(F, Value, Forward<AdditionalArgTypes>(AdditionalArgs)...));
		}
	}

	return Result;
}

/// Does what Algo::TransformIf does, but returns a new container instead of taking an output parameter.
template <class InContainerType, class PredicateType, class FunctionType, class... AdditionalArgTypes>
	requires(CInvokable<FunctionType, const typename InContainerType::ElementType&, AdditionalArgTypes...>)
auto TransformIf(
	const InContainerType& InContainer, PredicateType&& P, FunctionType&& F, AdditionalArgTypes&&... AdditionalArgs)
{
	using TransformedType = decltype(::Invoke(
		F, std::declval<typename InContainerType::ElementType>(), std::declval<AdditionalArgTypes>()...));

	return TransformToIf<TArray<TransformedType>>(
		InContainer,
		Forward<PredicateType>(P),
		Forward<FunctionType>(F),
		Forward<AdditionalArgTypes>(AdditionalArgs)...);
}

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
