#include "ZakazaneValidation.h"

#include "Editor.h"
#include "MessageLogModule.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/UObjectIterator.h"
#include "Zakazane/ReturnIfMacros.h"
#include "ZkzDataValidationEditorSettings.h"
#include "ZkzEditorValidatorSubsystem.h"

DEFINE_LOG_CATEGORY(ZakazaneValidation)

void FZakazaneValidationModule::StartupModule()
{
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	MessageLogModule.RegisterLogListing(MessageLogName, FText::AsCultureInvariant("Validation"));
	
	UPackage::PackageSavedWithContextEvent.AddRaw(this, &FZakazaneValidationModule::OnPackageSaved);

	UZkzDataValidationEditorSettings* EditorSettings = GetMutableDefault<UZkzDataValidationEditorSettings>();
	ZKZ_RETURN_IF_INVALID(EditorSettings);
	EditorSettings->CleanSettings();  // For Safety
}

void FZakazaneValidationModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("MessageLog"))
	{
		FMessageLogModule& MessageLogModule = FModuleManager::GetModuleChecked<FMessageLogModule>("MessageLog");
		MessageLogModule.UnregisterLogListing(MessageLogName);
	}
	
	UPackage::PackageSavedWithContextEvent.RemoveAll(this);
}

void FZakazaneValidationModule::OnPackageSaved(
	const FString& FileName, UPackage* Package, FObjectPostSaveContext SaveContext)
{
	ZKZ_RETURN_IF_INVALID(Package);
	ZKZ_RETURN_IF_INVALID(GEditor);
	UZkzEditorValidatorSubsystem* EditorValidatorSubsystem =
		GEditor->GetEditorSubsystem<UZkzEditorValidatorSubsystem>();
	ZKZ_RETURN_IF_INVALID_ENSURE(EditorValidatorSubsystem);

	EditorValidatorSubsystem->ValidateCookedPackage(Package->GetFName(), SaveContext.IsCooking());
}

IMPLEMENT_MODULE(FZakazaneValidationModule, ZakazaneValidation)
