#include "Zakazane/Component.h"

#include "Algo/AnyOf.h"
#include "ComponentUtils.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Zakazane/Blueprint.h"
#include "Zakazane/Object.h"

#if WITH_EDITOR
#include "Kismet2/BlueprintEditorUtils.h"
#include "SubobjectData.h"
#include "SubobjectDataHandle.h"
#include "SubobjectDataSubsystem.h"
#endif

namespace Zkz
{

namespace ComponentPrivate
{

USCS_Node* FindCorrespondingSCSNode(const USceneComponent& SceneComponent)
{
	if (USCS_Node* const Node = ComponentUtils::FindCorrespondingSCSNode(&SceneComponent); IsValid(Node))
	{
		return Node;
	}

	// Case if component was not found in its SCS nodes. This probably means that component is inherited from
	// another blueprint. To find corresponding node we have to go down inheritance tree.

	const USimpleConstructionScript* const SCS = ComponentUtils::GetSimpleConstructionScript(&SceneComponent);
	ZKZ_RETURN_IF_INVALID(SCS, nullptr);

	TArray<UBlueprintGeneratedClass*> BlueprintClassHierarchy;
	UBlueprint::GetBlueprintHierarchyFromClass(SCS->GetOwnerClass(), BlueprintClassHierarchy);

	for (int32 Index = 0; Index < BlueprintClassHierarchy.Num(); ++Index)
	{
		const UBlueprintGeneratedClass* const ParentBpClass = BlueprintClassHierarchy[Index];
		ZKZ_CONTINUE_IF_INVALID(ParentBpClass);
		const USimpleConstructionScript* const ParentSCS = ParentBpClass->SimpleConstructionScript;
		ZKZ_CONTINUE_IF_INVALID(ParentSCS);

		for (USCS_Node* const Node : ParentSCS->GetAllNodes())
		{
			ZKZ_CONTINUE_IF_INVALID(Node);
			if (Node->GetActualComponentTemplate(BlueprintClassHierarchy[0]) == &SceneComponent)
			{
				return Node;
			}
		}
	}

	return nullptr;
}

#if WITH_EDITOR

FSubobjectDataHandle* FindSubobjectDataHandle(TArray<FSubobjectDataHandle>& SubobjectDataHandles, const UObject& Object)
{
	return SubobjectDataHandles.FindByPredicate(
		[&Object](const FSubobjectDataHandle& SubobjectDataHandle)
		{
			const FSubobjectData* const SubobjectData = SubobjectDataHandle.GetData();
			ZKZ_RETURN_IF(SubobjectData == nullptr, false);
			return SubobjectData->GetObject() == &Object;
		});
}

#endif

}  // namespace ComponentPrivate

FString GetComponentNameNoSuffix(const FName ComponentName)
{
	return GetComponentNameNoSuffix(ComponentName.ToString());
}

FString GetComponentNameNoSuffix(FString ComponentName)
{
	ComponentName.RemoveFromEnd(UActorComponent::ComponentTemplateNameSuffix);
	return ComponentName;
}

FString GetComponentNameNoSuffix(const UActorComponent& Component)
{
	return GetComponentNameNoSuffix(Component.GetName());
}

TOptional<FString> GetActorAndComponentNameOrLabel(const UActorComponent* Component)
{
	ZKZ_RETURN_IF_INVALID(Component, NullOpt);

	const AActor* const Owner = Component->GetOwner();
	ZKZ_RETURN_IF_INVALID(Owner, NullOpt);

	return FString::Format(TEXT("{0}::{1}"), {Owner->GetActorNameOrLabel(), Component->GetName()});
}

FString GetActorAndComponentNameOrLabelOr(const UActorComponent* Component, const FString& IfInvalid)
{
	ZKZ_RETURN_IF_INVALID(Component, IfInvalid);

	return FString::Format(
		TEXT("{0}::{1}"), {GetObjectNameOrLabelOr(Component->GetOwner(), IfInvalid), Component->GetName()});
}

FComponentHierarchy::FComponentHierarchy(const AActor& InActor)
{
	// const_cast is fine, since we'll make sure not to run non-const ops because bComponentsMutable will be false
	ConstructFromActor(const_cast<AActor&>(InActor), false);
}

FComponentHierarchy::FComponentHierarchy(AActor& InActor, const bool bAllowMutableComponents)
{
	ConstructFromActor(InActor, bAllowMutableComponents);
}

FComponentHierarchy::FComponentHierarchy(
	const UActorComponent& ComponentInHierarchy, const bool bAllowMutableComponents)
{
#if WITH_EDITOR
	if (ComponentInHierarchy.HasAllFlags(RF_ArchetypeObject))
	{
		AActor* const ActorPtr = [this, &ComponentInHierarchy]() -> AActor*
		{
			UObject* const Outer = ComponentInHierarchy.GetOuter();
			ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(Outer, nullptr);

			if (AActor* const OuterActor = Cast<AActor>(Outer))
			{
				return OuterActor;
			}
			if (UClass* const OuterClass = Cast<UClass>(Outer))
			{
				return GetMutableDefault<AActor>(OuterClass);
			}

			return nullptr;
		}();
		ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(ActorPtr);

		Actor = ActorPtr;

		const TSubclassOf<AActor> ActorClass{ActorPtr->GetClass()};
		ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(ActorClass);

		ConstructHierarchyFromCDO(ActorClass);

		bComponentsMutable = bAllowMutableComponents || bComponentsMutable;
	}
	else
#endif
	{
		// If component is not an archetype, don't construct the hierarchy, we'll read the hierarchy from the instance.
		Actor = Cast<AActor>(ComponentInHierarchy.GetOuter());
		ZKZ_RETURN_IF_ENSUREALWAYS(!Actor.IsValid());
		bComponentsMutable = bAllowMutableComponents;
	}
}

const UActorComponent* FComponentHierarchy::FindParent(const UActorComponent& Child) const
{
	return InternalFindParent(Child);
}

UActorComponent* FComponentHierarchy::FindParent(const UActorComponent& Child)
{
	ZKZ_RETURN_IF_ENSUREALWAYS(!bComponentsMutable, nullptr);

	return InternalFindParent(Child);
}

const UActorComponent* FComponentHierarchy::FindComponentByName(const FName Name) const
{
	const TWeakObjectPtr<UActorComponent>* FoundWeakComp = CompsByName.Find(FName{GetComponentNameNoSuffix(Name)});

	return FoundWeakComp == nullptr ? nullptr : FoundWeakComp->Get();
}

#if WITH_EDITOR

UActorComponent* FComponentHierarchy::AddNewSubobject(
	UClass& Class,
	UActorComponent* ParentComp,
	const FName& Name,
	const Editor::EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified)
{
	using namespace ComponentPrivate;

	ZKZ_RETURN_IF_ENSUREALWAYS(!Class.IsChildOf(UActorComponent::StaticClass()), nullptr);
	ZKZ_RETURN_IF_ENSUREALWAYS(!bComponentsMutable, nullptr);

	AActor* const ActorPtr = Actor.Get();
	ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(ActorPtr, nullptr);

	USubobjectDataSubsystem* const SubobjectDataSubsystem = USubobjectDataSubsystem::Get();
	ZKZ_RETURN_IF_INVALID(SubobjectDataSubsystem, nullptr);

	TArray<FSubobjectDataHandle> SubobjectDataHandles;
	SubobjectDataSubsystem->GatherSubobjectData(ActorPtr, SubobjectDataHandles);
	ZKZ_RETURN_IF_ENSUREALWAYS(SubobjectDataHandles.IsEmpty(), nullptr);

	UBlueprint* const Blueprint = GetBlueprint(*ActorPtr);
	ZKZ_RETURN_IF_INVALID(Blueprint, nullptr);
	const UClass* const BlueprintGeneratedClass = Blueprint->GeneratedClass;
	ZKZ_RETURN_IF_INVALID(BlueprintGeneratedClass, nullptr);
	ZKZ_RETURN_IF_ENSUREALWAYS(ActorPtr != BlueprintGeneratedClass->GetDefaultObject(false), nullptr);

	const UObject* const Parent = (ParentComp == nullptr ? Cast<UObject>(ActorPtr) : ParentComp);
	ZKZ_RETURN_IF_INVALID(Parent, nullptr);
	const FSubobjectDataHandle* const ParentDataHandle = FindSubobjectDataHandle(SubobjectDataHandles, *Parent);
	ZKZ_RETURN_IF_ENSUREALWAYS(ParentDataHandle == nullptr, nullptr);

	FAddNewSubobjectParams Params;
	Params.ParentHandle = *ParentDataHandle;
	Params.NewClass = &Class;
	Params.AssetOverride = nullptr;
	Params.BlueprintContext = Blueprint;
	Params.bSkipMarkBlueprintModified = true;  // conditionally marking blueprint as modified after rename

	FText FailReason;
	const FSubobjectDataHandle NewSubobjectHandle = SubobjectDataSubsystem->AddNewSubobject(Params, FailReason);

	ZKZ_RETURN_IF(!NewSubobjectHandle.IsValid(), nullptr);

	const FSubobjectData* const NewSubobjectHandleData = NewSubobjectHandle.GetData();
	ZKZ_RETURN_IF(NewSubobjectHandleData == nullptr, nullptr);

	const UObject* const NewObject = NewSubobjectHandleData->GetObjectForBlueprint(Blueprint);

	// The const_cast here should be fine. We know the hierarchy is mutable, as we checked at the beginning of the function
	UActorComponent* const NewComp = const_cast<UActorComponent*>(Cast<UActorComponent>(NewObject));
	ZKZ_RETURN_IF_INVALID(NewComp, nullptr);
	ZKZ_RETURN_IF_ENSUREALWAYS(!NewComp->HasAllFlags(RF_ArchetypeObject), nullptr);

	CompsByParent.Emplace(ParentComp, NewComp);
	CompsByChild.Emplace(NewComp, ParentComp);

	const bool bRenameSuccessful = SubobjectDataSubsystem->RenameSubobject(NewSubobjectHandle, FText::FromName(Name));
	if (!bRenameSuccessful)
	{
		// #TODO #Buildings: would be nice to handle it more gracefully, but I don't have an idea how ATM
		const int32 NumRemoved = RemoveSubobject(*NewComp, Editor::EMarkBlueprintAsStructurallyModified::Disabled);
		ensureMsgf(NumRemoved > 0, TEXT("Failed to rename newly created subobject, then failed to remove it."));
		return nullptr;
	}

	CompsByName.Emplace(Name, NewComp);

	if (MarkBlueprintAsStructurallyModified == Editor::EMarkBlueprintAsStructurallyModified::Enabled)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}

	return NewComp;
}

int32 FComponentHierarchy::RemoveSubobject(
	const UActorComponent& Comp, const Editor::EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified)
{
	return RemoveSubobjects({&Comp}, MarkBlueprintAsStructurallyModified);
}

int32 FComponentHierarchy::RemoveSubobjects(
	const TArray<const UActorComponent*>& Comps,
	const Editor::EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified)
{
	using namespace ComponentPrivate;

	ZKZ_RETURN_IF_ENSUREALWAYS(!bComponentsMutable, 0);

	AActor* const ActorPtr = Actor.Get();
	ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(ActorPtr, 0);

	USubobjectDataSubsystem* const SubobjectDataSubsystem = USubobjectDataSubsystem::Get();
	ZKZ_RETURN_IF_INVALID(SubobjectDataSubsystem, 0);

	TArray<FSubobjectDataHandle> SubobjectDataHandles;
	SubobjectDataSubsystem->GatherSubobjectData(ActorPtr, SubobjectDataHandles);
	ZKZ_RETURN_IF_ENSUREALWAYS(SubobjectDataHandles.IsEmpty(), 0);

	UBlueprint* const Blueprint = GetBlueprint(*ActorPtr);
	ZKZ_RETURN_IF_INVALID(Blueprint, 0);
	ZKZ_RETURN_IF_ENSUREALWAYS(ActorPtr != Blueprint->GeneratedClass->GetDefaultObject(false), 0);

	const FSubobjectDataHandle* const OwnerDataHandle = FindSubobjectDataHandle(SubobjectDataHandles, *ActorPtr);
	ZKZ_RETURN_IF(OwnerDataHandle == nullptr, 0);

	const TArray<FSubobjectDataHandle> CompDataHandles = [&Comps, &SubobjectDataHandles]()
	{
		TArray<FSubobjectDataHandle> Result;
		for (const UActorComponent* const Comp : Comps)
		{
			ZKZ_CONTINUE_IF_INVALID(Comp);
			const FSubobjectDataHandle* const CompDataHandle = FindSubobjectDataHandle(SubobjectDataHandles, *Comp);
			ZKZ_CONTINUE_IF_ENSUREALWAYS(CompDataHandle == nullptr);
			Result.Emplace(*CompDataHandle);
		}
		return Result;
	}();

	const TArray<FName> CompNames = [&Comps]()
	{
		TArray<FName> Result;
		for (const UActorComponent* const Comp : Comps)
		{
			ZKZ_CONTINUE_IF_INVALID(Comp);
			Result.Emplace(GetComponentNameNoSuffix(*Comp));
		}
		return Result;
	}();

	const int32 NumRemoved = SubobjectDataSubsystem->DeleteSubobjects(*OwnerDataHandle, CompDataHandles, Blueprint);

	ZKZ_RETURN_IF(NumRemoved == 0, 0);

	for (const UActorComponent* const Comp : Comps)
	{
		const TWeakObjectPtr<UActorComponent> Parent = CompsByChild.FindAndRemoveChecked(Comp);
		CompsByParent.Remove(Parent, const_cast<UActorComponent*>(Comp));

		const TArray<TWeakObjectPtr<UActorComponent>> ReparentedChildren = [this, Comp]
		{
			TArray<TWeakObjectPtr<UActorComponent>> Result;
			for (auto It = CompsByParent.CreateKeyIterator(Comp); It; ++It)
			{
				Result.Emplace(It.Value());
				It.RemoveCurrent();
			}

			return Result;
		}();

		for (const TWeakObjectPtr<UActorComponent>& ReparentedChild : ReparentedChildren)
		{
			CompsByParent.Emplace(Parent, ReparentedChild);
			CompsByChild.FindChecked(ReparentedChild) = Parent;
		}
	}

	for (const FName CompName : CompNames)
	{
		CompsByName.Remove(CompName);
	}

	if (MarkBlueprintAsStructurallyModified == Editor::EMarkBlueprintAsStructurallyModified::Enabled)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}

	return NumRemoved;
}

#endif

bool FComponentHierarchy::ComponentsMutable() const
{
	return bComponentsMutable;
}

void FComponentHierarchy::ConstructFromActor(AActor& InActor, const bool bAllowMutableComponents)
{
	Actor = &InActor;

#if WITH_EDITOR
	if (InActor.HasAllFlags(RF_ArchetypeObject))
	{
		const TSubclassOf<AActor> ActorClass{InActor.GetClass()};
		ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(ActorClass);

		ConstructHierarchyFromCDO(ActorClass);

		bComponentsMutable = bAllowMutableComponents || bComponentsMutable;
	}
	else
#endif
	{
		// If actor is not an archetype, don't construct the hierarchy, we'll read the hierarchy from the instance.
		bComponentsMutable = bAllowMutableComponents;
	}
}

#if WITH_EDITOR
void FComponentHierarchy::ConstructHierarchyFromCDO(const TSubclassOf<AActor>& ActorClass)
{
	CompsByParent.Reset();
	CompsByChild.Reset();
	CompsByName.Reset();

	ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(ActorClass.Get());

	if (Cast<UBlueprintGeneratedClass>(ActorClass.Get()))
	{
		bComponentsMutable = true;

		// This looks strange, but unfortunately the parent data is not reliable when accessing from
		// USCS_Node->GetParentComponentTemplate. Sometimes a node returns a null parent, while it's actually
		// a child of a node. This seems to be the case in blueprint hierarchies especially. The nodes do keep
		// information about their children though, so we can construct the hierarchy by scanning parent-to-child
		// relationships first.
		TMap<const USCS_Node*, const USCS_Node*> SCSNodeParentsByChild;

		ForEachNodeInInheritanceTreeOf(
			ActorClass,
			[&SCSNodeParentsByChild](const USCS_Node& SCSNode, const UBlueprintGeneratedClass& BlueprintGeneratedClass)
			{
				for (const USCS_Node* const ChildNode : SCSNode.GetChildNodes())
				{
					SCSNodeParentsByChild.Emplace(ChildNode, &SCSNode);
				}
			});

		ForEachNodeInInheritanceTreeOf(
			ActorClass,
			[&](const USCS_Node& SCSNode, UBlueprintGeneratedClass& BlueprintGeneratedClass)
			{
				ZKZ_RETURN_IF_INVALID(SCSNode.ComponentTemplate);

				UBlueprint* const Blueprint = Cast<UBlueprint>(BlueprintGeneratedClass.ClassGeneratedBy);
				ZKZ_RETURN_IF_INVALID(Blueprint);

				UActorComponent* const Parent = [&]
				{
					const USCS_Node** const KnownParentSCSNodePtrPtr = SCSNodeParentsByChild.Find(&SCSNode);
					UActorComponent* const KnownParentComp =
						(KnownParentSCSNodePtrPtr == nullptr
							 ? nullptr
							 : (*KnownParentSCSNodePtrPtr)->GetActualComponentTemplate(&BlueprintGeneratedClass));
					UActorComponent* const ReportedParentComp = SCSNode.GetParentComponentTemplate(Blueprint);

					ensureAlways(
						KnownParentComp == nullptr || ReportedParentComp == nullptr
						|| (KnownParentComp == ReportedParentComp));

					if (ReportedParentComp != nullptr)
					{
						return ReportedParentComp;
					}

					return KnownParentComp;
				}();

				CompsByParent.Emplace(Parent, SCSNode.ComponentTemplate);
				CompsByChild.Emplace(SCSNode.ComponentTemplate, Parent);
				CompsByName.Emplace(
					GetComponentNameNoSuffix(*SCSNode.ComponentTemplate),
					SCSNode.GetActualComponentTemplate(&BlueprintGeneratedClass));
			});
	}
	else
	{
		bComponentsMutable = false;

		AActor::ForEachComponentOfActorClassDefault<UActorComponent>(
			ActorClass,
			[this](const UActorComponent* const Comp)
			{
				ZKZ_RETURN_IF_INVALID(Comp, true);

				// This const cast is fine - bComponentsMutable is false, so we're only allow const access to the stored
				// data.
				UActorComponent* const MutableComp = const_cast<UActorComponent*>(Comp);

				const USceneComponent* const SceneComp = Cast<USceneComponent>(Comp);
				USceneComponent* const Parent = IsValid(SceneComp) ? SceneComp->GetAttachParent() : nullptr;
				CompsByParent.Emplace(Parent, MutableComp);
				CompsByChild.Emplace(MutableComp, Parent);
				CompsByName.Emplace(GetComponentNameNoSuffix(*Comp), MutableComp);
				return true;
			});
	}
}
#endif

UActorComponent* FComponentHierarchy::InternalFindParent(const UActorComponent& Child) const
{
	const AActor* const ActorPtr = Actor.Get();
	ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(ActorPtr, nullptr);

#if WITH_EDITOR
	if (Actor->HasAllFlags(RF_ArchetypeObject))
	{
		const TWeakObjectPtr<UActorComponent>* const FoundParent = CompsByChild.Find(&Child);
		return FoundParent == nullptr ? nullptr : FoundParent->Get();
	}
	else
#endif
	{
		const USceneComponent* const ChildSceneComp = Cast<USceneComponent>(&Child);
		return IsValid(ChildSceneComp) ? ChildSceneComp->GetAttachParent() : nullptr;
	}
}

AActor* GetOwner(const USceneComponent& SceneComp)
{
#if WITH_EDITOR
	if (SceneComp.HasAllFlags(RF_ArchetypeObject))
	{
		const UBlueprint* const OwnerBlueprint = GetBlueprint(SceneComp);
		ZKZ_RETURN_IF_INVALID(OwnerBlueprint, {});
		ZKZ_RETURN_IF_INVALID(OwnerBlueprint->GeneratedClass, {});

		AActor* const CDO = OwnerBlueprint->GeneratedClass->GetDefaultObject<AActor>();
		ZKZ_RETURN_IF_INVALID(CDO, {});

		return CDO;
	}
#endif

	return SceneComp.GetOwner();
}

UActorComponent* FindComponentInSubtree(
	UActorComponent& RootComp, const TFunction<bool(const UActorComponent&)>& Predicate)
{
	UActorComponent* FoundComp = nullptr;

	const auto StoreIfSatisfiesPred = [&FoundComp, &Predicate](UActorComponent& Comp)
	{
		if (Predicate(Comp))
		{
			FoundComp = &Comp;
			// Component found, return false to stop traversal
			return false;
		}

		// Continue traversal
		return true;
	};

	FComponentHierarchy{RootComp}
		.ForEachComponentInSubtree<EForEachComponentRecursionType::PrefixCond>(RootComp, StoreIfSatisfiesPred);

	return FoundComp;
}

const UActorComponent* FindComponentInSubtree(
	const UActorComponent& RootComp, const TFunction<bool(const UActorComponent&)>& Predicate)
{
	return FindComponentInSubtree(const_cast<UActorComponent&>(RootComp), Predicate);
}

UActorComponent* FindComponentInSubtreeByName(UActorComponent& RootComp, const FName& Name)
{
	const FString NameNoSuffix = GetComponentNameNoSuffix(Name);

	const auto Predicate = [&NameNoSuffix](const UActorComponent& Comp)
	{ return GetComponentNameNoSuffix(Comp) == NameNoSuffix; };

	return FindComponentInSubtree(RootComp, Predicate);
}

const UActorComponent* FindComponentInSubtreeByName(const UActorComponent& RootComp, const FName& Name)
{
	return FindComponentInSubtreeByName(const_cast<UActorComponent&>(RootComp), Name);
}

bool ComponentHasAnyTag(const UActorComponent& Comp, const TArrayView<const FName> Tags)
{
	return Algo::AnyOf(Tags, [&Comp](const FName& Tag) { return Comp.ComponentHasTag(Tag); });
}

#if WITH_EDITOR
namespace Editor
{

UActorComponent* AddNewSubobject(
	UClass& Class,
	AActor& Owner,
	UActorComponent* ParentComp,
	const FName& Name,
	const EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified)
{
	return FComponentHierarchy{Owner, true}
		.AddNewSubobject(Class, ParentComp, Name, MarkBlueprintAsStructurallyModified);
}

int32 RemoveSubobject(
	AActor& Owner,
	const UActorComponent& Comp,
	const EMarkBlueprintAsStructurallyModified MarkBlueprintAsStructurallyModified)
{
	return FComponentHierarchy{Owner, true}.RemoveSubobject(Comp, MarkBlueprintAsStructurallyModified);
}

}  // namespace Editor
#endif

}  // namespace Zkz
