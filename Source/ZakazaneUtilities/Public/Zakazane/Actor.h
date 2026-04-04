// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Zkz
{
#if WITH_EDITOR
/// Iterates over all World Partition actor descriptors, loading their corresponding OFPA assets into memory to execute the callback.
/// @note Bypasses standard world initialization.
/// Callback is executed only if the loaded Actor is valid.
/// @param bInRecursive
/// @param bInKeepReferences If true, actors remain pinned in memory (required if storing actor pointers in your validator).
/// If false, they are unloaded immediately after the callback to save memory.
ZAKAZANEUTILITIES_API void ForEachActorWithLoadingInWorld(
	const UWorld* InWorld,
	const TFunction<bool(AActor&)>& InCallback,
	bool bInRecursive = false,
	bool bInKeepReferences = false);
#endif
}  // namespace Zkz
