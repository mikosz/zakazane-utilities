// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "TypeTraits.h"

namespace Zkz::Optional
{

/// If the optional value contains a value, returns TOptional{Func(value)}, otherwise propagates null opt.
template <class T, class FuncType, class... ArgTypes>
	requires(CInvokable<FuncType, const T&, ArgTypes...>)
auto Transform(const TOptional<T>& Optional, FuncType&& Func, ArgTypes&&... Args)
	-> TOptional<decltype(Invoke(Func, std::declval<const T&>(), std::declval<ArgTypes>()...))>
{
	return Optional.IsSet() ? Invoke(Forward<FuncType>(Func), Optional.GetValue(), Forward<ArgTypes>(Args)...)
							: NullOpt;
}

}  // namespace Zkz::Optional
