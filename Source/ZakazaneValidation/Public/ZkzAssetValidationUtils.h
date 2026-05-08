// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "AssetRegistry/AssetData.h"

namespace Zkz::Game::Validation
{

// ~FErrorHelper
/// Struct that helps in printing Error also used to make summary more descriptive
/// Validators can inherit from it to add custom data so it limits amount of arguments in check functions
struct ZAKAZANEVALIDATION_API FErrorHelper
{
	const FAssetData& AssetData;

	FDataValidationContext& Context;

	FErrorHelper(const FAssetData& AssetData, FDataValidationContext& Context);

	/// Adds Issue and Prints Error Message
	/// @param Issue short string key used for summary of validation
	/// @param Format localizable FText that takes arguments for format
	/// @param Args arguments to pass to FTextFormat
	template <typename... ArgsType>
	void AddIssueAndPrintError(const FString& Issue, const FText& Format, ArgsType&&... Args);

	/// Adds Issue and Prints Error Message
	/// @param Issue short string key used for summary of validation
	/// @param Message localizable FText
	void AddIssueAndPrintError(const FString& Issue, const FText& Message);

	/// Returns all issues joined by comma
	FText GetFormattedIssues() const;

	void AddInstructionURLMessage(const FString& InstructionURL) const;

	int32 GetIssuesCount() const;

private:
	/// Adds token to Asset that is validated and formatted message
	void PrintError(const FText& ErrorMessage) const;

	TArray<FString> FoundIssues;
};

template <typename... ArgsType>
void FErrorHelper::AddIssueAndPrintError(const FString& Issue, const FText& Format, ArgsType&&... Args)
{
	FoundIssues.Emplace(Issue);
	PrintError(FText::Format(Format, Forward<ArgsType>(Args)...));
}

// ~FErrorHelper

// ~FActorTokenInfo
/// Stores Actor and its bounding box in world space.
/// @note: In World Partitioned Levels when we look for actors they are stored with location relative to the world owning them.
/// It makes Tokens to Actors from PLIs to move the Viewport Camera to Actor Location relative to PLI origin but in World Space.
struct FActorTokenInfo
{
	TWeakObjectPtr<const AActor> Actor;

	/// Used in Tokens to center the viewport on the Actor
	FBox BoundingBoxInWorldSpace;
};
// ~FActorTokenInfo

/// Function Library for Validators
class ZAKAZANEVALIDATION_API FZkzAssetValidationUtils
{
public:
	/// Based on the given Package FName we try to get Primary Asset
	/// returns false if failed
	static bool TryGetPrimaryAssetDataFromPackage(const FName& PackageName, FAssetData& OutAssetData);

	/// Get NeverCookDirectories from Project Settings
	static void AddNeverCookDirectories(TSet<FString>& OutNeverCookDirectories);

	/// Moves the viewport to given Actor. Selecting the Actor if possible.
	static void MoveViewportToActor(const FActorTokenInfo& ActorWithLocation);

	/// Helper function for loading and initializing a world from a map name.
	/// Using UWorld::InitializationValues to prevent the loading heavy systems like AI or navigation.
	/// Based on WorldPartitionCommandletHelpers::LoadAndInitWorld, which in inconveniently inaccessible
	/// @note World will be added to root make sure to remove it after processing
	static UWorld* PrepareWorldForInitByName(const FString& MapName);

	/// Helper function for loading and initializing a world from an asset UObject.
	/// Based on WorldPartitionCommandletHelpers::LoadAndInitWorld, which in inconveniently inaccessible
	static UWorld* PrepareWorldForInitByAsset(UObject* InAsset);

private:
	/// Performs a lightweight initialization on a World, bypassing heavy runtime systems.
	static void SetWorldInitializationValues(UWorld* World);
};

}  // namespace Zkz::Game::Validation
