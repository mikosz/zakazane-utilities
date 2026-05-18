#include "ZkzAssetValidationUtils.h"

#include "Algo/Find.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "Interfaces/IPluginManager.h"
#include "LevelEditorViewport.h"
#include "Logging/TokenizedMessage.h"
#include "Misc/DataValidation.h"
#include "Misc/UObjectToken.h"
#include "Settings/ProjectPackagingSettings.h"
#include "Zakazane/ContinueIfMacros.h"
#include "Zakazane/Logging.h"
#include "Zakazane/ReturnIfMacros.h"
#include "ZakazaneValidation.h"
#include "ZkzDataValidationProjectSettings.h"

namespace Zkz::Game::Validation
{
// ~ FErrorHelper
FErrorHelper::FErrorHelper(const FAssetData& AssetData, FDataValidationContext& Context)
	: AssetData(AssetData), Context(Context)
{
}

void FErrorHelper::AddIssueAndPrintError(const FString& Issue, const FText& Message)
{
	FoundIssues.Emplace(Issue);
	PrintError(Message);
}

FText FErrorHelper::GetFormattedIssues() const
{
	return FText::FromString(FString::Join(FoundIssues, TEXT(", ")));
}

void FErrorHelper::AddInstructionURLMessage(const FString& InstructionURL) const
{
	ZKZ_RETURN_IF(InstructionURL.IsEmpty());
	Context.AddMessage(EMessageSeverity::Info)
		->AddToken(FURLToken::Create(InstructionURL, FText::FromString("Validator Instruction")));
}

int32 FErrorHelper::GetIssuesCount() const
{
	return FoundIssues.Num();
}

void FErrorHelper::PrintError(const FText& ErrorMessage) const
{
	Context.AddMessage(EMessageSeverity::Error)->AddText(ErrorMessage);
}

FActorTokenInfo::FActorTokenInfo(const AActor* InActor, const FBox& InBoundingBoxInWorldSpace)
{
	SetActor(InActor);
	BoundingBoxInWorldSpace = InBoundingBoxInWorldSpace;
}

void FActorTokenInfo::SetActor(const AActor* InActor)
{
	ZKZ_RETURN_IF_INVALID(InActor);

	Actor = InActor;
	ActorLabel = Actor->GetActorLabel();
}
// ~ FErrorHelper

bool FZkzAssetValidationUtils::TryGetPrimaryAssetDataFromPackage(const FName& PackageName, FAssetData& OutAssetData)
{
	TArray<FAssetData> Assets;
	IAssetRegistry::GetChecked().GetAssetsByPackageName(PackageName, Assets);
	ZKZ_RETURN_IF(Assets.IsEmpty(), false);

	const FName PackageShortName = FPackageName::GetShortFName(PackageName);
	const FAssetData* const PrimaryAsset = Algo::FindBy(Assets, PackageShortName, &FAssetData::AssetName);

	OutAssetData = PrimaryAsset == nullptr ? Assets[0] : *PrimaryAsset;
	return true;
}

void FZkzAssetValidationUtils::AddNeverCookDirectories(TSet<FString>& OutNeverCookDirectories)
{
	const UProjectPackagingSettings* const PackagingSettings = GetDefault<UProjectPackagingSettings>();
	ZKZ_RETURN_IF_INVALID(PackagingSettings);

	for (const FDirectoryPath& DirectoryPath : PackagingSettings->DirectoriesToNeverCook)
	{
		OutNeverCookDirectories.Emplace(DirectoryPath.Path);
	}
}

void FZkzAssetValidationUtils::AddEnginePluginsDirectories(TSet<FString>& OutEnginePluginsDirectories)
{
	const IPluginManager& PluginManager = IPluginManager::Get();
	for (const TSharedRef<IPlugin>& Plugin : PluginManager.GetEnabledPluginsWithContent())
	{
		ZKZ_CONTINUE_IF(Plugin->GetLoadedFrom() != EPluginLoadedFrom::Engine);

		OutEnginePluginsDirectories.Emplace(Plugin->GetMountedAssetPath());
	}
}

void FZkzAssetValidationUtils::AddGlobalValidationExcludePaths(TSet<FString>& OutGlobalValidationIgnorePaths)
{
	UZkzDataValidationProjectSettings* const Settings = GetMutableDefault<UZkzDataValidationProjectSettings>();
	ZKZ_RETURN_IF_INVALID(Settings);

	for (const FDirectoryPath& ExcludePath : Settings->GlobalValidationExcludePaths)
	{
		OutGlobalValidationIgnorePaths.Emplace(ExcludePath.Path);
	}
}

void FZkzAssetValidationUtils::AddGlobalValidationIncludePaths(TSet<FString>& OutGlobalValidationIncludePaths)
{
	UZkzDataValidationProjectSettings* const Settings = GetMutableDefault<UZkzDataValidationProjectSettings>();
	ZKZ_RETURN_IF_INVALID(Settings);

	for (const FDirectoryPath& IncludePath : Settings->GlobalValidationIncludePaths)
	{
		OutGlobalValidationIncludePaths.Emplace(IncludePath.Path);
	}
}

void FZkzAssetValidationUtils::MoveViewportToActor(const FActorTokenInfo& ActorWithLocation)
{
	ZKZ_RETURN_IF_INVALID(GEditor);
	const AActor* const Actor = ActorWithLocation.Actor.Get();
	ZKZ_RETURN_IF_INVALID(Actor);

	FEditorViewportClient* ViewportClient = GCurrentLevelEditingViewportClient;
	if (ViewportClient == nullptr)
	{
		for (FEditorViewportClient* Client : GEditor->GetAllViewportClients())
		{
			ZKZ_CONTINUE_IF(Client == nullptr || !Client->IsPerspective())

			ViewportClient = Client;
			break;
		}
	}
	ZKZ_RETURN_IF(ViewportClient == nullptr);

	GEditor->SelectNone(true, true);
	GEditor->SelectActor(const_cast<AActor*>(Actor), true, true);
	GEditor->NoteSelectionChange();

	ViewportClient->FocusViewportOnBox(ActorWithLocation.BoundingBoxInWorldSpace);
}

// ReSharper disable once CppPassValueParameterByConstReference
void FZkzAssetValidationUtils::OnActorTokenInfoActivated(const TSharedRef<IMessageToken>& Token, FBox InBoundingBox)
{
	const TSharedRef<FUObjectToken> UObjectToken = StaticCastSharedRef<FUObjectToken>(Token);

	UObject* const Object = FindObject<UObject>(nullptr, *UObjectToken->GetOriginalObjectPathName());
	AActor* const ActorPtr = Cast<AActor>(Object);

	if (IsValid(ActorPtr))
	{
		const FActorTokenInfo ActorTokenInfo{ActorPtr, InBoundingBox};
		MoveViewportToActor(ActorTokenInfo);
	}
	else
	{
		LogUserError(
			ZakazaneValidation,
			EMessageSeverity::Error,
			TEXT("Cannot move viewport. Actor is no longer available."));
	}
}

}  // namespace Zkz::Game::Validation
