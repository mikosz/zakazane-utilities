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

}  // namespace Zkz
