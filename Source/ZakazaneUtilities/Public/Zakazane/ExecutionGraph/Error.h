// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Zkz::ExecutionGraph
{

struct FPredecessorsDontHaveSameParent
{
	static FString ToString();
};

struct FAddedJobToClosedStageError
{
	static FString ToString();
};

struct FStageAlreadyClosedError
{
	static FString ToString();
};

struct FInvalidOperationError
{
	static FString ToString();
};

struct ZAKAZANEUTILITIES_API FCircularDependencyError
{
	using FCycle = TArray<FString>;

	FCycle Cycle;

	explicit FCircularDependencyError(FCycle InCycle);

	FString ToString() const;
};

using FError = TVariant<
	FPredecessorsDontHaveSameParent,
	FAddedJobToClosedStageError,
	FStageAlreadyClosedError,
	FCircularDependencyError,
	FInvalidOperationError>;

template <class VariantType, class... ArgTypes>
FError MakeError(ArgTypes&&... Args)
{
	return FError{TInPlaceType<VariantType>{}, Forward<ArgTypes>(Args)...};
}

ZAKAZANEUTILITIES_API FString ToString(const FError& Error);

}  // namespace Zkz::ExecutionGraph
