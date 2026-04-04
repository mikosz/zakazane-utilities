// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"

#include <type_traits>

#include "AssetFunctionLibrary.generated.h"

class UFactory;
class IAssetEditorInstance;

namespace Zkz::Game::Editor
{

ZAKAZANEUTILITIESEDITOR_API IAssetEditorInstance* FindAssetEditorFor(UObject& Asset);

}  // namespace Zkz::Game::Editor

UCLASS()
class ZAKAZANEUTILITIESEDITOR_API UZkzAssetFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Zakazane|Editor|Asset")
	static UObject* CreateAsset(FString AssetPath, UClass* AssetClass, UFactory* Factory);

	UFUNCTION(BlueprintCallable, Category = "Zakazane|Editor|Asset")
	static UObject* DuplicateAsset(UObject* SourceAsset, const FString& AssetPathTarget);

	static UObject* CreateDataAsset(const FString& AssetPath, UClass* DataAssetClass);

	static bool IsAssetExists(const FString& AssetPath);

	static UObject* GetAssetByPath(const FString& AssetPath);

	static FString GetWorldAssetPath(const UWorld* World, const FString& AssetName);

	/// Optionally prompts the user to save package of provided object, and then additionally prompts the user to check-out it. If the user cancels their way out of either dialog, no packages are saved. It is possible the user
	/// will be prompted again, if the saving process fails for any reason. In that case, the user will be prompted on a package-by-package basis, allowing them
	/// to retry saving, skip trying to save the current package, or to again cancel out of the entire dialog. After all packages are saved (or not), the user is provided with
	/// a warning about any packages that were writable on disk but not in source control, as well as a warning about which packages failed to save.
	/// @param		_CheckDirty					If true, only package will that is dirty will be saved
	/// @param		_PromptToSave				If true the user will be prompted with a list of packages to save, otherwise package is saved immediately
	UFUNCTION(BlueprintCallable, Category = "Zakazane|Editor|Asset")
	static void PromptForCheckoutAndSave(UObject* _ObjectToSave, bool _CheckDirty, bool _PromptToSave);

	/// @param _DirectoryPath Path for directory to search. Directory path should start from /Game
	/// @param _Recursive Set to true to include subdirectories
	/// @param _AssetClass Set to specific class if you want to filter result by class
	/// @param _RecursiveClasses Set to true if you want to filter also by subclasses of specified class
	/// @return
	UFUNCTION(BlueprintCallable, Category = "Zakazane|Editor|Asset")
	static TArray<FString> GetAssetPathsInDirectory(
		const FString& _DirectoryPath,
		bool _Recursive,
		TSubclassOf<UObject> _AssetClass,
		bool _RecursiveClasses = true);

	UFUNCTION(BlueprintCallable, Category = "Zakazane|Editor|Asset")
	static UObject* LoadAssetFromPath(const FString& _AssetPath);

	/// Safely prepares and saves an asset to disk.
	/// Automatically resolves whether the asset uses an OFPA external package
	/// or a standard outermost package, handles Source Control integration, and marks the object as dirty.
	UFUNCTION(BlueprintCallable, Category = "Zakazane|Editor|Asset")
	static bool CheckoutAndSaveAsset(UObject* InAsset);

private:
	/// Interfaces with the active Source Control provider to prepare a local file for modification.
	/// Will automatically Check Out tracked files or Mark for Add if the file is completely new/untracked.
	static bool CheckoutExternalPackage(const FString& InPackageFilename);
};
