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

/// Tries to find the editor counterpart of a runtime object.
ZAKAZANEUTILITIES_API UObject* TryGetEditorCounterpartObject(const UObject& Object);

template <class ObjectType>
	requires std::is_base_of_v<UObject, ObjectType>
ObjectType* TryGetEditorCounterpartObject(const ObjectType& Object)
{
	return Cast<ObjectType>(TryGetEditorCounterpartObject(static_cast<const UObject&>(Object)));
}

}  // namespace Editor
#endif

}  // namespace Zkz
