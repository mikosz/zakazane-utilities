// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Commandlets/Commandlet.h"

#include "ZkzDataValidationCommandlet.generated.h"

/// Zkz version of DataValidationCommandlet. Most code is from mentioned class (changes marked with Begin/End Zakazane Changes.
/// Available Switches:
/// -AssetType=AnyType - Only assets of type AnyType and children will be validated
/// -IncludeEngine - Engine files will also be validated
/// -IncludeOnlyDiskAssets - Only disk gathered data will be validated
/// -IncludeNeverCookDirectories - Files from Project's Settings Never Cook Directories will be validated
/// -IgnorePaths="/Path1/,/Path2/" - Files from provided paths won't be validated
UCLASS()
class ZAKAZANEVALIDATION_API UZkzDataValidationCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	// ~ UCommandlet
	/// Override needed to call function which is not virtual in UDataValidationCommandlet
	virtual int32 Main(const FString& FullCommandLine) override;
	// ~ UCommandlet

	/// Additional logic: Ignoring provided Paths and/or NeverCookDirectories
	bool ValidateData(const FString& FullCommandLine) const;

private:
	/// Altered Behavior from original code:
	/// If AssetType switch was found but nothing was specified we skip validation instead of validating everything
	/// Returns false if Failed
	bool TryBuildAssetDataList(
		const FString& FullCommandLine,
		const bool bIncludeOnlyOnDiskAssets,
		TArray<FAssetData>& OutAssetDataList) const;

	TSet<FString> BuildSelectPaths(const FString& FullCommandLine) const;

	/// Based on switches IgnorePaths are built
	TSet<FString> BuildIgnorePaths(const FString& FullCommandLine, const bool bIncludeNeverCookDirectories) const;

	/// Removes Assets that shouldn't be validated from validation list
	/// Altered Behavior from original code:
	/// Removal of IgnorePaths Assets, Before removing Engine assets
	void FilterAssetsToValidate(
		const bool bIncludeEngine,
		TArray<FAssetData>& AssetDataList,
		const TSet<FString>& SelectPaths,
		const TSet<FString>& IgnorePaths) const;

	void HandleBlueprintValidators(const TArray<FAssetData>& AssetDataList) const;

	void RunValidation(const TArray<FAssetData>& AssetDataList) const;
};
