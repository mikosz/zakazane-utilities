// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Misc/Optional.h"
#include "Optional.h"
#include "ReturnIfMacros.h"

#include <type_traits>

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

/// Convenience function safely doing T::StaticClass()->GetName() with pointer checks
template <class T>
	requires std::is_base_of_v<UObject, T>
TOptional<FName> GetClassFName()
{
	const auto* const Class = T::StaticClass();
	ZKZ_RETURN_IF_INVALID(Class, NullOpt);
	return Class->GetFName();
}

template <class T>
	requires std::is_base_of_v<UObject, T>
TOptional<FString> GetClassName()
{
	return Optional::Transform(GetClassFName<T>(), &FName::ToString);
}

}  // namespace Zkz
