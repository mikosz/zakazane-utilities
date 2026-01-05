#include "Zakazane/Property.h"

#include "UObject/PropertyAccessUtil.h"
#include "Zakazane/ContinueIfMacros.h"
#include "Zakazane/ReturnIfMacros.h"

namespace Zkz
{

bool MakeChangeNotify(FPropertyAccessChangeNotify& ChangeNotify, const TArrayView<const TTuple<UObject*, FName>> ObjectChain)
{
#if WITH_EDITOR
	ZKZ_RETURN_IF_ENSURE(ObjectChain.IsEmpty(), false);

	ChangeNotify.ChangedObject = ObjectChain.Last().Get<UObject*>();
	ChangeNotify.ChangeType = EPropertyChangeType::ValueSet;

	for (const auto& [Object, PropertyName] : ObjectChain)
	{
		ZKZ_CONTINUE_IF_INVALID_ENSURE(Object);

		const UClass* const Class = Object->GetClass();
		ZKZ_RETURN_IF_INVALID_ENSURE(Class, false);

		FProperty* const Property = Class->FindPropertyByName(PropertyName);
		ZKZ_RETURN_IF_ENSURE(Property == nullptr, false);

		ChangeNotify.ChangedPropertyChain.AddHead(Property);
		ChangeNotify.ChangedPropertyChain.SetActivePropertyNode(Property);
		ChangeNotify.ChangedPropertyChain.SetActiveMemberPropertyNode(Property);
	}
#endif

	return true;
}

void EmitPropertyChangeNotifications(
	const TArray<TTuple<UObject*, FName>>& ObjectChain,
	const bool bIdenticalValue,
	const TFunction<void()>& ChangeFunction)
{
#if WITH_EDITOR
	FPropertyAccessChangeNotify ChangeNotify;
	const bool bMadeChangeNotify = MakeChangeNotify(ChangeNotify, ObjectChain);
	ZKZ_RETURN_IF(!bMadeChangeNotify);

	PropertyAccessUtil::EmitPreChangeNotify(&ChangeNotify, bIdenticalValue);
	if (!bIdenticalValue)
	{
#endif
		
		ChangeFunction();

#if WITH_EDITOR
	}
	PropertyAccessUtil::EmitPostChangeNotify(&ChangeNotify, bIdenticalValue);
#endif
}

void EmitPropertyChangeNotifications(
	const FPropertyAccessChangeNotify& ChangeNotify, bool bIdenticalValue, const TFunction<void()>& ChangeFunction)
{
#if WITH_EDITOR
	PropertyAccessUtil::EmitPreChangeNotify(&ChangeNotify, bIdenticalValue);
	if (!bIdenticalValue)
	{
#endif
		
		ChangeFunction();

#if WITH_EDITOR
	}
	PropertyAccessUtil::EmitPostChangeNotify(&ChangeNotify, bIdenticalValue);
#endif
}

}  // namespace Zkz
