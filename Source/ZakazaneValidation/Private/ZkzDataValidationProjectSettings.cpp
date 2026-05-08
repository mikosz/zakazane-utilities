// Copyright ZAKAZANE Studio. All Rights Reserved.

#include "ZkzDataValidationProjectSettings.h"

#include "Utility/ZkzValidationLogCategory.h"
#include "Zakazane/ReturnIfMacros.h"
#include "ZkzDataValidationEditorSettings.h"

const FZkzValidatorConfig* UZkzDataValidationProjectSettings::GetActiveValidatorConfig(
	const TSubclassOf<UZkzValidatorBase> ValidatorClass)
{
	ZKZ_RETURN_IF_INVALID_ENSUREMSGF(
		ValidatorClass, TEXT("GetActiveValidatorConfig called with invalid ValidatorClass"), nullptr);

	const UZkzDataValidationEditorSettings* const EditorSettings = GetDefault<UZkzDataValidationEditorSettings>();
	if (IsValid(EditorSettings))
	{
		const FZkzValidatorConfig* Config = EditorSettings->ValidatorConfigsByClassOverrides.Find(ValidatorClass);

		ZKZ_RETURN_IF(Config != nullptr, Config);
	}

	const UZkzDataValidationProjectSettings* const Settings = GetDefault<UZkzDataValidationProjectSettings>();
	if (IsValid(Settings))
	{
		const FZkzValidatorConfig* const Config = Settings->ValidatorConfigsByClass.Find(ValidatorClass);

		ZKZ_RETURN_IF(Config != nullptr, Config);
	}

	return nullptr;
}

TArray<FName> UZkzDataValidationProjectSettings::GetAlwaysValidateAssetsPreSubmit()
{
	TArray<FName> Packages;
	const UZkzDataValidationProjectSettings* const Settings = GetDefault<UZkzDataValidationProjectSettings>();

	if (IsValid(Settings))
	{
		for (const TSoftObjectPtr<>& SoftPtr : Settings->AssetsToAlwaysValidatePreSubmit)
		{
			Packages.Add(FName(*SoftPtr.GetLongPackageName()));
		}
	}

	return Packages;
}

TArray<TSubclassOf<UActorComponent>> UZkzDataValidationProjectSettings::GetAllowedClusterActorsComponents()
{
	const UZkzDataValidationProjectSettings* const Settings = GetDefault<UZkzDataValidationProjectSettings>();
	ZKZ_RETURN_IF_INVALID(Settings, {});
	
	return Settings->AllowedClusterActorsComponents;
}
