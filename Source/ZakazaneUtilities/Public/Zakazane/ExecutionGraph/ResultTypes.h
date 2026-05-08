// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Zakazane/Future.h"
#include "Zakazane/Result.h"

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

using FTaskExecutionPromise = TScopedPromise<FJobCompletionPromise>;
using FFutureTaskExecution = TCancelableFuture<FJobCompletionPromise>;

using FFuturePredecessorCompletions = TArray<FFutureJobCompletion, TInlineAllocator<1>>;

}  // namespace Zkz::ExecutionGraph
