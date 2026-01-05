#include "Zakazane/Class.h"

#include "UObject/Class.h"
#include "UObject/Object.h"
#include "Zakazane/ReturnIfMacros.h"

namespace Zkz
{

TOptional<FString> GetClassName(const UObject* const Object)
{
	ZKZ_RETURN_IF_INVALID(Object, NullOpt);

	return GetClassName(*Object);
}

TOptional<FString> GetClassName(const UObject& Object)
{
	const UClass* const Class = Object.GetClass();
	ZKZ_RETURN_IF_INVALID(Class, NullOpt);

	return Class->GetName();
}

}  // namespace Zkz
