// Copyright ZAKAZANE Studio. All Rights Reserved.

#include "ZkzValidatorBase.h"

#include "Algo/AnyOf.h"
#include "Misc/DataValidation.h"
#include "Utility/ZkzValidationLogCategory.h"
#include "Zakazane/Logging.h"
#include "Zakazane/ReturnIfMacros.h"
#include "ZkzAssetValidationUtils.h"
#include "ZkzDataValidationProjectSettings.h"

#define LOCTEXT_NAMESPACE "ZkzValidatorBase"

UZkzValidatorBase::UZkzValidatorBase()
{
	bIsEnabled = true;
	bFallbackIsEnabledForCook = true;
	bOnlyPrintCustomMessage = true;
	FZkzAssetValidationUtils::AddNeverCookDirectories(FallbackDisallowedPaths);

	
}

bool UZkzValidatorBase::IsEnabledForCook() const
{
	const FZkzValidatorConfig* const Config = UZkzDataValidationProjectSettings::GetActiveValidatorConfig(GetClass());
	if (Config == nullptr)
	{
		return bFallbackIsEnabledForCook;
	}

	return Config->bIsEnabledCook;
}

bool UZkzValidatorBase::IsEnabledOnSave() const
{
	const FZkzValidatorConfig* const Config = UZkzDataValidationProjectSettings::GetActiveValidatorConfig(GetClass());
	if (Config == nullptr)
	{
		return bFallbackIsEnabledOnSave;
	}

	return Config->bIsEnabledOnSave;
}

FString UZkzValidatorBase::GetValidatorName() const
{
	const FZkzValidatorConfig* const Config = UZkzDataValidationProjectSettings::GetActiveValidatorConfig(GetClass());
	if (Config == nullptr)
	{
		return GetName();
	}

	return Config->ValidatorName;
}

FString UZkzValidatorBase::GetInstructionURL() const
{
	const FZkzValidatorConfig* const Config = UZkzDataValidationProjectSettings::GetActiveValidatorConfig(GetClass());
	if (Config == nullptr)
	{
		return FString();
	}

	return Config->URLToInstruction;
}

bool UZkzValidatorBase::CanValidateAsset_Implementation(
	const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	using namespace Zkz::Game::Validation;
	
	ZKZ_RETURN_IF_INVALID(InObject, false);
	
	const EDataValidationUsecase ValidationUsecase = InContext.GetValidationUsecase();
	if (ValidationUsecase == EDataValidationUsecase::Save && !IsRunningCookCommandlet())
	{
		ZKZ_RETURN_IF(!IsEnabledOnSave(), false);
	}
	
	const FString AssetPath = InAssetData.PackagePath.ToString();
	const bool bPathAllowed = CanValidatePath(AssetPath);
	ZKZ_RETURN_IF(!bPathAllowed, false);
	
	return true;
}

EDataValidationResult UZkzValidatorBase::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	UE_LOG(LogZkzValidation, Display, TEXT("[%s] Validating: %s"), *GetValidatorName(), *InAsset->GetPathName());
	return EDataValidationResult::NotValidated;
}

bool UZkzValidatorBase::IsEnabled() const
{
	const FZkzValidatorConfig* const Config = UZkzDataValidationProjectSettings::GetActiveValidatorConfig(GetClass());
	ZKZ_RETURN_IF(Config != nullptr, Config->bIsEnabled);

	return Super::IsEnabled();
}

EDataValidationResult UZkzValidatorBase::ValidationPassed(const UObject* InAsset)
{
	AssetPasses(InAsset);
	return EDataValidationResult::Valid;
}

EDataValidationResult UZkzValidatorBase::ValidationFailed(const UObject* InAsset, const FErrorHelper& ErrorHelper)
{
	const FText SummaryMessage = FText::Format(
		LOCTEXT("NeverCookContentValidationSummary", "{0} Failed. Reason: {1}. See log for details."),
		FText::AsCultureInvariant(GetValidatorName()),
		ErrorHelper.GetFormattedIssues());

	AssetFails(InAsset, SummaryMessage);

	if (!IsRunningCookCommandlet())
	{
		ErrorHelper.AddInstructionURLMessage(GetInstructionURL());
		Zkz::LogUserError(AssetCheck, EMessageSeverity::Type::Error, SummaryMessage.ToString());
	}

	return EDataValidationResult::Invalid;
}

EDataValidationResult UZkzValidatorBase::ValidationUnexpectedlySkipped(
	const UObject* InAsset, const TOptional<FText>& InWarnMsg)
{
	if (InWarnMsg.IsSet())
	{
		AssetWarning(InAsset, InWarnMsg.GetValue());

		if (!IsRunningCookCommandlet())
		{
			Zkz::LogUserError(AssetCheck, EMessageSeverity::Type::Warning, InWarnMsg.GetValue().ToString());
		}
	}

	return EDataValidationResult::Invalid;
}

TSet<FString> UZkzValidatorBase::BuildDisallowedPaths() const
{
	const FZkzValidatorConfig* const Config = UZkzDataValidationProjectSettings::GetActiveValidatorConfig(GetClass());
	if (Config == nullptr)
	{
		return FallbackDisallowedPaths;
	}

	TSet<FString> DisallowedPaths;
	for (const FDirectoryPath& DisallowedPath : Config->DisallowedPaths)
	{
		DisallowedPaths.Emplace(DisallowedPath.Path);
	}

	if (Config->bDisallowNeverCookDirectories)
	{
		FZkzAssetValidationUtils::AddNeverCookDirectories(DisallowedPaths);
	}

	return DisallowedPaths;
}

TSet<FString> UZkzValidatorBase::BuildAllowedPaths() const
{
	const FZkzValidatorConfig* const Config = UZkzDataValidationProjectSettings::GetActiveValidatorConfig(GetClass());
	if (Config == nullptr)
	{
		return FallbackAllowedPaths;
	}

	TSet<FString> AllowedPaths;
	for (const FDirectoryPath& AllowedPath : Config->AllowedPaths)
	{
		AllowedPaths.Emplace(AllowedPath.Path);
	}
	return AllowedPaths;
}

bool UZkzValidatorBase::CanValidatePath(const FString& InPath, const EStringCheck::Type StringCheckType) const
{
	const TSet<FString> DisallowedPaths = BuildDisallowedPaths();
	const TSet<FString> AllowedPaths = BuildAllowedPaths();

	bool bPathDisallowed;
	bool bPathAllowed;

	switch (StringCheckType)
	{
		// First checks if given asset is in Disallowed folder, if so then checks if it's in Allowed folder
		// TODO: change to LookUp setting per Path instead of per Validator
		default:
		case EStringCheck::Type::StartsWith:
			bPathDisallowed =
				Algo::AnyOf(DisallowedPaths, [&InPath](const FString& Path) { return InPath.StartsWith(Path); });

			if (!bPathDisallowed)
			{
				return true;
			}

			bPathAllowed =
				Algo::AnyOf(AllowedPaths, [&InPath](const FString& Path) { return InPath.StartsWith(Path); });

			return bPathAllowed;

		case EStringCheck::Type::Contains:
			bPathDisallowed =
				Algo::AnyOf(DisallowedPaths, [&InPath](const FString& Path) { return InPath.StartsWith(Path); });

			if (!bPathDisallowed)
			{
				return true;
			}

			bPathAllowed =
				Algo::AnyOf(AllowedPaths, [&InPath](const FString& Path) { return InPath.StartsWith(Path); });

			return bPathAllowed;
	}
}

#undef LOCTEXT_NAMESPACE
