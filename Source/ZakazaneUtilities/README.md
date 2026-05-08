# ZakazaneUtilities

1. [Algo.h](#algoh) - algorithms
1. [ArrayView.h](#arrayviewh) - array utilities
1. [Functional.h](#functionalh) - functional-programming utilities
1. [Future.h](#futureh) - utilities for working with Unreal's futures
1. [Logging.h](#loggingh) - logging utilities
1. [Pointer.h](#pointerh) - utilities for working with pointers
1. [Budgeting](#budgeting) - set of classes implementing runing tasks within an allotted time budget
1. [Execution Graph](#execution-graph) - set of classes implementing task scheduling with dependencies

## Algo.h

Extensions for Unreal's `Algo` namespace functions.

### `Transform`

Similar to `Algo::Transform`, but returns an range instead of taking it as an argument. Automatically
calls `Reserve` on the range to ensure it has enough capacity to hold the result.

## ArrayView.h

Utilities for working with `TArrayView`.

### `MakePtrToConstArrayView`

Create an array view of pointers to const. Note that while `MakeConstArrayView` exists, for pointers
it creates a view to const pointers, that is pointers that cannot be moved. Conversely, this function
creates an array view of pointers to objects that cannot be modified.

### `GetArrayViewTypeHash`

Hashing function for `TArrayView`s. Copied from Unreal's implementation of `GetTypeHash` for `TArray`.
Allows to use `TArrayView`s as keys in `TMap`s.

### `Equal(TConstArrayView, TConstArrayView)`

Compares two `TArrayView`s for equality by comparing their elements one by one (as opposed to telling
whether two array views point to the same data).

## Functional.h

### Compose

Allows to compose multiple function calls into one. Useful if you're running an algorithm and want to avoid
creating a new lambda for visibilty. Consider for example: you want to get an array of names of all employees
in an array. If the `FEmployee::GetName()` function returns an `FText` and you want an `FString`
`Algo::Transform(Employees, EmployeeNames, &FEmployee::GetName)` won't work, as there's no implicit conversion
from `FText` to `FString`. You can create a lambda, which is quite verbose, or use `Compose`:
`Algo::Transform(Employees, EmployeeNames, TCompose{&FEmployee::GetName, &FText::ToString})`}.

### Equals

Both Unreal and C++ standard library have a binary `EqualTo` functor, TEquals creates a unary functor which
compares the argument to the element passed during construction.

## Future.h

### `TScopedPromise` / `TCancelableFuture` / `IfNotCanceled`

Unreal's promises require that they are fulfilled before they are destroyed, making it hard to work with
them, considering Garbage Collection. `TScopedPromise` is a solution to this problem, it enforces fulfillment
of the promise at destruction time (if it hasn't been fulfilled yet). In case the promise is being fulfilled
normally, a valid `TResult` containing it is returned, while if it goes out of scope, an error result
containing a `FPromiseCanceled` is returned.

When using `TScopedPromise` it's better to use the `IfNotCanceled` helper function whenever you'd normally
use `Next`. This is because when implementing `Next`, you'd need to take a `TResult` argument, and handle
the promise-canceled case each time. `IfNotCanceled` handles this for you, only calling the passed function
if the result is valid, and providing the value as an argument.

```c++
TCancelableFuture<int> CalculateInt()
{
    TScopedPromise<int> Promise;
    AsyncFunctionCalculatingInt(MoveTemp(Promise));
    return Promise.GetFuture();
}

void Foo()
{
	IfNotCanceled(
		CalculateInt(),
		[](int Value) {
			// Note we're taking an int argument, not a TResult<int, FPromiseCanceled>
			UE_LOG(LogTemp, Log, TEXT("Calculated value: %d"), Value);
		}
	);
}
```

### `AggregateFutures` / `AggregateFutureResults`

Combines results of multiple futures into a single one.

`AggregateFutures` takes an array of futures and retrieves their returned values in order. The values
are then combined using the passed aggregation function.

`AggregateFutureResults` simplifies using `AggregateFutures` with `TCancelableFuture`s. It aggregates
the results of the futures in order, using the provided aggregation function. If any of the future
results is an error, aggregation is stopped and the error is returned.

```c++
TCancelableFuture<int> CalculateInt(int Idx)
{
    TScopedPromise<int> Promise;
    AsyncFunctionCalculatingInt(MoveTemp(Promise), Idx);
    return Promise.GetFuture();
}

TCancelableFuture<int> CalculateSumOfInts()
{
    TArray<TCancelableFuture<int>> FutureInts;
	for (int Idx = 0; Idx < 10; ++Idx)
	{
		FutureInts.Emplace(CalculateInt(Idx));
	}
	
	TCancelableFuture<int> FutureSum = AggregateFutureResults(
		FutureInts,
		0,
		[](int AggregatedValue, int Value)
		{
			return AggregatedValue + Value;
		});
		
	return FutureSum;
}
```

`AggregateFutureResults` also has a version returning `TResult<void, FPromiseCanceled>`, useful when
aggregating void futures or if you're not interested in the result, just that it's not an error.

### `MakeImmediateFuture`

Creates a `TScopedPromise`, immediately fulfills it with the provided value, and returns its future.
Useful for error handling in functions returning a future.

Very similar to Unreal's `MakeFulfilledPromise`, but while that returns a promise, we return a future.

## Logging.h

### `TLogCategoryRef`

Unreal's `UE_LOG` macro doesn't allow to pass a category reference. This is problematic when implementing
template classes that would like to be able to log to a user-defined log category. `TLogCategoryRef` works
around this issue. Your template can take a `TLogCategoryRef` argument, store it, and use it normally in
`UE_LOG` with compile-time verbosity handled normally.

There's also a `TIsLogCategory` type-trait and a `CLogCategory` to be used alongside.

```c++
template <CLogCategory Category>
class TSomeClass
{
public:
	TSomeClass(TLogCategoryRef InLogCategory)
		: LogCategory(MoveTemp(InLogCategory))
	{}
	
	void Foo()
	{
		UE_LOG(LogCategory, Log, TEXT("Foo called!"));
	}
private:
	TLogCategoryRef LogCategory;
};

DECLARE_LOG_CATEGORY_EXTERN(LogSuperCustom, Log, All);

void Bar()
{
	TSomeClass<decltype(LogSuperCustom)> SomeClass{LogSuperCustom};
	SomeClass.Foo();
	
	// ... or use template argument deduction
	TSomeClass SomeClass{LogSuperCustom};
	SomeClass.Foo();
}
```

## Pointer.h

### `Pointer::Get`, `Pointer::IsValid`

These functions provide a common interface for working with raw pointers and smart pointers, as well as pointers to
`UObject`s and other types.

#### `Pointer::Get`

- For raw pointers returns the pointer itself,
- for smart pointers returns the result of `.Get()`,
- for `TSharedRef` returns the result of `.Get()` as a pointer (the function returns a reference)

#### `Pointer::IsValid`

- For `UObject` pointers returns whether the pointer is not null and `::IsValid()` called on that object returns true,
- for smart pointers returns the result of `.IsValid()`,
- for raw pointers returns whether the pointer is not null,
- for `TSharedRef` returns true.

### Pointer identity traits, `TLifetimeTrackingPtrTraits`, `CLifetimeTrackingPtr`

These utilities can be used whenever you're implementing generic code that should be pointer-type agnostic and
needs a common interface.

A lifetime-tracking pointer is a smart pointer that knows whether the object it points to is still alive. This can
be a pointer to a UObject-based object or an unrelated type. Lifetime-tracking pointers can be:

- Owning-pointers, with unique (TUniquePtr) or shared ownership (TSharedPtr, TSharedRef, TStrongObjectPtr),
- or non-owning-pointers: TWeakPtr, TWeakObjectPtr.

Identity traits identifying the category of a lifetime-tracking pointer are provided, e.g.:

- `IsWeakPtr` / `CTypeOfWeakPtr`,
- `IsSharedPtr` / `CTypeOfSharedPtr`,

As well as category traits:

- `IsOwningPtr` / `CTypeOfOwningPtr`,
- `IsWeakOwningPtr` / `CTypeOfWeakOwningPtr`.

`TLifetimeTrackingPtrTraits` is traits class containing a `Pin` function, returning an owning version of
the pointed-to object. This is the equivalent of `Pin()` in weak pointers and identity for owning pointers.

Note that you can provide your own implementations for `TLifetimeTrackingPtrTraits` if you need to.

`CLifetimeTrackingPtr` is a concept for all types that implement `TLifetimeTrackingPtrTraits`.

## Result.h

Similar to Rust's Result type. Combines a value and error type into a single type. Data is stored as a union,
so the size of the object is the maximum of the two types (+ active type's index).

`Ok` and `Err` functions are provided to simplify instantiating results.

`ZKZ_PROPAGATE_IF_ERROR` macro is provided to simplify handling error results and passing them down the
call stack.

```c++
TResult<int, FString> GetSum(TOptional<int> First, TOptional<int> Second)
{
	if (First.IsSet() && Second.IsSet())
	{
		return Ok(First.GetValue() + Second.GetValue());
	}
	else
	{
		return Err(TEXT("Both arguments must be set"));
	}
}

// Note that the function must return a TResult with a compatible error type to use ZKZ_PROPAGATE_IF_ERROR
TResult<void, FString> LogSum(TOptional<int> First, TOptional<int> Second)
{
	TResult<int, FString> SumResult = GetSum(First, Second);
	ZKZ_PROPAGATE_IF_ERROR(SumResult);
	UE_LOG(LogTemp, Log, TEXT("Sum: %d"), SumResult.GetValue());
	return Ok();
}
```

## ScopedLock.h

### `TScopedLock`

Copied from Unreal's TScopeLock but implements move semantics.

# Budgeting

## `TBudget`, `UZkzTickableBudget`

Create a `TBudget` to allot some amount of time per-frame for execution of tasks from some category. Tasks that
can't be executed this frame will be executed in future frames as time allows.

`UZkzTickableBudget` is a wrapper for `TBudget` that ticks automatically (but doesn't allow customization of
the log category or inspections type).

```c++
...
class USpawningSubsystem : public UTickableWorldSubsystem
{
...

	virtual void Initialize(...) override
	{
		Budget.SetPerFrameBudget(FTimespan::FromMilliseconds(4.0);
	}

	virtual void Tick(...) override
	{
		Budget.Tick();
	}
	
	virtual void SpawnCharacter(FName CharacterName) override
	{
		// Lambda will be executed when time budget allows (immediately if still have budget this frame)
		Budget.EnqueueTask([CharacterName]() {
			...		
		});
	}
	
private:
	
	DECLARE_LOG_CATEGORY_CLASS(LogSpawning, Log, All);
	
	TBudget<decltype(LogSpawning)> Budget{LogSpawning};
	
};

```

## `EnqueueBudgetedTask`

This is a bridge between Budgeting and Execution Graph. It's a utility function adding an execution graph job
that executes within a budget. Note that the budget must be passed to the function as a lifetime-tracking pointer.
This is because we must ensure that the budget is alive when the task is executed.

## `DeveloperToolUI`

Helper class for creating an ImGui Developer Tool containing budget-related widgets.

## `Inspections`

Gathers monitoring data for budgets. By default, data gathering is enabled for non-shipping builds. You can change
this per-class by providing a inspections type as a template argument, or modify the default by defining
`FORCE_BUDGETING_INSPECTIONS` as 0 or 1.

# Execution Graph

Create a `TScheduler` to allow scheduling of tasks.

## JobId

Job identifiers are hierarchical. Segments are separated by a dot ("."). The last segments determines the job
name, the earlier segments are the stages the job belongs to. E.g., a task id "GameInitialization.LoadGame.SpawnCharacter"
is "SpawnCharacter" that belongs to the stage "GameInitialization", which in turn is contained in "LoadGame".

Job ids can be parsed from a string using `TScheduler::MakeJobIdFromString`.

Job ids must be unique within the scheduler. If you need to re-use a job id (as in the example above - it would make sense
to create multiple "SpawnCharacter" tasks for each spawned character), you should use `TScheduler::MakeUnique(JobId)` or
`TScheduler::MakeUniqueJobIdFromString(FStringView)`.

The internal type used to store the job id is determined by a template parameter, `InJobIdTraitsType`. This defaults
to `TDefaultSchedulerJobIdTraits`, a type that should cover most use-cases. Note that `TDefaultSchedulerJobIdTraits`
uses a fixed-size array to represent the internal hierarchy, so if you need more segments, either change the default
`DefaultSchedulerJobIdMaxSegments` or instantiate the type with a larger number.

## Predecessors

All jobs in the scheduler can define a set of predecessors. These are jobs that must complete before the dependent
job can start.

Note that all predecessors must belong to the same stage as the dependent job, so "Game.MoveCharacter" can
depend on "Game.SpawnCharacter", but can't depend on "Initialization.LoadAssets". Such dependency should be
represented by the "Game" stage depending on the "Initialization" stage.

## Stage

A stage can be understood as a directory for jobs.

Stages are enqueued in the scheduler using `TScheduler::EnqueueStage`. This function enqueues the stage for
execution, defining its predecessors. When all predecessors are completed, the stage triggers its jobs with
no predecessors for immediate execution. Stages remain in the executing stage until all their jobs are
completed *and* they are closed. Stages are closed using the `TScheduler::CloseStage` function. Closed stages
don't allow enqueueing of new jobs.

## Task

A task is a job that performs user-defined work. Communication between the defined task and user-code is
performed via `FFutureTaskExecution`. The user enqueues a task, gets a future task execution in return,
calls `IfNotCanceled` to define what work should be done. The future contains a `FJobCompletionPromise`
which the user-code must fulfill to notify the scheduler that the task is complete.

## Order of execution

The execution order for enqueueing jobs is not enforced. You can enqueue jobs within undefined stages or
introduce dependencies to undefined jobs. The scheduler will create stubs for such jobs allowing for later
definition.

## Thread-safety

`TScheduler`'s thread safety is determined by the `SynchronizationTraits` template parameter. By default,
it uses the `FThreadUnsafe` trait. The `FThreadSafe` trait can be used to enable thread-safety.
The thread unsafe trait asserts that all work is performed in the game thread.

Note that when using the thread-safe trait, `TScheduler` doesn't guarantee that tasks execute on any
specific thread. This means that in a thread-safe context tasks should probably use `Async` or a similar
function to make sure the code is executed on the correct thread.
