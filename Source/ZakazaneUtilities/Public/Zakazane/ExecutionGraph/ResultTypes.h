// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Zakazane/Future.h"

namespace Zkz::ExecutionGraph
{

using FJobCompletionPromise = TScopedPromise<void>;
using FFutureJobCompletion = TCancelableFuture<void>;

// Expects to typically hold 3 promises for stages - for external code enqueueing the job,
// for the containing stage and for a successor.
// Tasks will probably typically have a single completion for the containing stage.
using FJobCompletionPromises = TArray<FJobCompletionPromise, TInlineAllocator<3>>;

using FJobExecutionPromise = TScopedPromise<void>;
using FFutureJobExecution = TCancelableFuture<void>;

struct FTaskArgs
{
	FJobCompletionPromise CompletionPromise;
	void* Payload = nullptr;
};

using FTaskExecutionPromise = TScopedPromise<FTaskArgs>;
using FFutureTaskExecution = TCancelableFuture<FTaskArgs>;

using FFuturePredecessorCompletions = TArray<FFutureJobCompletion, TInlineAllocator<1>>;

}  // namespace Zkz::ExecutionGraph
