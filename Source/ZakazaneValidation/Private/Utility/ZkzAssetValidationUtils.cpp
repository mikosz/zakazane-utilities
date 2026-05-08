#include "ZkzAssetValidationUtils.h"

#include "Algo/Find.h"
#include "AssetRegistry/AssetDataToken.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "ISourceControlModule.h"
#include "LevelEditorViewport.h"
#include "LevelInstance/LevelInstanceLevelStreaming.h"
#include "Logging/TokenizedMessage.h"
#include "Misc/DataValidation.h"
#include "Settings/ProjectPackagingSettings.h"
#include "SourceControlOperations.h"
#include "UObject/SavePackage.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionHelpers.h"
#include "Zakazane/ContinueIfMacros.h"
#include "Zakazane/ReturnIfMacros.h"
#include "ZkzValidationLogCategory.h"

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

UWorld* FZkzAssetValidationUtils::PrepareWorldForInitByName(const FString& MapName)
{
	UPackage* const MapPackage = LoadPackage(nullptr, *MapName, LOAD_None);
	ZKZ_RETURN_IF_INVALID(MapPackage, nullptr);

	UWorld* const World = UWorld::FindWorldInPackage(MapPackage);
	ZKZ_RETURN_IF_INVALID(World, nullptr);

	World->WorldType = EWorldType::Editor;
	World->AddToRoot();

	SetWorldInitializationValues(World);

	return World;
}

UWorld* FZkzAssetValidationUtils::PrepareWorldForInitByAsset(UObject* InAsset)
{
	UWorld* const World = Cast<UWorld>(InAsset);
	ZKZ_RETURN_IF_INVALID(World, nullptr);

	World->WorldType = EWorldType::Editor;

	SetWorldInitializationValues(World);

	return World;
}

void FZkzAssetValidationUtils::SetWorldInitializationValues(UWorld* World)
{
	ZKZ_RETURN_IF(World->bIsWorldInitialized);

	UWorld::InitializationValues InitializationValues;
	InitializationValues.RequiresHitProxies(false);
	InitializationValues.ShouldSimulatePhysics(false);
	InitializationValues.EnableTraceCollision(false);
	InitializationValues.CreateNavigation(false);
	InitializationValues.CreateAISystem(false);
	InitializationValues.AllowAudioPlayback(false);

	World->InitWorld(InitializationValues);
	World->PersistentLevel->UpdateModelComponents();
	World->UpdateWorldComponents(true, false);
	World->FlushLevelStreaming(EFlushLevelStreamingType::Full);
}

}  // namespace Zkz::Game::Validation
