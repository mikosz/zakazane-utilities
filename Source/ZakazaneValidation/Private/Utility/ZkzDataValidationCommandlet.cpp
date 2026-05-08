// Copyright ZAKAZANE Studio. All Rights Reserved.

#include "ZkzDataValidationCommandlet.h"

#include "Algo/AnyOf.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/BlueprintSupport.h"
#include "Editor.h"
#include "EditorUtilityBlueprint.h"
#include "EditorValidatorSubsystem.h"
#include "Zakazane/ContinueIfMacros.h"
#include "Zakazane/ReturnIfMacros.h"
#include "ZkzAssetValidationUtils.h"
#include "ZkzValidationLogCategory.h"

namespace ZkzDataValidationCommandletPrivate
{

TSet<FString> ParsePaths(const FString& FullCommandLine, const FString& SwitchName)
{
	TSet<FString> Paths;

	FString IgnorePathsArgs;
	if (FParse::Value(*FullCommandLine, *SwitchName, IgnorePathsArgs, false))
	{
		TArray<FString> ParsedPaths;
		IgnorePathsArgs.ParseIntoArray(ParsedPaths, TEXT(","));

		for (const FString& Path : ParsedPaths)
		{
			FString CleanPath = Path.TrimStartAndEnd();
			ZKZ_CONTINUE_IF(CleanPath.IsEmpty());
			Paths.Emplace(CleanPath);
		}
	}

	return Paths;
}

}  // namespace ZkzDataValidationCommandletPrivate

int32 UZkzDataValidationCommandlet::Main(const FString& FullCommandLine)
{
	// This commandlet won't work properly when used outside an editor executable
	// Because ValidatorSubsystem is UEditorSubsystem
	check(GEditor);

	UE_LOG(
		LogZkzValidation,
		Log,
		TEXT("--------------------------------------------------------------------------------------------"));
	UE_LOG(LogZkzValidation, Log, TEXT("Running ZkzDataValidation Commandlet"));

	// validate data
	if (!ValidateData(FullCommandLine))
	{
		UE_LOG(LogZkzValidation, Warning, TEXT("Errors occurred while validating data"));
		return 2;  // return something other than 1 for error since the engine will return 1 if any other system (possibly unrelated) logged errors during execution.
	}

	UE_LOG(LogZkzValidation, Log, TEXT("Successfully finished running ZkzDataValidation Commandlet"));
	UE_LOG(
		LogZkzValidation,
		Log,
		TEXT("--------------------------------------------------------------------------------------------"));
	return 0;
}

bool UZkzDataValidationCommandlet::ValidateData(const FString& FullCommandLine) const
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> Params;
	ParseCommandLine(*FullCommandLine, Tokens, Switches, Params);

	const bool bIncludeEngine = Switches.Contains(TEXT("IncludeEngine"));
	const bool bIncludeOnlyOnDiskAssets = Switches.Contains(TEXT("IncludeOnlyOnDiskAssets"));
	const bool bIncludeNeverCookDirectories = Switches.Contains(TEXT("IncludeNeverCookDirectories"));

	// Begin Zakazane Changes. Check functions' comments for more details
	TArray<FAssetData> AssetDataList;
	const bool bAssetListBuilt = TryBuildAssetDataList(FullCommandLine, bIncludeOnlyOnDiskAssets, AssetDataList);
	ZKZ_RETURN_IF(!bAssetListBuilt, false);

	if (AssetDataList.IsEmpty())
	{
		UE_LOG(LogZkzValidation, Warning, TEXT("No assets found to validate with the current filters."));
		return true;
	}

	const TSet<FString> SelectPaths = BuildSelectPaths(FullCommandLine);
	const TSet<FString> IgnorePaths = BuildIgnorePaths(FullCommandLine, bIncludeNeverCookDirectories);

	FilterAssetsToValidate(bIncludeEngine, AssetDataList, SelectPaths, IgnorePaths);
	// End Zakazane Changes

	HandleBlueprintValidators(AssetDataList);

	RunValidation(AssetDataList);

	return true;
}

bool UZkzDataValidationCommandlet::TryBuildAssetDataList(
	const FString& FullCommandLine, const bool bIncludeOnlyOnDiskAssets, TArray<FAssetData>& OutAssetDataList) const
{
	const FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	FString AssetTypeString;
	if (!FParse::Value(*FullCommandLine, TEXT("AssetType="), AssetTypeString))
	{
		AssetRegistry.GetAllAssets(OutAssetDataList, bIncludeOnlyOnDiskAssets);
	}
	else
	{
		if (AssetTypeString.IsEmpty())
		{
			UE_LOG(LogZkzValidation, Error, TEXT("AssetType switch was used but the AssetType was not provided."));
			return false;
		}

		if (FPackageName::IsShortPackageName(AssetTypeString))
		{
			const UClass* const Class =
				FindFirstObject<UClass>(*AssetTypeString, EFindFirstObjectOptions::EnsureIfAmbiguous);
			if (IsValid(Class))
			{
				AssetTypeString = Class->GetPathName();
			}
			else
			{
				UE_LOG(
					LogZkzValidation,
					Error,
					TEXT("Unable to resolve class path name given short name: \"%s\""),
					*AssetTypeString);
				return false;
			}
		}

		FARFilter Filter;
		Filter.ClassPaths.Add(FTopLevelAssetPath(AssetTypeString));
		Filter.bRecursiveClasses = true;
		Filter.bIncludeOnlyOnDiskAssets = bIncludeOnlyOnDiskAssets;
		AssetRegistry.GetAssets(Filter, OutAssetDataList);
	}

	return true;
}

TSet<FString> UZkzDataValidationCommandlet::BuildSelectPaths(const FString& FullCommandLine) const
{
	using namespace ZkzDataValidationCommandletPrivate;

	return ParsePaths(FullCommandLine, TEXT("SelectPaths="));
}

TSet<FString> UZkzDataValidationCommandlet::BuildIgnorePaths(
	const FString& FullCommandLine, const bool bIncludeNeverCookDirectories) const
{
	using namespace ZkzDataValidationCommandletPrivate;

	TSet<FString> IgnorePaths = ParsePaths(FullCommandLine, TEXT("IgnorePaths="));

	if (!bIncludeNeverCookDirectories)
	{
		Zkz::Game::Validation::FZkzAssetValidationUtils::AddNeverCookDirectories(IgnorePaths);
	}

	return IgnorePaths;
}

void UZkzDataValidationCommandlet::FilterAssetsToValidate(
	const bool bIncludeEngine,
	TArray<FAssetData>& AssetDataList,
	const TSet<FString>& SelectPaths,
	const TSet<FString>& IgnorePaths) const
{
	// Begin Zakazane Changes
	if (!SelectPaths.IsEmpty())
	{
		AssetDataList.RemoveAll(
			[&SelectPaths](const FAssetData& AssetData)
			{
				const FString AssetPackagePath = AssetData.PackageName.ToString();

				const bool bIsSelected = Algo::AnyOf(
					SelectPaths,
					[&AssetPackagePath](const FString& IgnorePath) { return AssetPackagePath.StartsWith(IgnorePath); });
				return !bIsSelected;
			});
	}

	if (!IgnorePaths.IsEmpty())
	{
		AssetDataList.RemoveAll(
			[&IgnorePaths](const FAssetData& AssetData)
			{
				const FString AssetPackagePath = AssetData.PackageName.ToString();

				const bool bIsIgnored = Algo::AnyOf(
					IgnorePaths,
					[&AssetPackagePath](const FString& IgnorePath) { return AssetPackagePath.StartsWith(IgnorePath); });
				return bIsIgnored;
			});
	}
	// End Zakazane Changes

	if (!bIncludeEngine)
	{
		FString EngineDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
		AssetDataList.RemoveAll(
			[&EngineDir](const FAssetData& AssetData)
			{
				// Remove /Engine and any plugins from /Engine, but keep /Game and any plugins under /Game.
				FString FileName;
				FString PackageName;
				AssetData.PackageName.ToString(PackageName);
				if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, FileName))
				{
					// We don't recognize this Package Path, so keep it
					return false;
				}
				// ConvertLongPackageNameToFilename can return ../../Plugins for some plugins instead of
				// ../../../Engine/Plugins. We should fix that in FPackageName to always return the normalized
				// filename. For now, workaround it by converting to absolute paths.
				FileName = FPaths::ConvertRelativePathToFull(MoveTemp(FileName));
				return FPathViews::IsParentPathOf(EngineDir, FileName);
			});
	}
}

void UZkzDataValidationCommandlet::HandleBlueprintValidators(const TArray<FAssetData>& AssetDataList) const
{
	ZKZ_RETURN_IF(GEditor->IsInitialized());

	// Check if we have some BP validator that were created using an editor utility
	const FTopLevelAssetPath EditorUtilityClassPath = UEditorUtilityBlueprint::StaticClass()->GetClassPathName();
	const FString EditorValidatorBaseClassExportPath =
		FObjectPropertyBase::GetExportPath(UEditorValidatorBase::StaticClass());
	const bool bHasAnEditorUtilityDataValidator = AssetDataList.ContainsByPredicate(
		[EditorUtilityClassPath, &EditorValidatorBaseClassExportPath](const FAssetData& AssetData)
		{
			if (AssetData.AssetClassPath == EditorUtilityClassPath)
			{
				if (AssetData.TagsAndValues
						.ContainsKeyValue(FBlueprintTags::NativeParentClassPath, EditorValidatorBaseClassExportPath))
				{
					return true;
				}
			}

			return false;
		});

	if (bHasAnEditorUtilityDataValidator)
	{
		// Those Editor Utilities Validator might have a dependency to an editor module that is loaded during the editor initialization.
		GEditor->LoadDefaultEditorModules();
	}
}

void UZkzDataValidationCommandlet::RunValidation(const TArray<FAssetData>& AssetDataList) const
{
	const UEditorValidatorSubsystem* const EditorValidationSubsystem =
		GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
	check(EditorValidationSubsystem);

	FValidateAssetsSettings Settings;
	FValidateAssetsResults Results;

	Settings.bSkipExcludedDirectories = true;
	Settings.bShowIfNoFailures = true;
	Settings.ValidationUsecase = EDataValidationUsecase::Commandlet;

	EditorValidationSubsystem->ValidateAssetsWithSettings(AssetDataList, Settings, Results);
}
