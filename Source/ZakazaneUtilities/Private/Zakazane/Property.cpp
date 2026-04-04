#include "Zakazane/Property.h"

#include "UObject/PropertyAccessUtil.h"
#include "Zakazane/ContinueIfMacros.h"
#include "Zakazane/ReturnIfMacros.h"

namespace Zkz::Game::Editor
{

bool MakeChangeNotify(FPropertyAccessChangeNotify& ChangeNotify, const TArray<TTuple<UObject*, FName>>& ObjectChain)
{
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

	return true;
}

void EmitPropertyChangeNotifications(
	const TArray<TTuple<UObject*, FName>>& ObjectChain,
	const bool bIdenticalValue,
	const TFunction<void()>& ChangeFunction)
{
	FPropertyAccessChangeNotify ChangeNotify;
	const bool bMadeChangeNotify = MakeChangeNotify(ChangeNotify, ObjectChain);
	ZKZ_RETURN_IF(!bMadeChangeNotify);

	PropertyAccessUtil::EmitPreChangeNotify(&ChangeNotify, bIdenticalValue);
	if (!bIdenticalValue)
	{
		ChangeFunction();
	}
	PropertyAccessUtil::EmitPostChangeNotify(&ChangeNotify, bIdenticalValue);
}

FString AppendPropertyToPathName(
	const FStringView PathName, const FStringView PropertyName, const TOptional<int32> Index)
{
	if (Index.IsSet())
	{
		return FString::Format(TEXT("{0}.{1}[{2}]"), {PathName, PropertyName, *Index});
	}

	return FString::Format(TEXT("{0}.{1}"), {PathName, PropertyName});
}

FString AppendPropertyToPathName(const FStringView PathName, const FName PropertyName, const TOptional<int32> Index)
{
	return AppendPropertyToPathName(PathName, PropertyName.ToString(), Index);
}

}  // namespace Zkz::Game::Editor
