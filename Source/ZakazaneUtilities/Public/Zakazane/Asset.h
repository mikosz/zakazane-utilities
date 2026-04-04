// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "AssetRegistry/AssetData.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Zakazane/Future.h"
#include "Zakazane/ReturnIfMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogZkzAsset, Log, All);

namespace Zkz
{

ZAKAZANEUTILITIES_API TArray<FAssetData> FindAssetsByClass(
	const UClass& AssetClass, TArray<FName> PackagePaths = {}, bool bIncludeSubclasses = false);

template <class AssetType UE_REQUIRES(std::is_base_of_v<UObject, AssetType>)>
TArray<FAssetData> FindAssetsByClass(TArray<FName> PackagePaths = {}, bool bIncludeSubclasses = false);

/// Finds an asset with the given name, of the given class. Note that this operation first finds all assets of
/// the given class and then looks for the asset with the given name. This is an expensive operation, so if you
/// need to retrieve multiple assets, better to call FindAssetsByClass and then find the ones with the names
/// you need. Note that the cost of this call can be alleviated by providing PackagePath(s) containing relatively
/// few assets.
ZAKAZANEUTILITIES_API UObject* FindAssetByName(
	FName AssetName, const UClass& AssetClass, TArray<FName> PackagePaths = {});

/// Finds an asset with the given name, of the given type. Note that this operation first finds all assets of
/// the given class and then looks for the asset with the given name. This is an expensive operation, so if you
/// need to retrieve multiple assets, better to call FindAssetsByClass and then find the ones with the names
/// you need. Note that the cost of this call can be alleviated by providing PackagePath(s) containing relatively
/// few assets.
template <class AssetType UE_REQUIRES(std::is_base_of_v<UObject, AssetType>)>
AssetType* FindAssetByName(FName AssetName, TArray<FName> PackagePaths = {});

}  // namespace Zkz

// -- implementations

namespace Zkz
{

namespace Private
{

template <class ReturnType, class SoftPtrType>
TCancelableFuture<ReturnType> LoadAsynchronously(const SoftPtrType& SoftPointer)
{
	TSharedRef<TScopedPromise<ReturnType>> Promise = MakeShared<TScopedPromise<ReturnType>>();

	const FStreamableDelegate StreamableDelegate =
		FStreamableDelegate::CreateLambda([SoftPointer, Promise] { Promise->SetValue(SoftPointer.Get()); });

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	const TSharedPtr<FStreamableHandle> StreamableHandle =
		StreamableManager.RequestAsyncLoad(MakeArrayView(&SoftPointer.ToSoftObjectPath(), 1), StreamableDelegate);

	if (!StreamableHandle.IsValid())
	{
		Promise->SetValue(nullptr);
	}
	else
	{
		StreamableHandle->BindCancelDelegate(StreamableDelegate);
	}

	return Promise->GetFuture();
}

}  // namespace Private

template <class AssetType UE_REQUIRES(std::is_base_of_v<UObject, AssetType>)>
TArray<FAssetData> FindAssetsByClass(TArray<FName> PackagePaths, const bool bIncludeSubclasses)
{
	const UClass* const Class = AssetType::StaticClass();
	ZKZ_RETURN_IF_INVALID(Class, {});
	return FindAssetsByClass(*Class, MoveTemp(PackagePaths), bIncludeSubclasses);
}

template <class AssetType UE_REQUIRES(std::is_base_of_v<UObject, AssetType>)>
AssetType* FindAssetByName(FName AssetName, TArray<FName> PackagePaths)
{
	const UClass* const Class = AssetType::StaticClass();
	ZKZ_RETURN_IF_INVALID(Class, nullptr);
	return Cast<AssetType>(FindAssetByName(AssetName, *Class, MoveTemp(PackagePaths)));
}

template <class T>
TCancelableFuture<T*> LoadAsynchronously(const TSoftObjectPtr<T>& SoftObject)
{
	return Private::LoadAsynchronously<T*>(SoftObject);
}

template <class T>
TCancelableFuture<TSubclassOf<T>> LoadAsynchronously(const TSoftClassPtr<T>& SoftClass)
{
	return Private::LoadAsynchronously<TSubclassOf<T>>(SoftClass);
}

}  // namespace Zkz
