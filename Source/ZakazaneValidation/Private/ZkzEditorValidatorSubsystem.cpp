// Copyright ZAKAZANE Studio. All Rights Reserved.

#include "ZkzEditorValidatorSubsystem.h"

#include "AssetRegistry/AssetDataToken.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "DataValidationChangelist.h"
#include "Editor.h"
#include "EditorValidatorHelpers.h"
#include "Logging/MessageLog.h"
#include "Utility/ZkzValidationLogCategory.h"
#include "Zakazane/ContinueIfMacros.h"
#include "Zakazane/ReturnIfMacros.h"
#include "ZkzAssetValidationUtils.h"
#include "ZkzDataValidationProjectSettings.h"
#include "ZkzValidatorBase.h"
#include "Zakazane/Logging.h"

#define LOCTEXT_NAMESPACE "ValidatorSubsystem"

namespace UZkzEditorValidatorSubsystemPrivate
{
/// Takes care of the case when the validator class was loaded after the default settings object was loaded by reloading the default settings object from .ini file again
/// Without this hack settings object would clear out data related to these classes, because at the moment of the initial config load they were invalid
void EnsureValidationSettingsInitialized()
{
	const UClass* const DataValidationSettingsClass = UZkzDataValidationProjectSettings::StaticClass();
	ZKZ_RETURN_IF_INVALID_ENSURE(DataValidationSettingsClass);

	UObject* const DataValidationSettingsDefaultObject = DataValidationSettingsClass->GetDefaultObject();
	ZKZ_RETURN_IF_INVALID_ENSURE(DataValidationSettingsDefaultObject);

	DataValidationSettingsDefaultObject->ReloadConfig();
}

void LogValidatorsNotInSettings()
{
	TArray<UClass*> ValidatorClasses;
	GetDerivedClasses(UZkzValidatorBase::StaticClass(), ValidatorClasses);

	for (UClass* const ValidatorClass : ValidatorClasses)
	{
		const bool bAbstract = ValidatorClass->HasAnyClassFlags(CLASS_Abstract);
		ZKZ_CONTINUE_IF(bAbstract);

		const FZkzValidatorConfig* const Config = UZkzDataValidationProjectSettings::GetActiveValidatorConfig(
			ValidatorClass);
		if (Config == nullptr)
		{
			Zkz::LogUserError(
				LogZkzValidation,
				EMessageSeverity::Type::Warning,
				FString::Printf(
					TEXT(
						"Config not set for %s in Project Setting or Editor Preferences. Default values will be used!"),
					*ValidatorClass->GetName()));
		}
	}
}
}

void UZkzEditorValidatorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FCoreDelegates::OnAllModuleLoadingPhasesComplete
		.AddUObject(this, &UZkzEditorValidatorSubsystem::OnAllModuleLoadingPhasesComplete);
}

void UZkzEditorValidatorSubsystem::Deinitialize()
{
	ZkzValidators.Empty();

	Super::Deinitialize();
}

EDataValidationResult UZkzEditorValidatorSubsystem::ValidateChangelist(
	UDataValidationChangelist* InChangelist,
	const FValidateAssetsSettings& InSettings,
	FValidateAssetsResults& OutResults) const
{
	EDataValidationResult Result = Super::ValidateChangelist(InChangelist, InSettings, OutResults);

	const TArray<FName> ExtraPackages = UZkzDataValidationProjectSettings::GetAlwaysValidateAssetsPreSubmit();
	ZKZ_RETURN_IF(ExtraPackages.IsEmpty(), Result);

	FValidateAssetsSettings ExtraSettings = InSettings;
	ExtraSettings.bCollectPerAssetDetails = true;

	IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();
	TArray<FAssetData> ExtraAssetsToValidate;
	for (const FName& PkgName : ExtraPackages)
	{
		ZKZ_CONTINUE_IF(InChangelist->ModifiedPackageNames.Contains(PkgName));

		TArray<FAssetData> AssetsInPackage;
		AssetRegistry.GetAssetsByPackageName(PkgName, AssetsInPackage);
		ExtraAssetsToValidate.Append(AssetsInPackage);
	}

	ZKZ_RETURN_IF(ExtraAssetsToValidate.IsEmpty(), Result);

	FValidateAssetsResults ExtraResults;
	ValidateAssetsWithSettings(ExtraAssetsToValidate, ExtraSettings, ExtraResults);

	if (ExtraResults.NumInvalid > 0 || ExtraResults.NumWarnings > 0)
	{
		if (ExtraResults.NumInvalid > 0)
		{
			Result = EDataValidationResult::Invalid;
		}
		else if (Result != EDataValidationResult::Invalid && ExtraResults.NumWarnings > 0)
		{
			Result = EDataValidationResult::Valid;
		}

		FValidateAssetsDetails& ChangelistSummary = OutResults.AssetsDetails.FindOrAdd(InChangelist->GetPathName());
		ChangelistSummary.Result = Result;

		bool bHasAddedHeader = false;

		for (const auto& [AssetPath, AssetDetails] : ExtraResults.AssetsDetails)
		{
			if (AssetDetails.Result == EDataValidationResult::Invalid)
			{
				FText BodyMsg = FText::Format(
					LOCTEXT("SubmitErrorShort", "{0} didn't pass validation."),
					FText::FromString(AssetDetails.AssetName.ToString())
					);

				if (!bHasAddedHeader)
				{
					FText HeaderMsg = LOCTEXT(
						"SubmitErrorHeader",
						"Your changes affected validation of assets from outside the changelist:");
					ChangelistSummary.ValidationErrors.Add(
						FText::Format(FText::FromString(TEXT("{0}\n{1}")), HeaderMsg, BodyMsg));
					bHasAddedHeader = true;
				}
				else
				{
					ChangelistSummary.ValidationErrors.Add(BodyMsg);
				}
			}

			if (AssetDetails.Result == EDataValidationResult::Valid && !AssetDetails.ValidationWarnings.IsEmpty())
			{
				FText WarningMessage = FText::Format(
					LOCTEXT("SubmitWarningShort", "{0} valid with warnings."),
					FText::FromString(AssetDetails.AssetName.ToString()));

				ChangelistSummary.ValidationWarnings.Add(WarningMessage);
				OutResults.NumWarnings++;
			}

			OutResults.AssetsDetails.Add(AssetPath, AssetDetails);
		}

		OutResults.NumInvalid += ExtraResults.NumInvalid;
		OutResults.NumWarnings += ExtraResults.NumWarnings;
		OutResults.NumChecked += ExtraResults.NumChecked;
	}

	return Result;
}

void UZkzEditorValidatorSubsystem::ValidateCookedPackage(const FName CookedPackageName, const bool bIsCooking) const
{
	ZKZ_RETURN_IF(!bIsCooking);

	FAssetData DataAsset;
	FZkzAssetValidationUtils::TryGetPrimaryAssetDataFromPackage(CookedPackageName, DataAsset);
	FDataValidationContext ValidationContext(IsRunningCookCommandlet(), EDataValidationUsecase::Save, {DataAsset});
	EDataValidationResult FinalValidationResult = EDataValidationResult::NotValidated;

	{
		[[maybe_unused]] static const auto LogValidatorsListOnce = [this]()
		{
			UE_LOG(LogZkzValidation, Display, TEXT("Enabled Cook Validators:"));
			ForEachEnabledCookValidator(
				[](const UZkzValidatorBase* CookValidator)
				{
					ZKZ_RETURN_IF_INVALID(CookValidator, true);
					UE_LOG(LogZkzValidation, Display, TEXT("\t%s"), *CookValidator->GetValidatorName());
					return true;
				});
			return true;
		}();
	}

	TArray<FAssetData> AssetList;
	IAssetRegistry::GetChecked().GetAssetsByPackageName(CookedPackageName, AssetList);

	ZKZ_RETURN_IF(AssetList.Num() <= 0);

	FMessageLog DataValidationLog(UE::DataValidation::MessageLogName);
	for (const FAssetData& AssetData : AssetList)
	{
		UObject* Asset = AssetData.GetAsset();
		ZKZ_CONTINUE_IF_INVALID(Asset);

		DataValidationLog.Info()
		                 ->AddToken(FAssetDataToken::Create(AssetData))
		                 ->AddToken(FTextToken::Create(LOCTEXT("Data.ValidatingAsset", "Validating asset")));

		UE::DataValidation::FScopedLogMessageGatherer LogGatherer;
		EDataValidationResult ValidationResult = IsCookedObjectValidWithContext(Asset, ValidationContext);

		TArray<FString> LogWarnings;
		TArray<FString> LogErrors;
		LogGatherer.Stop(LogWarnings, LogErrors);

		if (LogWarnings.Num() > 0)
		{
			TStringBuilder<2048> Buffer;
			Buffer.Join(LogWarnings, LINE_TERMINATOR);
			ValidationContext.AddMessage(EMessageSeverity::Warning)
			                 ->AddToken(FAssetDataToken::Create(AssetData))
			                 ->AddText(
				                 LOCTEXT(
					                 "DataValidation.DuringValidationWarnings",
					                 "Warnings logged while validating asset {0}"),
				                 FText::FromStringView(Buffer.ToView()));
		}

		if (LogErrors.Num() > 0)
		{
			TStringBuilder<2048> Buffer;
			Buffer.Join(LogErrors, LINE_TERMINATOR);
			ValidationContext.AddMessage(EMessageSeverity::Error)
			                 ->AddToken(FAssetDataToken::Create(AssetData))
			                 ->AddText(
				                 LOCTEXT(
					                 "DataValidation.DuringValidationErrors",
					                 "Errors logged while validating asset {0}"),
				                 FText::FromStringView(Buffer.ToView()));
			ValidationResult = EDataValidationResult::Invalid;
		}

		FinalValidationResult = CombineDataValidationResults(FinalValidationResult, ValidationResult);

		UE::DataValidation::AddAssetValidationMessages(AssetData, DataValidationLog, ValidationContext);
		DataValidationLog.Flush();
	}
}

void UZkzEditorValidatorSubsystem::OnAllModuleLoadingPhasesComplete()
{
	RegisterZkzValidators();
	UZkzEditorValidatorSubsystemPrivate::EnsureValidationSettingsInitialized();
	UZkzEditorValidatorSubsystemPrivate::LogValidatorsNotInSettings();
}

void UZkzEditorValidatorSubsystem::RegisterZkzValidators()
{
	for (const TPair<FTopLevelAssetPath, TObjectPtr<UEditorValidatorBase>>& Validator : Validators)
	{
		UZkzValidatorBase* const ZkzValidator = Cast<UZkzValidatorBase>(Validator.Value);
		ZKZ_CONTINUE_IF_INVALID(ZkzValidator);

		ZkzValidators.Emplace(ZkzValidator);
	}
}

EDataValidationResult UZkzEditorValidatorSubsystem::IsCookedObjectValidWithContext(
	UObject* InObject,
	FDataValidationContext& InContext) const
{
	EDataValidationResult Result = EDataValidationResult::NotValidated;
	ZKZ_RETURN_IF_ENSURE(!InObject, Result);

	const IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();

	FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(InObject), true);
	if (!AssetData.IsValid())
	{
		AssetData = FAssetData(InObject);
	}
	ZKZ_RETURN_IF_ENSURE(!AssetData.IsValid(), Result);

	Result = const_cast<const UObject*>(InObject)->IsDataValid(InContext);
	ZKZ_RETURN_IF(Result == EDataValidationResult::Invalid, Result);

	ForEachEnabledCookValidator(
		[&Result, &AssetData, &InObject, &InContext](UZkzValidatorBase* ZkzValidator)
		{
			const EDataValidationResult NewResult = ZkzValidator->ValidateLoadedAsset(AssetData, InObject, InContext);
			Result = CombineDataValidationResults(Result, NewResult);

			return true;
		});

	return Result;
}

void UZkzEditorValidatorSubsystem::ForEachEnabledCookValidator(
	const TFunctionRef<bool(UZkzValidatorBase* ZkzValidator)>& Callback) const
{
	for (UZkzValidatorBase* ZkzValidator : ZkzValidators)
	{
		ZKZ_CONTINUE_IF_INVALID(ZkzValidator);
		ZKZ_CONTINUE_IF(!ZkzValidator->IsEnabledForCook());

		ZKZ_RETURN_IF(!Callback(ZkzValidator));
	}
}

#undef LOCTEXT_NAMESPACE