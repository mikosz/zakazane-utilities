// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"
#include "ZkzDataValidationProjectSettings.h"

#include "ZkzDataValidationEditorSettings.generated.h"

/// Settings for validators. Override project settings, local per developer
/// Resets each time the Project is opened
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "Data Validation Settings"))
class ZAKAZANEVALIDATION_API UZkzDataValidationEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/// Settings that can override config set in Project Settings
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Validator Overrides",
		meta = (TitleProperty = "Enabled = {bIsEnabled}, Enabled for Cook = {bIsEnabledCook}"))
	TMap<TSubclassOf<UZkzValidatorBase>, FZkzValidatorConfig> ValidatorConfigsByClassOverrides;

	/// Called when project opens to remove per user settings
	void CleanSettings();
};
