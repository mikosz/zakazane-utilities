// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"
#include "Templates/IsInvocable.h"
#include "Templates/SubclassOf.h"
#include "Zakazane/ContinueIfMacros.h"
#include "Zakazane/ReturnIfMacros.h"

class USCS_Node;

namespace Zkz
{

namespace ComponentPrivate
{

ZAKAZANEUTILITIES_API USCS_Node* FindCorrespondingSCSNode(const USceneComponent& SceneComponent);

}  // namespace ComponentPrivate

ZAKAZANEUTILITIES_API FString GetComponentNameNoSuffix(FName ComponentName);
ZAKAZANEUTILITIES_API FString GetComponentNameNoSuffix(FString ComponentName);
ZAKAZANEUTILITIES_API FString GetComponentNameNoSuffix(const UActorComponent& Component);

/// Returns ActorNameOrLabel::ComponentName. If Component or its owner if invalid, returns NullOpt
ZAKAZANEUTILITIES_API TOptional<FString> GetActorAndComponentNameOrLabel(const UActorComponent* Component);

/// Returns ActorNameOrLabel::ComponentLabel. If Component is invalid, returns IfInvalid. If Component's owner is invalid,
/// returns IfInvalid::ComponentName.
ZAKAZANEUTILITIES_API FString
GetActorAndComponentNameOrLabelOr(const UActorComponent* Component, const FString& IfInvalid = TEXT("[INVALID]"));

#if WITH_EDITOR
namespace Editor
{

enum class EMarkBlueprintAsStructurallyModified
{
	Disabled,
	Enabled,
};

}  // namespace Editor
#endif

enum class EForEachComponentRecursionType
{
	/// Don't recurse
	NotRecursive,
	/// Calls the function for parents, then for their descendants
	Prefix,
	/// Calls the function for parents, then for their descendants. If the function returns false,
	/// stops the recursion in the current branch.
	PrefixCond,
	/// Calls the function first for the lowest components in the hierarchy
	Suffix,
};

enum class EForEachComponentAttachedActorsRecursionType
{
	NotRecursive,
	Recursive,
};

/// Scans the current component hierarchy of an actor. Works for instanced actors, blueprints and c++ classes.
/// This object may be quite large for big hierarchies for actors that are not instanced and the construction
/// of the hierarchy may take a moment, so better to construct it once and reuse it. Conversely, creation for
/// instanced actors is basically free and [almost] no offline data is stored.
class ZAKAZANEUTILITIES_API FComponentHierarchy
{
public:
	/// Constructs a component hierarchy from an actor reference. The hierarchy will be immutable.
	explicit FComponentHierarchy(const AActor& InActor);

	/// Constructs a component hierarchy from an actor reference.
	/// @see ComponentsMutable for the concept of mutability of components. The final mutability depends also on the
	///		value of the bAllowMutableComponents argument - components will not be mutable regardless of where they
	///		come from if this is false.
	explicit FComponentHierarchy(AActor& InActor, const bool bAllowMutableComponents = true);

	/// Constructs a component hierarchy from a component in hierarchy. This still constructs a full hierarchy from
	///		the containing class, not just descendants of the given component.
	/// @see ComponentsMutable for the concept of mutability of components. The final mutability depends also on the
	///		value of the bAllowMutableComponents argument - components will not be mutable regardless of where they
	///		come from if this is false.
	explicit FComponentHierarchy(
		const UActorComponent& ComponentInHierarchy, const bool bAllowMutableComponents = true);

	const UActorComponent* FindParent(const UActorComponent& Child) const;
	UActorComponent* FindParent(const UActorComponent& Child);

	/// Calls Func for each component. Order is undetermined.
	template <class FuncType, class... AdditionalArgTypes>
	void ForEachComponent(FuncType&& Func, AdditionalArgTypes&&... AdditionalArgs) const;

	template <class FuncType, class... AdditionalArgTypes>
	void ForEachComponent(
		EForEachComponentAttachedActorsRecursionType AttachedActorsRecursion,
		FuncType&& Func,
		AdditionalArgTypes&&... AdditionalArgs) const;

	/// Calls Func for each immediate child of the given component.
	template <class ArgComponentType, class FuncType, class... AdditionalArgTypes>
	void ForEachChildComponent(
		const ArgComponentType& Component, FuncType&& Func, AdditionalArgTypes&&... AdditionalArgs) const;

	/// Calls Func for each component that doesn't have a parent - that is for the scene root component as well
	/// as all non-scene components.
	template <class FuncType, class... AdditionalArgTypes>
	void ForEachRootComponent(FuncType&& Func, AdditionalArgTypes&&... AdditionalArgs) const;

	/// Calls Func for each component in hierarchy of RootComp. Order of calls depends on provided RecursionType.
	/// Components passed to Func are guaranteed to be valid.
	/// If RecursionType == PrefixCond, the given function is expected to return whether traversal should continue.
	template <EForEachComponentRecursionType RecursionType, class FuncType, class... AdditionalArgTypes>
	void ForEachComponentInSubtree(
		UActorComponent& RootComp, FuncType&& Func, AdditionalArgTypes&&... AdditionalArgs) const;

	/// Calls Func for each component in hierarchy of RootComp. Order of calls depends on provided RecursionType.
	/// Components passed to Func are guaranteed to be valid.
	/// If RecursionType == PrefixCond, the given function is expected to return whether traversal should continue.
	template <EForEachComponentRecursionType RecursionType, class FuncType, class... AdditionalArgTypes>
	void ForEachComponentInSubtree(
		const UActorComponent& RootComp, FuncType&& Func, AdditionalArgTypes&&... AdditionalArgs) const;

	const UActorComponent* FindComponentByName(FName Name) const;

#if WITH_EDITOR
	/// Adds a new subobject to an already constructed default object. Typically, this is possible only using
	/// CreateDefaultSubobject in the object's constructor.
	UActorComponent* AddNewSubobject(
		UClass& Class,
		UActorComponent* ParentComp,
		const FName& Name,
		Editor::EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified =
			Editor::EMarkBlueprintAsStructurallyModified::Enabled);

	/// Adds a new subobject to an already constructed default object. Typically, this is possible only using
	/// CreateDefaultSubobject in the object's constructor.
	template <class T>
	T* AddNewSubobject(
		UActorComponent* ParentComp,
		const FName& Name,
		Editor::EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified =
			Editor::EMarkBlueprintAsStructurallyModified::Enabled);

	/// Adds a new subobject to an already constructed default object. Typically, this is possible only using
	/// CreateDefaultSubobject in the object's constructor.
	template <class T>
	T* AddNewSubobject(
		TSubclassOf<T> Class,
		UActorComponent* ParentComp,
		const FName& Name,
		Editor::EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified =
			Editor::EMarkBlueprintAsStructurallyModified::Enabled);

	/// Removes a subobject from an already constructed default object.
	/// @returns The number of removed objects
	int32 RemoveSubobject(
		const UActorComponent& Comp,
		Editor::EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified =
			Editor::EMarkBlueprintAsStructurallyModified::Enabled);

	/// Removes multiple subobjects from an already constructed default object.
	/// @returns The number of removed objects
	int32 RemoveSubobjects(
		const TArray<const UActorComponent*>& Comps,
		Editor::EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified =
			Editor::EMarkBlueprintAsStructurallyModified::Enabled);
#endif

	/// Whether the components in the hierarchy are mutable, or const-only. If false, ForEachChildComponent will only
	/// allow functors taking a const actor component pointer. A non-const actor component pointer argument will
	/// ensure and not call the functor.
	/// This is because mutability of component pointers depends on where they come from. Components in instanced
	/// actors and components of blueprint generated classes (even inherited ones) are mutable, while default
	/// subobjects implemented in c++ are not.
	bool ComponentsMutable() const;

private:
	using FCompsByParent = TMultiMap<TWeakObjectPtr<UActorComponent>, TWeakObjectPtr<UActorComponent>>;
	using FCompsByChild = TMap<TWeakObjectPtr<UActorComponent>, TWeakObjectPtr<UActorComponent>>;
	using FCompsByName = TMap<FName, TWeakObjectPtr<UActorComponent>>;

	TWeakObjectPtr<AActor> Actor;

	FCompsByParent CompsByParent;
	FCompsByChild CompsByChild;
	FCompsByName CompsByName;

	/// @see ComponentsMutable
	bool bComponentsMutable = false;

	void ConstructFromActor(AActor& InActor, const bool bAllowMutableComponents);

#if WITH_EDITOR
	void ConstructHierarchyFromCDO(const TSubclassOf<AActor>& ActorClass);
#endif

	UActorComponent* InternalFindParent(const UActorComponent& Child) const;
};

/// This function works both for instanced components and archetypes (components in blueprints). For instanced components
/// returns owner, for archetype components returns default object of class generated by blueprint.
ZAKAZANEUTILITIES_API AActor* GetOwner(const USceneComponent& SceneComp);

/// Similar to ForEachComponentInHierarchy, but not called for root component
template <
	EForEachComponentRecursionType RecursionType,
	class SceneComponentType,
	class FuncType,
	class... AdditionalArgTypes UE_REQUIRES(
		TIsDerivedFrom<typename TDecay<SceneComponentType>::Type, USceneComponent>::Value)>
void ForEachDescendant(
	const FComponentHierarchy& ComponentHierarchy,
	SceneComponentType& ParentComp,
	FuncType&& Func,
	AdditionalArgTypes&&... AdditionalArgs)
{
	ComponentHierarchy.ForEachChildComponent(
		ParentComp,
		[&](UActorComponent& Component)
		{ ComponentHierarchy.ForEachComponentInSubtree<RecursionType>(Component, Func, AdditionalArgs...); });
}

/// Similar to ForEachComponentInHierarchy, but not called for root component
template <
	EForEachComponentRecursionType RecursionType,
	class SceneComponentType,
	class FuncType,
	class... AdditionalArgTypes UE_REQUIRES(
		TIsDerivedFrom<typename TDecay<SceneComponentType>::Type, USceneComponent>::Value)>
void ForEachDescendant(SceneComponentType& ParentComp, FuncType&& Func, AdditionalArgTypes&&... AdditionalArgs)
{
	ForEachDescendant<RecursionType>(
		FComponentHierarchy{ParentComp},
		ParentComp,
		Forward<FuncType>(Func),
		Forward<AdditionalArgTypes>(AdditionalArgs)...);
}

ZAKAZANEUTILITIES_API UActorComponent* FindComponentInSubtree(
	UActorComponent& RootComp, const TFunction<bool(const UActorComponent&)>& Predicate);
ZAKAZANEUTILITIES_API const UActorComponent* FindComponentInSubtree(
	const UActorComponent& RootComp, const TFunction<bool(const UActorComponent&)>& Predicate);

ZAKAZANEUTILITIES_API UActorComponent* FindComponentInSubtreeByName(UActorComponent& RootComp, const FName& Name);
ZAKAZANEUTILITIES_API const UActorComponent* FindComponentInSubtreeByName(
	const UActorComponent& RootComp, const FName& Name);

ZAKAZANEUTILITIES_API bool ComponentHasAnyTag(const UActorComponent& Comp, TArrayView<const FName> Tags);

#if WITH_EDITOR
namespace Editor
{

/// Adds a new subobject to an already constructed default object. Typically, this is possible only using
/// CreateDefaultSubobject in the object's constructor.
ZAKAZANEUTILITIES_API UActorComponent* AddNewSubobject(
	UClass& Class,
	AActor& Owner,
	UActorComponent* ParentComp,
	const FName& Name,
	EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified =
		EMarkBlueprintAsStructurallyModified::Enabled);

/// Removes a subobject from an already constructed default object.
/// @returns The number of removed objects
ZAKAZANEUTILITIES_API int32 RemoveSubobject(
	AActor& Owner,
	const UActorComponent& Comp,
	EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified =
		EMarkBlueprintAsStructurallyModified::Enabled);

}  // namespace Editor
#endif

}  // namespace Zkz

// -- Template implementations

namespace Zkz
{

template <class FuncType, class... AdditionalArgTypes>
void FComponentHierarchy::ForEachComponent(FuncType&& Func, AdditionalArgTypes&&... AdditionalArgs) const
{
	constexpr bool bIsConstInvocable = TIsInvocable<FuncType, const UActorComponent&, AdditionalArgTypes...>::Value;
	constexpr bool bIsNonConstInvocable = TIsInvocable<FuncType, UActorComponent&, AdditionalArgTypes...>::Value;
	static_assert(
		bIsConstInvocable || bIsNonConstInvocable,
		"Invalid functor signature. Expected functor taking a [const] UActorComponent, AdditionalArgTypes...");

	ZKZ_RETURN_IF_ENSUREALWAYSMSGF(
		!bIsConstInvocable && !ComponentsMutable(),
		"ForEachChildComponent called with functor taking non-const components for an immutable hierarchy");

	AActor* const ActorPtr = Actor.Get();
	ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(ActorPtr);

	if (ActorPtr->HasAllFlags(RF_ArchetypeObject))
	{
		for (const auto& [WeakParent, WeakComp] : CompsByParent)
		{
			UActorComponent* const Comp = WeakComp.Get();
			ZKZ_CONTINUE_IF_INVALID(Comp);

			Func(*Comp, AdditionalArgs...);
		}
	}
	else
	{
		ActorPtr->ForEachComponent(
			false,
			[&](UActorComponent* const Comp)
			{
				ZKZ_RETURN_IF_INVALID(Comp);
				Func(*Comp, AdditionalArgs...);
			});
	}
}

template <class FuncType, class... AdditionalArgTypes>
void FComponentHierarchy::ForEachComponent(
	EForEachComponentAttachedActorsRecursionType AttachedActorsRecursion,
	FuncType&& Func,
	AdditionalArgTypes&&... AdditionalArgs) const
{
	ForEachComponent(Func, AdditionalArgs...);

	if (AttachedActorsRecursion == EForEachComponentAttachedActorsRecursionType::Recursive)
	{
		AActor* const ActorPtr = Actor.Get();
		ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(ActorPtr);

		ActorPtr->ForEachAttachedActors(
			[&](AActor* const AttachedActor)
			{
				ZKZ_RETURN_IF_INVALID(AttachedActor, true);
				FComponentHierarchy{*AttachedActor, bComponentsMutable}
					.ForEachComponent(AttachedActorsRecursion, Func, AdditionalArgs...);

				return true;
			});
	}
}

template <class ArgComponentType, class FuncType, class... AdditionalArgTypes>
void FComponentHierarchy::ForEachChildComponent(
	const ArgComponentType& Component, FuncType&& Func, AdditionalArgTypes&&... AdditionalArgs) const
{
	constexpr bool bIsConstInvocable = TIsInvocable<FuncType, const UActorComponent&, AdditionalArgTypes...>::Value;
	constexpr bool bIsNonConstInvocable = TIsInvocable<FuncType, UActorComponent&, AdditionalArgTypes...>::Value;
	static_assert(
		bIsConstInvocable || bIsNonConstInvocable,
		"Invalid functor signature. Expected functor taking a [const] UActorComponent, AdditionalArgTypes...");

	ZKZ_RETURN_IF_ENSUREALWAYSMSGF(
		!bIsConstInvocable && !ComponentsMutable(),
		"ForEachChildComponent called with functor taking non-const components for an immutable hierarchy");

	if (Component.HasAllFlags(RF_ArchetypeObject))
	{
		for (auto It = CompsByParent.CreateConstKeyIterator(&Component); It; ++It)
		{
			UActorComponent* const CompPtr = It->Value.Get();
			ZKZ_CONTINUE_IF_INVALID(CompPtr);
			// #TODO #Property: is this ok? Using Forward would probably move from rvalue references, which would cause
			// following calls to potentially receive a different argument, while not forwarding causes lvalue references
			// to be copied? Also see Property.h and other ::Invoke calls here
			::Invoke(Func, *CompPtr, AdditionalArgs...);
		}
	}
	else
	{
		auto* const SceneComp = Cast<USceneComponent>(&Component);
		ZKZ_RETURN_IF_INVALID(SceneComp);

		const int32 NumChildrenComponents = SceneComp->GetNumChildrenComponents();
		for (int32 ChildIdx = 0; ChildIdx < NumChildrenComponents; ++ChildIdx)
		{
			USceneComponent* const ChildComp = SceneComp->GetChildComponent(ChildIdx);
			ZKZ_CONTINUE_IF_INVALID(ChildComp);
			::Invoke(Func, *ChildComp, AdditionalArgs...);
		}
	}
}

template <class FuncType, class... AdditionalArgTypes>
void FComponentHierarchy::ForEachRootComponent(FuncType&& Func, AdditionalArgTypes&&... AdditionalArgs) const
{
	constexpr bool bIsConstInvocable = TIsInvocable<FuncType, const UActorComponent&, AdditionalArgTypes...>::Value;
	constexpr bool bIsNonConstInvocable = TIsInvocable<FuncType, UActorComponent&, AdditionalArgTypes...>::Value;
	static_assert(
		bIsConstInvocable || bIsNonConstInvocable,
		"Invalid functor signature. Expected functor taking a [const] UActorComponent, AdditionalArgTypes...");

	ZKZ_RETURN_IF_ENSUREALWAYSMSGF(
		!bIsConstInvocable && !ComponentsMutable(),
		"ForEachRootComponent called with functor taking non-const components for an immutable hierarchy");

	AActor* const ActorPtr = Actor.Get();
	ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(ActorPtr);

	if (Actor->HasAllFlags(RF_ArchetypeObject))
	{
		for (auto It = CompsByParent.CreateConstKeyIterator(nullptr); It; ++It)
		{
			UActorComponent* const CompPtr = It->Value.Get();
			ZKZ_CONTINUE_IF_INVALID(CompPtr);
			::Invoke(Func, *CompPtr, AdditionalArgs...);
		}
	}
	else
	{
		Actor->ForEachComponent(
			false,
			[&](UActorComponent* const Comp)
			{
				if (USceneComponent* const SceneComp = Cast<USceneComponent>(Comp))
				{
					if (SceneComp->GetAttachParent() == nullptr)
					{
						Func(*SceneComp, AdditionalArgs...);
					}
				}
				else
				{
					Func(*Comp, AdditionalArgs...);
				}
			});
	}
}

template <EForEachComponentRecursionType RecursionType, class FuncType, class... AdditionalArgTypes>
void FComponentHierarchy::ForEachComponentInSubtree(
	UActorComponent& RootComp, FuncType&& Func, AdditionalArgTypes&&... AdditionalArgs) const
{
	using namespace ComponentPrivate;

	constexpr bool bIsConstInvocable = TIsInvocable<FuncType, const UActorComponent&, AdditionalArgTypes...>::Value;
	constexpr bool bIsNonConstInvocable = TIsInvocable<FuncType, UActorComponent&, AdditionalArgTypes...>::Value;
	static_assert(
		bIsConstInvocable || bIsNonConstInvocable,
		"Invalid functor signature. Expected functor taking a [const] UActorComponent, AdditionalArgTypes...");

	// If Suffix - we'll call for root at end of function. Otherwise, call now (non-recursive will also call at this point).
	if constexpr (RecursionType != EForEachComponentRecursionType::Suffix)
	{
		if constexpr (RecursionType == EForEachComponentRecursionType::PrefixCond)
		{
			static_assert(
				std::is_convertible_v<TInvokeResult_T<FuncType, USceneComponent&, AdditionalArgTypes...>, bool>,
				"PrefixCond recursion type expects functor to return a type convertible to bool");

			const bool bContinue = ::Invoke(Func, RootComp, AdditionalArgs...);
			if (!bContinue)
			{
				return;
			}
		}
		else
		{
			::Invoke(Func, RootComp, AdditionalArgs...);

			if (RecursionType == EForEachComponentRecursionType::NotRecursive)
			{
				return;
			}
		}
	}

	// Recursively call for children.
	using ChildComponentType = std::conditional_t<bIsConstInvocable, const UActorComponent, UActorComponent>;
	ForEachChildComponent(
		RootComp,
		[&](ChildComponentType& ChildComponent)
		{ ForEachComponentInSubtree<RecursionType>(ChildComponent, Func, AdditionalArgs...); });

	// If Suffix - call function for root now.
	if constexpr (RecursionType == EForEachComponentRecursionType::Suffix)
	{
		::Invoke(Func, RootComp, AdditionalArgs...);
	}
}

template <EForEachComponentRecursionType RecursionType, class FuncType, class... AdditionalArgTypes>
void FComponentHierarchy::ForEachComponentInSubtree(
	const UActorComponent& RootComp, FuncType&& Func, AdditionalArgTypes&&... AdditionalArgs) const
{
	constexpr bool bIsConstInvocable = TIsInvocable<FuncType, const UActorComponent&, AdditionalArgTypes...>::Value;
	static_assert(
		bIsConstInvocable,
		"Invalid functor signature. Expected functor taking a const UActorComponent, AdditionalArgTypes...");

	ForEachComponentInSubtree<RecursionType>(
		const_cast<UActorComponent&>(RootComp),
		Forward<FuncType>(Func),
		Forward<AdditionalArgTypes>(AdditionalArgs)...);
}

#if WITH_EDITOR
template <class T>
T* FComponentHierarchy::AddNewSubobject(
	UActorComponent* ParentComp,
	const FName& Name,
	Editor::EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified)
{
	TSubclassOf<T> Class = T::StaticClass();
	ZKZ_RETURN_IF_INVALID(Class, nullptr);

	return AddNewSubobject<T>(*Class, ParentComp, Name, MarkBlueprintAsStructurallyModified);
}

template <class T>
T* FComponentHierarchy::AddNewSubobject(
	TSubclassOf<T> Class,
	UActorComponent* ParentComp,
	const FName& Name,
	Editor::EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified)
{
	ZKZ_RETURN_IF_INVALID(Class, nullptr);
	return Cast<T>(AddNewSubobject(*Class.Get(), ParentComp, Name, MarkBlueprintAsStructurallyModified));
}

#endif

}  // namespace Zkz