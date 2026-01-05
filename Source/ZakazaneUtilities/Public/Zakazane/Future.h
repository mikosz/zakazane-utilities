// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Async/Future.h"
#include "Result.h"
#include "ReturnIfMacros.h"

namespace Zkz
{

template <class T, class E>
using TFutureResult = TFuture<TResult<T, E>>;

template <class T, class E>
using TResultPromise = TPromise<TResult<T, E>>;

/// Error meaning the promise was destroyed before being fulfilled. This will most typically happen if the world
/// is being destroyed, or the object containing the promise gets garbage collected.
struct FPromiseCanceled
{
};

constexpr FPromiseCanceled PromiseCanceled;

template <class T>
using TCancelableFuture = TFutureResult<T, FPromiseCanceled>;

template <class T>
using TCancelableFutureResult = TResult<T, FPromiseCanceled>;

// #TODO #Promise: Review usages of TPromise and potentially replace with TScopedPromise
/// Wrapper for TPromise gracefully handling destruction prior to being fulfilled. The internal promise is
/// TResultPromise<T, FPromiseCancelled> and if the promise gets destroyed, the set value is
/// an error result.
template <class T>
class TScopedPromise
{
public:
	TScopedPromise() = default;

	explicit TScopedPromise(TUniqueFunction<void()>&& CompletionCallback) : Promise{MoveTemp(CompletionCallback)}
	{
	}

	TScopedPromise(TScopedPromise&& Other) : Promise{MoveTemp(Other.Promise)}, bFulfilled{MoveTemp(Other.bFulfilled)}
	{
		Other.bFulfilled = true;
	}

	TScopedPromise& operator=(TScopedPromise&& Other)
	{
		ZKZ_RETURN_IF(this == &Other, *this);

		Promise = MoveTemp(Other.Promise);
		bFulfilled = Other.bFulfilled;

		Other.bFulfilled = true;
		return *this;
	}

	~TScopedPromise()
	{
		// #TODO #Promise: Should have IsFulfilled in TPromise (push request)
		Cancel();
	}

	void Cancel()
	{
		if (!bFulfilled)
		{
			bFulfilled = true;
			Promise.EmplaceValue(Unexpect, PromiseCanceled);
		}
	}

	template <class... ArgTypes>
	void EmplaceValue(ArgTypes&&... Args)
	{
		bFulfilled = true;
		Promise.EmplaceValue(InPlace, Forward<ArgTypes>(Args)...);
	}

	template <class ValueType UE_REQUIRES(!std::is_void_v<T>)>
	void SetValue(ValueType&& Value)
	{
		bFulfilled = true;
		Promise.EmplaceValue(InPlace, Forward<ValueType>(Value));
	}

	TCancelableFuture<T> GetFuture()
	{
		return Promise.GetFuture();
	}

private:
	TResultPromise<T, FPromiseCanceled> Promise;

	bool bFulfilled = false;
};

/// Helper function similar to Next, but only calls the continuation function if the future result does not hold a
/// canceled promise error.
template <class T, class FunctionType UE_REQUIRES(!std::is_void_v<T> && TIsInvocable<FunctionType, T>::Value)>
void IfNotCanceled(TCancelableFuture<T> CancelableFuture, FunctionType F)
{
	CancelableFuture.Next(
		[F = MoveTemp(F)](TResult<T, FPromiseCanceled> Result) mutable
		{
			ZKZ_RETURN_IF(!Result.HasValue());
			F(MoveTemp(Result).GetValue());
		});
}

template <class FunctionType UE_REQUIRES(TIsInvocable<FunctionType>::Value)>
void IfNotCanceled(TCancelableFuture<void> CancelableFuture, FunctionType F)
{
	CancelableFuture.Next(
		[F = MoveTemp(F)](const TResult<void, FPromiseCanceled> Result) mutable
		{
			ZKZ_RETURN_IF(!Result.HasValue());
			F();
		});
}

/// Like TFuture::Next, but returns another TFuture for chaining. The TFuture type depends on the value returned by the provided
/// function. This has overhead, just use TFuture::Next if you don't need chaining.
/// Example:
/// <pre>
///			TFuture<int> FutureInt = FunctionReturningFutureInt();
///			Next(
///				MoveTemp(FutureInt),
///				[](int V)
///				{
///					// This is called first after FunctionReturningFutureInt completes
///					return LexToString(V);
///				})
///				.Next(
///					[](const FString& String)
///					{
///						// This is called after the first continuation completes
///						DoSomething(String);
///					});
/// </pre>
template <class T, class FunctionType UE_REQUIRES(TIsInvocable<FunctionType, T>::Value)>
[[nodiscard]] auto Next(TFuture<T> Future, FunctionType Continuation)
	-> TFuture<decltype(::Invoke(Continuation, DeclVal<T>()))>
{
	using FContinuationResult = decltype(::Invoke(Continuation, DeclVal<T>()));
	TPromise<FContinuationResult> ChainPromise;
	auto ChainFuture = ChainPromise.GetFuture();

	Future.Next(
		[ChainPromise = MoveTemp(ChainPromise), Continuation = MoveTemp(Continuation)](T Value) mutable
		{
			if constexpr (std::is_same_v<FContinuationResult, void>)
			{
				::Invoke(Continuation, Forward<T>(Value));
				ChainPromise.EmplaceValue();
			}
			else
			{
				ChainPromise.EmplaceValue(::Invoke(Continuation, Forward<T>(Value)));
			}
		});

	return ChainFuture;
}

template <class FunctionType UE_REQUIRES(TIsInvocable<FunctionType>::Value)>
[[nodiscard]] auto Next(TFuture<void> Future, FunctionType Continuation) -> TFuture<decltype(::Invoke(Continuation))>
{
	using FContinuationResult = decltype(::Invoke(Continuation));
	TPromise<FContinuationResult> ChainPromise;
	auto ChainFuture = ChainPromise.GetFuture();

	Future.Next(
		[ChainPromise = MoveTemp(ChainPromise), Continuation = MoveTemp(Continuation)]() mutable
		{
			if constexpr (std::is_same_v<FContinuationResult, void>)
			{
				::Invoke(Continuation);
				ChainPromise.EmplaceValue();
			}
			else
			{
				ChainPromise.EmplaceValue(::Invoke(Continuation));
			}
		});

	return ChainFuture;
}

template <class T, class E>
TFuture<TResult<T, E>> CollapseFutureCanceledToError(TCancelableFuture<TResult<T, E>> Future, E&& ErrorIfCanceled)
{
	return Next(
		MoveTemp(Future),
		[ErrorIfCanceled = MoveTemp(ErrorIfCanceled)](TCancelableFutureResult<TResult<T, E>> Result) mutable {
			return Result.HasValue() ? MoveTemp(Result).GetValue() : TResult<T, E>{Unexpect, MoveTemp(ErrorIfCanceled)};
		});
}

namespace AggregateFuturesPrivate
{

template <class FutureType, class ResultType, class AggregateFuncType>
auto DoAggregateFutures(
	const TArrayView<TFuture<FutureType>> Futures, ResultType&& Initial, AggregateFuncType&& AggregateFunc)
{
	// The example work done for (Initial, {Fut0, Fut1, Fut2}) is:
	// * creates Promise {Fut0, Fut1, Fut2}
	// * set Next handler for Fut0 - the handler:
	// ** calls the aggregate function for (Initial, Fut0)
	// ** calls aggregate futures for (result from line above, {Fut1, Fut2}) recursively
	// ** sets a Next handler for that call to fulfil the result promise

	TPromise<ResultType> Promise;
	auto AggregatedFuture = Promise.GetFuture();

	if (Futures.IsEmpty())
	{
		Promise.SetValue(Forward<ResultType>(Initial));
		return AggregatedFuture;
	}

	auto& Head = Futures[0];

	Head.Next(
		[Tail = Futures.RightChop(1),
		 Promise = MoveTemp(Promise),
		 Initial = Forward<ResultType>(Initial),
		 AggregateFunc = MoveTemp(AggregateFunc)]<class FutureResultType>(FutureResultType&& FutureResult) mutable
		{
			DoAggregateFutures(
				MoveTemp(Tail),
				::Invoke(AggregateFunc, Forward<ResultType>(Initial), Forward<FutureResultType>(FutureResult)),
				MoveTemp(AggregateFunc))
				.Next([Promise = MoveTemp(Promise)]<class FinalResultType>(FinalResultType&& FinalResult) mutable
					  { Promise.EmplaceValue(Forward<FinalResultType>(FinalResult)); });
		});

	return AggregatedFuture;
}

}  // namespace AggregateFuturesPrivate

/// Creates a single future from multiple futures. The result value is built by calling the given aggregate func.
/// The aggregate func is a binary function taking the accumulated result (or the Initial value) and a future
/// result. The futures are aggregated in order passed to the Futures argument.
template <
	class FutureType,
	class ResultType,
	class AggregateFuncType UE_REQUIRES(TIsInvocable<AggregateFuncType, ResultType, FutureType>::Value)>
auto AggregateFutures(TArray<TFuture<FutureType>> Futures, ResultType&& Initial, AggregateFuncType&& AggregateFunc)
{
	using namespace AggregateFuturesPrivate;

	// This is a recursive function calling a helper DoAggregateFutures. This is so that the futures are passed by value
	// here (need to be kept alive in the Next handler until the whole set is ready) but we can offset into it using
	// array view. The initial Futures array is kept alive by that last Next handler

	TPromise<ResultType> Promise;
	auto AggregatedFuture = Promise.GetFuture();

	DoAggregateFutures(MakeArrayView(Futures), Forward<ResultType>(Initial), Forward<AggregateFuncType>(AggregateFunc))
		.Next([Futures = MoveTemp(Futures),
			   Promise = MoveTemp(Promise),
			   Initial = Forward<ResultType>(Initial),
			   AggregateFunc = MoveTemp(AggregateFunc)]<class AggregatedResultType>(
				  AggregatedResultType&& Result) mutable { Promise.SetValue(Forward<AggregatedResultType>(Result)); });

	return AggregatedFuture;
}

}  // namespace Zkz
