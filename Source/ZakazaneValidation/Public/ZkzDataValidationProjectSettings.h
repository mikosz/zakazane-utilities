// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"
#include "Templates/SubclassOf.h"
#include "ZkzValidatorBase.h"

#include "ZkzDataValidationProjectSettings.generated.h"

/// Configuration setting for each validator
USTRUCT(BlueprintType)
struct FZkzValidatorConfig
{
	GENERATED_BODY()

	/// User-friendly name used in logs
	UPROPERTY(Config, EditAnywhere, Category = "Validator")
	FString ValidatorName;

	/// If false, this validator will not work when Validating Assets or On Save (doesn't affect Is Enabled Cook)
	UPROPERTY(Config, EditAnywhere, Category = "Validator")
	bool bIsEnabled = true;

	/// If false, this validator will not work during the cook process (isn't affected by Is Enabled)
	UPROPERTY(Config, EditAnywhere, Category = "Validator")
	bool bIsEnabledCook = true;

	/// If false, this validator will not work on manual save in the editor
	UPROPERTY(Config, EditAnywhere, Category = "Validator")
	bool bIsEnabledOnSave = true;

	/// Paths that are DISALLOWED by this validator
	/// Depending on implementation Validator uses whole Path like /Game/Developers or just part of it like /_Generated_
	UPROPERTY(Config, EditAnywhere, Category = "Paths", meta = (RelativeToGameContentDir))
	TArray<FDirectoryPath> DisallowedPaths;

	/// Automatically include Never Cook Directories in the Disallowed Paths
	UPROPERTY(Config, EditAnywhere, Category = "Paths")
	bool bDisallowNeverCookDirectories = true;

	/// Paths that are ALLOWED, even if they are in a Disallowed Paths (e.g. /Game/Developers/Important)
	UPROPERTY(Config, EditAnywhere, Category = "Paths", meta = (RelativeToGameContentDir))
	TArray<FDirectoryPath> AllowedPaths;

	/// URL to the instruction for Validator
	UPROPERTY(Config, EditAnywhere, Category = "Validator")
	FString URLToInstruction;
};

/// Settings for validators. Default and shared for the whole team.
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Data Validation Settings"))
class ZAKAZANEVALIDATION_API UZkzDataValidationProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/// Settings that override config set in Code
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Validators",
		meta =
			(TitleProperty =
				 "Enabled = {bIsEnabled}, Enabled for Cook = {bIsEnabledCook}, Enabled On Save = {bIsEnabledOnSave}"))
	TMap<TSubclassOf<UZkzValidatorBase>, FZkzValidatorConfig> ValidatorConfigsByClass;
	
	/// Add assets that will always be validated pre-submit
	UPROPERTY(Config, EditAnywhere, Category = "Submit")
	TArray<TSoftObjectPtr<UObject>> AssetsToAlwaysValidatePreSubmit;
	
	UPROPERTY(Config, EditAnywhere, Category = "ValidatorSpecific|ClusterActors")
	TArray<TSubclassOf<UActorComponent>> AllowedClusterActorsComponents;
	
	/// Helper function to find the config for a specific validator class if config from "Editor Preferences" has higher priority.
	static const FZkzValidatorConfig* GetActiveValidatorConfig(TSubclassOf<UZkzValidatorBase> ValidatorClass);
	
	/// Helper function to get assets that should always be validated pre-submit
	static TArray<FName> GetAlwaysValidateAssetsPreSubmit();
	
	/// Getter for allowed cluster actors components
	static TArray<TSubclassOf<UActorComponent>> GetAllowedClusterActorsComponents();
};
