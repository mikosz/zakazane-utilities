// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorSubsystem.h"

#include "ZkzEditorValidatorSubsystem.generated.h"

class UZkzValidatorBase;

/// Extension for EditorValidatorSubsystem
/// Adds extra logic that takes care of validating during cooking process
UCLASS()
class ZAKAZANEVALIDATION_API UZkzEditorValidatorSubsystem : public UEditorValidatorSubsystem
{
	GENERATED_BODY()

public:
	// ~ UEditorValidatorSubsystem

	/// Adds ZkzValidators
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/// Removes ZkzValidators
	virtual void Deinitialize() override;

	/// Runs addition validation on assets that should be always validated
	virtual EDataValidationResult ValidateChangelist(
		UDataValidationChangelist* InChangelist,
		const FValidateAssetsSettings& InSettings,
		FValidateAssetsResults& OutResults) const override;

	// ~ UEditorValidatorSubsystem

	/// Called from the module when any Package is saved
	/// Body of this function mostly consists of FDataValidationModule::OnValidateSourcePackageDuringCook
	void ValidateCookedPackage(const FName CookedPackageName, const bool bIsCooking) const;

private:
	void OnAllModuleLoadingPhasesComplete();
	
	/// Adds ZkzValidators to exclusive Array
	/// @note: In 5.6 the way of how validators are initialized changed. UE Validators are registered OnPostEngineInit,
	/// So to prepare the array only with ZkzValidators, we use this callback to OnAllModuleLoadingPhasesComplete
	void RegisterZkzValidators();
	
	/// IsObjectValidWithContext function suited for Cooked Objects
	/// Body of this function mostly consists of UEditorValidatorSubsystem::IsObjectValidWithContext and ValidateObjectInternal
	EDataValidationResult IsCookedObjectValidWithContext(UObject* InObject, FDataValidationContext& InContext) const;

	/// Iterates through all ZkzValidators and call passed Callback on Valid and Enabled to Cook Validators
	void ForEachEnabledCookValidator(const TFunctionRef<bool(UZkzValidatorBase* Validator)>& Callback) const;

	/// Array of ZkzValidators they can be enabled for cooking
	UPROPERTY()
	TArray<UZkzValidatorBase*> ZkzValidators;
};
