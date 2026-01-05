// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "Misc/TVariant.h"
#include "TypeTag.h"

namespace Zkz
{

namespace Private
{

template <class FunctionType, class HeadType, class... TailTypes>
void ForEachVariantType(TTypeTag<TVariant<HeadType, TailTypes...>>, FunctionType&& Function)
{
	Function(TTypeTag<HeadType>{});

	if constexpr (sizeof...(TailTypes) > 0)
	{
		ForEachVariantType(TTypeTag<TVariant<TailTypes...>>{}, Forward<FunctionType>(Function));
	}
}

}  // namespace Private

template <class T>
constexpr bool IsVariant_V = false;

template <class... Types>
constexpr bool IsVariant_V<TVariant<Types...>> = true;

static_assert(!IsVariant_V<int>);
static_assert(IsVariant_V<TVariant<int, double, char>>);

/// Calls the Function for each type in the type list of the given variant passed as a TTypeTag. E.g.
/// @code
/// ForEachVariantType<TVariant<int, double, char>>(Func);
/// @endcode
///
/// calls:
///
/// @code
/// Func(TTypeTag<int>);
/// Func(TTypeTag<double>);
/// Func(TTypeTag<char>);
/// @endcode
template <class VariantType, class FunctionType UE_REQUIRES(IsVariant_V<VariantType>)>
void ForEachVariantType(FunctionType&& Function)
{
	Private::ForEachVariantType(TTypeTag<VariantType>{}, Forward<FunctionType>(Function));
}

/// Executes Invoke(Function, UnderlyingVariantValue). Useful instead of Visit when calling a function from a common
/// superclass of all variants. E.g. for
/// @code
/// TVariant<const UStaticMeshComponent*, const UDynamicMeshComponent*>
/// @endcode
/// both types derive from UPrimitiveComponent, so you could call
/// @code VariantInvoke(V, &USceneComponent::GetComponentTransform) @endcode instead of the more complicated Visit.
template <class VariantType, class FunctionType UE_REQUIRES(IsVariant_V<std::decay_t<VariantType>>)>
auto VariantInvoke(VariantType&& Variant, FunctionType&& Function)
{
	return Visit([&]<class T>(T&& V) { return Invoke(Function, Forward<T>(V)); }, Forward<VariantType>(Variant));
}

/// Constructs a variant object in-place and returns a reference to the object.
template <class EmplaceType, class VariantType, class... ArgTypes UE_REQUIRES(IsVariant_V<std::decay_t<VariantType>>)>
EmplaceType& VariantEmplace_GetRef(VariantType& Variant, ArgTypes&&... Args)
{
	Variant.template Emplace<EmplaceType>(Forward<ArgTypes>(Args)...);
	return Variant.template Get<EmplaceType>();
}

}  // namespace Zkz
