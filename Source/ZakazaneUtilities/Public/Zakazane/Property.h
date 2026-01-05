// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Algo/Transform.h"
#include "Containers/Deque.h"
#include "Templates/Invoke.h"
#include "Templates/IsInvocable.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "UObject/PropertyAccessUtil.h"

namespace Zkz
{

/// Utility function for initializing FPropertyAccessChangeNotify objects. The user can use it to call
///	PropertyAccessUtil::EmitPreChangeNotify / EmitPostChangeNotify. Alternatively you can call
///	EmitPropertyChangeNotifications that does all this in one go.
///
/// @param ObjectChain a list of object / PropertyName pairs from outermost to innermost. Doesn't support structs at the moment.
ZAKAZANEUTILITIES_API bool MakeChangeNotify(
	FPropertyAccessChangeNotify& ChangeNotify, TArrayView<const TTuple<UObject*, FName>> ObjectChain);

/// Calls pre change notify -> ChangeFunction -> post change notify. When dealing with multiple objects / properties,
/// use MakeChangeNotify and call all EmitPreChangeNotify / EmitPostChangeNotify together before and after you modify
/// the properties. Otherwise, calling a single post change notify function may reconstruct and invalidate other modified
/// objects.
///
/// @param ObjectChain a list of object / PropertyName pairs from outermost to innermost. Doesn't support structs at the moment.
/// @param bIdenticalValue whether the value being set is equal to the previous value
/// @param ChangeFunction function that performs the actual modification of the property value
ZAKAZANEUTILITIES_API void EmitPropertyChangeNotifications(
	const TArray<TTuple<UObject*, FName>>& ObjectChain, bool bIdenticalValue, const TFunction<void()>& ChangeFunction);

/// Calls pre change notify -> ChangeFunction -> post change notify. When dealing with multiple objects / properties,
/// call all EmitPreChangeNotify / EmitPostChangeNotify together before and after you modify the properties. Otherwise,
/// calling a single post change notify function may reconstruct and invalidate other modified objects.
///
/// @param ChangeNotify a change notify object constructed manually or via MakeChangeNotify
/// @param bIdenticalValue whether the value being set is equal to the previous value
/// @param ChangeFunction function that performs the actual modification of the property value
ZAKAZANEUTILITIES_API void EmitPropertyChangeNotifications(
	const FPropertyAccessChangeNotify& ChangeNotify, bool bIdenticalValue, const TFunction<void()>& ChangeFunction);

/// Modify the given object and propagate the changes to its instances if it's an archetype.
/// Modify() will only be called on the ModifiedObject, but not the instances. This is because there's no need
/// to save the change. If the object gets re-instanced in the future, it will load the value from the archetype.
///
/// NOTE: This function does not emit property changed notifications. Add call to EmitPropertyChangeNotifications for
/// this to work. Don't add these calls inside the lambda but rather around the call to this function,
/// otherwise instances may be destroyed before we're able to update them.
///
/// Collecting the existing instances is based on FPropertyNode::PropagatePropertyChange, so if anything doesn't work
/// consult how it's solved in that function. That function contains complexities regarding objects in containers and
/// default subobjects of archetypes, which I didn't find useful for the time being, so I just skipped it.
template <
	class ObjectType,
	class FunctionType,
	class... AdditionalArgTypes UE_REQUIRES(std::is_base_of_v<UObject, ObjectType>)>
void ModifyAndPropagateChange(
	ObjectType& ModifiedObject, FunctionType&& ChangeFunction, AdditionalArgTypes&&... AdditionalArgs)
{
	static_assert(
		TIsInvocable<FunctionType, ObjectType&, AdditionalArgTypes...>::Value,
		"Expected ChangeFunction of type void(ObjectType&, AdditionalArgs...)");

	TArray<TNonNullPtr<ObjectType>> ArchetypeInstances;
	TDeque<TNonNullPtr<ObjectType>> ObjectsToChange;
	ObjectType* Object = &ModifiedObject;

	if (Object->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		// Object is a default subobject, collect all instances.
		TArray<UObject*> ArchetypeInstanceObjects;
		Object->GetArchetypeInstances(ArchetypeInstanceObjects);
		Algo::TransformIf(
			ArchetypeInstanceObjects,
			ArchetypeInstances,
			[](const UObject* const InObject) { return IsValid(InObject) && InObject->IsA<ObjectType>(); },
			[](UObject* const InObject) { return Cast<ObjectType>(InObject); });
	}

	ObjectsToChange.EmplaceLast(Object);

	while (!ObjectsToChange.IsEmpty())
	{
		const TNonNullPtr<ObjectType> ObjectToChange = ObjectsToChange[0];
		ObjectsToChange.PopFirst();

		// #TODO #Property: is this ok? Using Forward would probably move from rvalue references, which would cause
		// following calls to potentially receive a different argument, while not forwarding causes lvalue references
		// to be copied? Also see Component.h
		::Invoke(ChangeFunction, *ObjectToChange, AdditionalArgs...);

		for (auto ArchetypeInstanceIt = ArchetypeInstances.CreateIterator(); ArchetypeInstanceIt; ++ArchetypeInstanceIt)
		{
			TNonNullPtr<ObjectType> ArchetypeInstance = *ArchetypeInstanceIt;

			if (ArchetypeInstance->GetArchetype() == ObjectToChange)
			{
				ObjectsToChange.EmplaceLast(ArchetypeInstance);
				ArchetypeInstanceIt.RemoveCurrent();
			}
		}
	}

	ModifiedObject.Modify();
}

}  // namespace Zkz
