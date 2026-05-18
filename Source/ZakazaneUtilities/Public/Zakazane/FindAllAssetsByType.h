// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ReturnIfMacros.h"

namespace Zkz
{

/// Calls Func for each asset of given class (and its children).
template <class FuncType>
void ForEachAssetByType(const UClass* Class, FuncType Func)
{
	const FAssetRegistryModule* AssetRegistryModule =
		FModuleManager::LoadModulePtr<FAssetRegistryModule>("AssetRegistry");
	ZKZ_RETURN_IF(AssetRegistryModule == nullptr);

	FARFilter Filter;
	Filter.ClassPaths.Emplace(Class->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.bIncludeOnlyOnDiskAssets = false;

	AssetRegistryModule->Get().EnumerateAssets(
		Filter,
		[&Func](const FAssetData& AssetData)
		{
			UObject* Asset = AssetData.GetAsset();
			ZKZ_RETURN_IF_INVALID(Asset, true);

			return Invoke(Func, Asset);
		});
}

/// Calls Func for each asset of type T (and its children).
template <class T, class FuncType>
void ForEachAssetByType(FuncType Func)
{
	const UClass* Class = StaticClass<T>();
	ZKZ_RETURN_IF_INVALID(Class);

	ForEachAssetByType(Class, Func);
}

/// Finds and returns all assets of given class (and its children).
template <class ContainerType>
void FindAllAssetsByType(UClass* Class, ContainerType& Container)
{
	ZKZ_RETURN_IF_INVALID(Class);

	ForEachAssetByType(
		Class,
		[&Container](UObject* Object)
		{
			ZKZ_RETURN_IF_INVALID(Object, true);
			Container.Emplace(Object);

			return true;
		});
}

/// Finds and returns all assets of type T (and its children).
template <class T, class ContainerType>
void FindAllAssetsByType(ContainerType& Container)
{
	ForEachAssetByType<T>(
		[&Container](UObject* Object)
		{
			T* CastObject = Cast<T>(Object);
			ZKZ_RETURN_IF_INVALID(CastObject, true);
			Container.Emplace(CastObject);

			return true;
		});
}

}  // namespace Zkz
