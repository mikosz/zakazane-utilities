#include "Algo/Transform.h"
#include "Zakazane/Functional.h"
#include "Zakazane/Future.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::Test
{

ZKZ_BEGIN_AUTOMATION_TEST(
	FFutureTest,
	"Zakazane.ZakazaneUtilities.Future",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(ScopedPromiseSetsOnDeleteIfNotExecutedOrMovedFrom)
{
	{
		const TFuture F = []
		{
			TScopedPromise<bool> P;
			TFuture F = P.GetFuture();
			P.SetValue(true);
			return F;
		}();

		TestEqual("SetValuePresentInFuture", F.Get().GetValueOr(false), true);
	}

	{
		const TFuture F = []
		{
			TScopedPromise<bool> P;
			TFuture F = P.GetFuture();
			return F;
		}();

		TestTrue("CancelledValuePresentInFuture", F.Get().HasError());
	}

	{
		const TFuture F = []
		{
			TScopedPromise<bool> MovedFrom;
			TFuture F = MovedFrom.GetFuture();

			{
				TScopedPromise MovedTo{MoveTemp(MovedFrom)};
				MovedTo.SetValue(true);
			}

			return F;
		}();

		TestEqual("SetValuePresentInFutureMoved", F.Get().GetValueOr(false), true);
	}

	{
		const TFuture F = []
		{
			TScopedPromise<bool> MovedFrom;
			TFuture F = MovedFrom.GetFuture();

			{
				TScopedPromise MovedTo{MoveTemp(MovedFrom)};
			}

			return F;
		}();

		TestTrue("CancelledValuePresentInFutureMoved", F.Get().HasError());
	}

	{
		const TFuture F = []
		{
			TScopedPromise<int> MovedFrom;
			TFuture F = MovedFrom.GetFuture();

			{
				// ReSharper disable once CppJoinDeclarationAndAssignment
				TScopedPromise<int> MovedTo;
				MovedTo = MoveTemp(MovedFrom);
				MovedTo.SetValue(2);
			}

			return F;
		}();

		TestEqual("SetValuePresentInFutureMoveAssigned", F.Get().GetValueOr(-1), 2);
	}

	{
		const TFuture F = []
		{
			TScopedPromise<int> MovedFrom;
			TFuture F = MovedFrom.GetFuture();

			{
				// ReSharper disable once CppEntityAssignedButNoRead
				// ReSharper disable once CppJoinDeclarationAndAssignment
				TScopedPromise<int> MovedTo;
				MovedTo = MoveTemp(MovedFrom);
			}

			return F;
		}();

		TestTrue("CancelledValuePresentInFutureMoveAssigned", F.Get().HasError());
	}
}

ZKZ_ADD_TEST(AggregateFuturesAccumulatesResults)
{
	TArray<TPromise<int>> Promises;
	Promises.SetNum(10);

	TArray<TFuture<int>> Futures;
	for (TPromise<int>& Promise : Promises)
	{
		Futures.Emplace(Promise.GetFuture());
	}

	TArray<int> InitialResult;
	InitialResult.Reserve(Promises.Num());	// Reserve ensures initial result contains enough space to hold all
		// results. Thanks to this, the array should not be reallocated and we can verify the array is moved correctly
		// by checking that the data address is the same.
	const int* OrigData = InitialResult.GetData();

	const TFuture<TArray<int>> AggregatedFuture = AggregateFutures(
		MoveTemp(Futures),
		MoveTemp(InitialResult),
		[](TArray<int>&& Results, const int Result) -> TArray<int>
		{
			Results.Emplace(Result);
			return MoveTemp(Results);
		});

	for (int Idx = 0; Idx < Promises.Num(); ++Idx)
	{
		Promises[Idx].SetValue(Idx + 1);
	}

	const TArray<int>& Result = AggregatedFuture.Get();
	TestEqual("ResultsGivenInOrder", Result, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
	TestEqual("AggregateMovesResult", Result.GetData(), OrigData);
}

ZKZ_ADD_TEST(AggregateFuturesCanUseScopedPromise)
{
	const TFuture<int> AggregatedFuture = []
	{
		TArray<TScopedPromise<int>> Promises;
		// Scoped promise will return -1 if unfulfilled
		for (int Idx = 0; Idx < 10; ++Idx)
		{
			Promises.Emplace();
		}

		TArray<TCancelableFuture<int>> Futures;
		for (TScopedPromise<int>& Promise : Promises)
		{
			Futures.Emplace(Promise.GetFuture());
		}

		// Set value for odd numbers only
		for (int Idx = 1; Idx < Promises.Num(); Idx += 2)
		{
			Promises[Idx].SetValue(Idx);
		}

		return AggregateFutures(
			MoveTemp(Futures),
			0,
			[](const int Sum, const TResult<int, FPromiseCanceled>& Result) { return Sum + Result.GetValueOr(-1); });
	}();

	// Result should be 1 + 3 + ... + 9 - 5 (for the even numbers) = 20
	TestEqual("AggregateFuturesAccumulatesResults", AggregatedFuture.Get(), 20);
}

ZKZ_ADD_TEST(NextChainsFutures)
{
	// int -> FString
	{
		TPromise<int> IntPromise;

		bool bContinuationCalled = false;
		Next(IntPromise.GetFuture(), [](int V) { return LexToString(V); })
			.Next(
				[this, &bContinuationCalled](const FString& S)
				{
					TestEqual("(int -> FString) Got expected argument", S, TEXT("42"));
					bContinuationCalled = true;
				});

		IntPromise.SetValue(42);
		TestTrue("(int -> FString) Continuation called", bContinuationCalled);
	}

	// void -> FString
	{
		TPromise<void> VoidPromise;

		bool bContinuationCalled = false;
		Next(VoidPromise.GetFuture(), []() { return TEXT("Good"); })
			.Next(
				[this, &bContinuationCalled](const FString& S)
				{
					TestEqual("(void -> FString) Got expected argument", S, TEXT("Good"));
					bContinuationCalled = true;
				});

		VoidPromise.SetValue();
		TestTrue("(void -> FString) Continuation called", bContinuationCalled);
	}

	// int -> void
	{
		TPromise<int> IntPromise;

		bool bContinuationCalled = false;
		Next(IntPromise.GetFuture(), [this](int V) { TestEqual("(int -> void) Got expected argument", V, 123); })
			.Next([&bContinuationCalled] { bContinuationCalled = true; });

		IntPromise.SetValue(123);
		TestTrue("(int -> void) Continuation called", bContinuationCalled);
	}

	// void -> void
	{
		TPromise<void> IntPromise;

		bool bContinuation1Called = false;
		bool bContinuation2Called = false;

		Next(IntPromise.GetFuture(), [this, &bContinuation1Called]() { bContinuation1Called = true; })
			.Next([&bContinuation2Called] { bContinuation2Called = true; });

		IntPromise.SetValue();
		TestTrue("(void -> void) Continuation 1 called", bContinuation1Called);
		TestTrue("(void -> void) Continuation 2 called", bContinuation2Called);
	}
}

ZKZ_ADD_TEST(CollapseFutureCanceledToError)
{
	TFutureResult<FString, int> FutureResult;

	{
		TScopedPromise<TResult<FString, int>> StringIntPromise;
		FutureResult = CollapseFutureCanceledToError(StringIntPromise.GetFuture(), 3);
	}

	ZKZ_RETURN_IF(!TestTrue("PromiseFulfilled", FutureResult.IsReady()));
	ZKZ_RETURN_IF(!TestTrue("HasError", FutureResult.Get().HasError()));
	TestEqual("ErrorHasCollapsedValue", FutureResult.Get().GetError(), 3);
}

ZKZ_END_AUTOMATION_TEST(FFutureTest);

}  // namespace Zkz::Test
