// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include <functional>

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

/// Allows function composition. Uses std::invoke for all invocations, so works with pointers to members, member
/// functions etc.
/// E.g.:
///		TCompose{&UActorComponent::Name, &FName::GetDisplayIndex}(ActorComp)
///	is equivalent to
///		ActorComp->Name.GetDisplayIndex()
template <class HeadFuncType, class... TailFuncTypes>
struct TCompose
{
	constexpr explicit TCompose(HeadFuncType InHead, TailFuncTypes... InTail)
		: Head{std::move(InHead)}, Tail{std::move(InTail)...}
	{
	}

	template <class... ArgType>
	constexpr decltype(auto) operator()(ArgType&&... Arg) &
	{
		return std::invoke(Tail, std::invoke(Head, std::forward<ArgType>(Arg)...));
	}

	template <class... ArgType>
	constexpr decltype(auto) operator()(ArgType&&... Arg) const&
	{
		return std::invoke(Tail, std::invoke(Head, std::forward<ArgType>(Arg)...));
	}

	template <class... ArgType>
	constexpr decltype(auto) operator()(ArgType&&... Arg) &&
	{
		return std::invoke(std::move(Tail), std::invoke(std::move(Head), std::forward<ArgType>(Arg)...));
	}

private:
	HeadFuncType Head;

	// This is a hack to avoid create a specialization for TCompose with an empty argument list and
	// to allow accepting multiple arguments.
	// If TailFuncTypes is just one function type, this evaluates to that function type, otherwise evaluates
	// to TCompose<TailFuncTypes...>.
	std::conditional_t<
		(sizeof...(TailFuncTypes) > 1),
		TCompose<TailFuncTypes...>,
		std::tuple_element_t<0, std::tuple<TailFuncTypes...>>>
		Tail;
};

template <class... FuncTypes>
TCompose(FuncTypes...) -> TCompose<std::decay_t<FuncTypes>...>;

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

/// Unary functor returning the result of == operator against the value passed during construction.
template <class LhsType>
struct TEquals
{
	LhsType Lhs;

	constexpr explicit TEquals(LhsType InLhs) : Lhs{MoveTemp(InLhs)}
	{
	}

	template <class RhsType>
	constexpr bool operator()(RhsType&& Rhs) const
	{
		return Lhs == Forward<RhsType>(Rhs);
	}
};

static_assert(TEquals{3}(3));
static_assert(!TEquals{3}(4));

/// Constexpr version of FIdentityFunctor
struct FIdentityFunctor
{
	template <typename T>
	constexpr T&& operator()(T&& Val) const
	{
		return Forward<T>(Val);
	}
};

static_assert(TCompose{FSum{}, FIdentityFunctor{}, FSum{}}(3, 4) == 7);

}  // namespace Zkz
