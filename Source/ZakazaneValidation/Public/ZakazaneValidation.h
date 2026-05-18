#pragma once

#include "CoreMinimal.h"

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(ZakazaneValidation, Log, All)

class FZakazaneValidationModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/// Callback for UPackage::PackageSavedWithContextEvent
	void OnPackageSaved(const FString& FileName, UPackage* Package, FObjectPostSaveContext SaveContext);
	
private:
	const FName MessageLogName{"ZakazaneValidation"};
};
