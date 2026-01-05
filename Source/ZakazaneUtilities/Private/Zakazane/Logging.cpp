#include "Zakazane/Logging.h"

#include "Engine/Engine.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Zakazane/Object.h"
#include "Zakazane/ReturnIfMacros.h"

#if WITH_EDITOR
#include "MessageLogModule.h"
#endif

namespace Logging::Private
{
#if WITH_EDITOR

FString GetReadableContextObjectName(const UObject& Object)
{
	if (const AActor* ActorObject = Cast<AActor>(&Object))
	{
		return ActorObject->GetActorLabel();
	}

	if (const UActorComponent* ComponentObject = Cast<UActorComponent>(&Object))
	{
		const AActor* const ComponentOwnerActor = ComponentObject->GetOwner();
		if (IsValid(ComponentOwnerActor))
		{
			return FString::Printf(TEXT("%s / %s"), *ComponentOwnerActor->GetActorLabel(), *ComponentObject->GetName());
		}
	}

	return Object.GetName();
}

#endif
}  // namespace Logging::Private

void UZkzLogSubsystem::LogUserError(
	const FLogCategoryBase& LogCategory,
	const EMessageSeverity::Type Severity,
	const FString& MessageStr,
	const UObject* ContextObject,
	const bool bTryPointToSourceObject)
{
#if WITH_EDITOR
	const FName LogCategoryName = LogCategory.GetCategoryName();

	// suppressing because logging is done independently, so it's universal for editor and build
	FMessageLog& MessageLogInstance = FMessageLog{LogCategoryName}.SuppressLoggingToOutputLog();

	TSharedRef<FTokenizedMessage> TokenizedMessage =
		MessageLogInstance.AddMessage(FTokenizedMessage::Create(Severity, FText::FromString(MessageStr)));

	if (IsValid(ContextObject))
	{
		const UObject* ObjectToLink = [&]
		{
			if (bTryPointToSourceObject)
			{
				const UObject* EditorCounterpartObject = Zkz::Editor::TryGetEditorCounterpartObject(*ContextObject);
				if (IsValid(EditorCounterpartObject))
				{
					return EditorCounterpartObject;
				}
			}

			return ContextObject;
		}();

		const FString ObjectToLinkName = Logging::Private::GetReadableContextObjectName(*ObjectToLink);
		TokenizedMessage->AddToken(FUObjectToken::Create(ObjectToLink, FText::FromString(ObjectToLinkName)));
	}

	if (!MessageNotificationActive.IsValid()
		|| MessageNotificationActive->GetCompletionState() == SNotificationItem::CS_Success)
	{
		// Add notification
		FNotificationInfo Info(FText::FromString(TEXT("Error occured")));
		Info.SubText = FText::FromString(MessageStr);
		Info.bFireAndForget = false;
		Info.bUseThrobber = false;
		Info.ExpireDuration = 0;
		Info.FadeInDuration = 0.1f;
		Info.FadeOutDuration = 0.1f;
		Info.bUseCopyToClipboard = true;

		FNotificationButtonInfo OpenMessageLogButtonInfo = FNotificationButtonInfo(
			FText::FromString(TEXT("Open Message Log")),
			FText::FromString(TEXT("")),
			FSimpleDelegate::CreateLambda(
				[LogCategoryName, this]
				{
					FMessageLogModule& MessageLogModule =
						FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
					MessageLogModule.OpenMessageLog(LogCategoryName);

					if (MessageNotificationActive.IsValid())
					{
						MessageNotificationActive->ExpireAndFadeout();
						MessageNotificationActive->SetCompletionState(SNotificationItem::CS_Success);
						MessageNotificationActive.Reset();
					}
				}),
			SNotificationItem::CS_None);

		FNotificationButtonInfo CloseNotificationButtonInfo = FNotificationButtonInfo(
			FText::FromString(TEXT("Close")),
			FText::FromString(TEXT("")),
			FSimpleDelegate::CreateLambda(
				[this]
				{
					if (MessageNotificationActive.IsValid())
					{
						MessageNotificationActive->ExpireAndFadeout();
						MessageNotificationActive->SetCompletionState(SNotificationItem::CS_Success);
						MessageNotificationActive.Reset();
					}
				}),
			SNotificationItem::CS_None);

		Info.ButtonDetails.Add(MoveTemp(OpenMessageLogButtonInfo));
		Info.ButtonDetails.Add(MoveTemp(CloseNotificationButtonInfo));

		MessageNotificationActive = FSlateNotificationManager::Get().AddNotification(Info);

		MessageNotificationsToDisplay.Empty();
		MessageNotificationsToDisplay.Emplace(MessageStr);
	}
	else
	{
		MessageNotificationActive->Pulse(FLinearColor::Red);

		MessageNotificationsToDisplay.Emplace(MessageStr);
		if (MessageNotificationsToDisplay.Num() > 10)
		{
			MessageNotificationsToDisplay.RemoveAt(0);
		}

		MessageNotificationActive->SetText(
			FText::FromString(FString::Printf(TEXT("Error occured (%d)"), MessageNotificationsToDisplay.Num())));

		MessageNotificationActive->SetSubText(FText::FromString(ConstructNotificationErrorString()));
	}

	MessageLogInstance.Flush();
#endif

	// Log to console
	const TCHAR* const LogColor = FMessageLog::GetLogColor(Severity);
	if (LogColor)
	{
		SET_WARN_COLOR(LogColor);
	}

	FMsg::Logf(
		__FILE__,
		__LINE__,
		LogCategory.GetCategoryName(),
		FMessageLog::GetLogVerbosity(Severity),
		TEXT("%s"),
		*MessageStr);

	CLEAR_WARN_COLOR();
}

#if NO_LOGGING
ZAKAZANEUTILITIES_API void LogUserError(
	const FNoLoggingCategory& LogCategory,
	const EMessageSeverity::Type Severity,
	const FString& MessageStr,
	const UObject* ContextObject,
	const bool bTryPointToSourceObject)
{
}
#endif

#if WITH_EDITOR
FString UZkzLogSubsystem::ConstructNotificationErrorString()
{
	TStringBuilder<1024> StringBuilder;
	for (int i = 0; i < MessageNotificationsToDisplay.Num(); ++i)
	{
		StringBuilder << MessageNotificationsToDisplay[i];

		if (i < MessageNotificationsToDisplay.Num() - 1)
		{
			StringBuilder << "\n\n";
		}
	}

	FString Result = StringBuilder.ToString();
	return Result;
}
#endif

namespace Zkz
{
#if NO_LOGGING
void LogToScreenAndConsole(const FNoLoggingCategory& Category, ELogVerbosity::Type Verbosity, const FString& Message)
{
}
#endif

void LogToScreenAndConsole(const FLogCategoryBase& Category, ELogVerbosity::Type Verbosity, const FString& Message)
{
	if (IsValid(GEngine))
	{
		const FColor Color = [Verbosity]
		{
			switch (Verbosity)
			{
				case ELogVerbosity::Fatal:
					[[fallthrough]];
				case ELogVerbosity::Error:
					return FColor::Red;
				case ELogVerbosity::Warning:
					return FColor::Yellow;
				default:
					return FColor::White;
			}
		}();

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, Color, Message);
	}

	switch (Verbosity)
	{
		case ELogVerbosity::Fatal:
			UE_LOG_REF(Category, Fatal, TEXT("%s"), *Message);
			break;
		case ELogVerbosity::Error:
			UE_LOG_REF(Category, Error, TEXT("%s"), *Message);
			break;
		case ELogVerbosity::Warning:
			UE_LOG_REF(Category, Warning, TEXT("%s"), *Message);
			break;
		case ELogVerbosity::Display:
			UE_LOG_REF(Category, Display, TEXT("%s"), *Message);
			break;
		case ELogVerbosity::Log:
			UE_LOG_REF(Category, Log, TEXT("%s"), *Message);
			break;
		case ELogVerbosity::Verbose:
			UE_LOG_REF(Category, Verbose, TEXT("%s"), *Message);
			break;
		case ELogVerbosity::VeryVerbose:
			UE_LOG_REF(Category, VeryVerbose, TEXT("%s"), *Message);
			break;
		default:;
			UE_LOG_REF(Category, Error, TEXT("[unexpected verbosity] %s"), *Message);
			break;
	}
}

void LogUserError(
	const FLogCategoryBase& LogCategory,
	const EMessageSeverity::Type Severity,
	const FString& MessageStr,
	const UObject* ContextObject,
	const bool bTryPointToSourceObject)
{
	ZKZ_RETURN_IF_INVALID(GEngine);

	UZkzLogSubsystem* const LogSubsystem = GEngine->GetEngineSubsystem<UZkzLogSubsystem>();
	ZKZ_RETURN_IF_INVALID(LogSubsystem);

	LogSubsystem->LogUserError(LogCategory, Severity, MessageStr, ContextObject, bTryPointToSourceObject);
}

#if NO_LOGGING
ZAKAZANEUTILITIES_API void LogUserError(
	const FNoLoggingCategory& LogCategory,
	const EMessageSeverity::Type Severity,
	const FString& MessageStr,
	const UObject* ContextObject,
	const bool bTryPointToSourceObject)
{
}
#endif
}  // namespace Zkz