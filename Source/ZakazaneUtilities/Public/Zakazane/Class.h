// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Misc/Optional.h"

namespace Zkz
{

/// Convenience function safely doing Object->GetClass()->GetName() with pointer checks
ZAKAZANEUTILITIES_API TOptional<FString> GetClassName(const UObject* Object);

/// Convenience function safely doing Object.GetClass()->GetName() with pointer checks
ZAKAZANEUTILITIES_API TOptional<FString> GetClassName(const UObject& Object);

/// Returns Class Display Name if it's available or normal Class Name in other case
ZAKAZANEUTILITIES_API FString GetClassDisplayNameOrClassName(const UClass& Class);

/// Returns Object's Class Display Name if it's available or normal Class Name in other case
ZAKAZANEUTILITIES_API TOptional<FString> GetClassDisplayNameOrClassName(const UObject& Object);

}  // namespace Zkz
