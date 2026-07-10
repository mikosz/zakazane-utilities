#pragma once

#include "IDesktopPlatform.h"

namespace Zkz::Filesystem
{
struct FZkzFileDialogSettings
{
	FText Title;

	FString DefaultPath = FPaths::ProjectSavedDir();

	FString DefaultFileName = TEXT("NewFile");

	FString FileTypes = TEXT("All Files|*.*");

	EFileDialogFlags::Type Flags = EFileDialogFlags::None;
};

struct FZkzDirectoryDialogSettings
{
	FText Title;

	FString DefaultPath = FPaths::ProjectContentDir();
};

ZAKAZANEUTILITIESEDITOR_API bool ShowOpenFileDialog(const FZkzFileDialogSettings& Settings, TArray<FString>& OutPaths);

ZAKAZANEUTILITIESEDITOR_API bool ShowSaveFileDialog(const FZkzFileDialogSettings& Settings, TArray<FString>& OutPaths);

ZAKAZANEUTILITIESEDITOR_API bool ShowOpenDirectoryDialog(const FZkzDirectoryDialogSettings& Settings, FString& OutPath);

}  // namespace Zkz::DeveloperToolkit::Editor