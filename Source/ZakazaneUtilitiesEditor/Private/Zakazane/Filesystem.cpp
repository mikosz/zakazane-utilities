#include "Zakazane/Filesystem.h"

#include "DesktopPlatformModule.h"
#include "Zakazane/ReturnIfMacros.h"

namespace Zkz::Filesystem
{

bool ShowOpenFileDialog(const FZkzFileDialogSettings& Settings, TArray<FString>& OutPaths)
{
	const auto DesktopPlatform = FDesktopPlatformModule::Get();
	ZKZ_RETURN_IF(DesktopPlatform == nullptr, {});

	const FString ResolvedPath = Settings.DefaultPath.IsEmpty() ? FPaths::ProjectSavedDir() : Settings.DefaultPath;

	return DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		Settings.Title.ToString(),
		ResolvedPath,
		Settings.DefaultFileName,
		Settings.FileTypes,
		Settings.Flags,
		OutPaths);
}

bool ShowSaveFileDialog(const FZkzFileDialogSettings& Settings, TArray<FString>& OutPaths)
{
	const auto DesktopPlatform = FDesktopPlatformModule::Get();
	ZKZ_RETURN_IF(DesktopPlatform == nullptr, {});

	const FString ResolvedPath = Settings.DefaultPath.IsEmpty() ? FPaths::ProjectSavedDir() : Settings.DefaultPath;

	return DesktopPlatform->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		Settings.Title.ToString(),
		ResolvedPath,
		Settings.DefaultFileName,
		Settings.FileTypes,
		Settings.Flags,
		OutPaths);
}

bool ShowOpenDirectoryDialog(const FZkzDirectoryDialogSettings& Settings, FString& OutPath)
{
	const auto DesktopPlatform = FDesktopPlatformModule::Get();
	ZKZ_RETURN_IF(DesktopPlatform == nullptr, {});

	return DesktopPlatform->OpenDirectoryDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		Settings.Title.ToString(),
		Settings.DefaultPath,
		OutPath);
}

}  // namespace Zkz::Filesystem