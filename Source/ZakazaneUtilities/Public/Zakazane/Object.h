// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Zkz
{

/// If the object is an actor and the label is available, returns the actor's label. Otherwise returns the
/// object name.
ZAKAZANEUTILITIES_API FString GetObjectNameOrLabel(const UObject& Object);

/// If the object is an actor and the label is available, returns the actor's label. Otherwise returns the
/// object name. If Object is invalid, returns NullOpt.
ZAKAZANEUTILITIES_API TOptional<FString> GetObjectNameOrLabel(const UObject* Object);

/// If the object is an actor and the label is available, returns the actor's label. Otherwise returns the
/// object name. If Object is invalid, returns IfInvalid.
ZAKAZANEUTILITIES_API FString
GetObjectNameOrLabelOr(const UObject* Object, const FString& IfInvalid = TEXT("[INVALID]"));

#if WITH_EDITOR
namespace Editor
{

ZAKAZANEUTILITIES_API UObject* TryGetEditorCounterpartObject(const UObject& Object);

}  // namespace Editor
#endif

}  // namespace Zkz
