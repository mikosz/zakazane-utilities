// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"

#include "ZkzValidatorBase.generated.h"

namespace Zkz::Game::Validation
{
struct FErrorHelper;
}
class FAssetRegistryModule;

namespace Zkz::Game::Validation::EStringCheck
{

enum class Type : uint8
{
	StartsWith,
	Contains,
};

}  // namespace Zkz::Game::Validation::EStringCheck

using namespace Zkz::Game::Validation;

/// Base validator class, allows to hook up validator to Save event which can trigger logic when cooking.
/// Can handle: Validation (Saving, Validate Asset) or/and Cook
/// Settings for specific Validators should be set in Project Settings or overriden in Editor Preferences
UCLASS(Abstract)
class ZAKAZANEVALIDATION_API UZkzValidatorBase : public UEditorValidatorBase
{
	GENERATED_BODY()

public:
	UZkzValidatorBase();

	bool IsEnabledForCook() const;

	bool IsEnabledOnSave() const;

	FString GetValidatorName() const;

	FString GetInstructionURL() const;

	/// ~ UEditorValidatorBase
	/// Checks if validator is enabled on save
	virtual bool CanValidateAsset_Implementation(
		const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;

	/// Adding Log message shared by all ZkzValidators
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(
		const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	/// ~ UEditorValidatorBase

protected:
	/// ~ UEditorValidatorBase

	/// Triers to get settings from Editor Preferences or Project Settings
	/// Falls back to default if didn't succeed
	virtual bool IsEnabled() const override;

	/// ~ UEditorValidatorBase

	/// Helper function to combine Validation Marking and Return - When asset is Valid
	EDataValidationResult ValidationPassed(const UObject* InAsset);

	/// Helper function to combine Validation Marking and Return - When asset is Invalid
	/// Prints Summary, Adds ZkzError and instruction
	EDataValidationResult ValidationFailed(const UObject* InAsset, const FErrorHelper& ErrorHelper);

	/// Helper function to combine Validation Marking and Return - When Validation had to return earlier than expected
	EDataValidationResult ValidationUnexpectedlySkipped(const UObject* InAsset, const TOptional<FText>& InWarnMsg);

	/// Obtains data from settings and creates the Disallowed Paths
	TSet<FString> BuildDisallowedPaths() const;

	/// Obtains data from settings and creates the Allowed Paths
	TSet<FString> BuildAllowedPaths() const;

	/// Compare Path against Disallowed and Allowed paths to evaluate if the Asset can be Validated
	bool CanValidatePath(
		const FString& InPath, EStringCheck::Type StringCheckType = EStringCheck::Type::StartsWith) const;

	/// Fallback, If true Validator will hook up to UPackage::PackageSavedWithContextEvent
	bool bFallbackIsEnabledForCook = false;

	/// Fallback, If true Validator will hook up to UPackage::PackageSavedWithContextEvent
	bool bFallbackIsEnabledOnSave = true;

	/// In case we fail to find Config, it is fallback source of bare minimum Disallowed Paths
	TSet<FString> FallbackDisallowedPaths;

	/// In case we fail to find Config, it is fallback source of bare minimum Allowed Paths
	TSet<FString> FallbackAllowedPaths;
};
