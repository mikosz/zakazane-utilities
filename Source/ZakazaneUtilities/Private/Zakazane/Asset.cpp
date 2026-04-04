// Copyright ZAKAZANE Studio. All Rights Reserved.

#include "Zakazane/Asset.h"

#include "Algo/Find.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"

DEFINE_LOG_CATEGORY(LogZkzAsset);

namespace Zkz
{

TArray<FAssetData> FindAssetsByClass(
	const UClass& AssetClass, TArray<FName> PackagePaths, const bool bIncludeSubclasses)
{
	const IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();

	FARFilter Filter;
	Filter.ClassPaths.Emplace(AssetClass.GetClassPathName());
	Filter.bRecursiveClasses = bIncludeSubclasses;

	Filter.PackagePaths = MoveTemp(PackagePaths);
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDatas;
	AssetRegistry.GetAssets(Filter, AssetDatas);

	return AssetDatas;
}

UObject* FindAssetByName(const FName AssetName, const UClass& AssetClass, TArray<FName> PackagePaths)
{
	const TArray<FAssetData> AssetDatas = FindAssetsByClass(AssetClass, MoveTemp(PackagePaths));
	const FAssetData* const AssetData = Algo::FindBy(AssetDatas, AssetName, &FAssetData::AssetName);
	ZKZ_RETURN_IF(AssetData == nullptr, nullptr);

	return AssetData->GetAsset();
}

}  // namespace Zkz
