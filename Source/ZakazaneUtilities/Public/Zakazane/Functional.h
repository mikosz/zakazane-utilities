// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Zkz
{

/// Simple way to create a functor returning a literal.
/// Usage:
/// <pre>
///		TLiteralFunction<3>{}() // will return 3
///	</pre>
///
///	Useful with functions taking a functor as parameter, where we want to return a constant. E.g.:
///	<pre>
///		TLazy{TLiteralFunction<false>{}}
///	</pre>
///
///	Will lazily evaluate to false.
template <auto LiteralValue>
struct TLiteralFunction
{
	constexpr auto operator()() const
	{
		return LiteralValue;
	}
};

static_assert(TLiteralFunction<42>{}() == 42);

/// Sum functor similar to std::plus, but doesn't require an explicit template argument for the result type or the
/// arguments of type convertible to result type (will work for any arguments for each operator + is defined and return
/// type that's the result of operator +. Works with 1 or more arguments.
struct FSum
{
	template <class... ArgTypes>
	constexpr auto operator()(const ArgTypes&... Args) const
	{
		return (... + Args);
	}
};

static_assert(FSum{}(1) == 1);
static_assert(FSum{}(1, 2) == 3);
static_assert(FSum{}(1, 2, 3) == 6);
static_assert(FSum{}(1.0f, 2.0, 3) == 6);

/// Constexpr version of FIdentityFunctor
struct FIdentityFunctor
{
	template <typename T>
	constexpr T&& operator()(T&& Val) const
	{
		return Forward<T>(Val);
	}
};

}  // namespace Zkz
