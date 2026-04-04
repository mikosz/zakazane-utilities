// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Logging/TokenizedMessage.h"
#include "Subsystems/EngineSubsystem.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "Logging.generated.h"

UCLASS()
class UZkzLogSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	// TODO: #Log add blueprint callable function for user error logs
	ZAKAZANEUTILITIES_API
	void LogUserError(
		const FLogCategoryBase& LogCategory,
		const EMessageSeverity::Type Severity,
		const FString& MessageStr,
		TArrayView<const UObject*> ContextObjects = {},
		const bool bTryPointToSourceObject = true);

#if NO_LOGGING
	ZAKAZANEUTILITIES_API void LogUserError(
		const FNoLoggingCategory& LogCategory,
		const EMessageSeverity::Type Severity,
		const FString& MessageStr,
		TArrayView<const UObject*> ContextObjects = {},
		const bool bTryPointToSourceObject = true);
#endif

private:
#if WITH_EDITOR
	FString ConstructNotificationErrorString();

	TSharedPtr<SNotificationItem> MessageNotificationActive;
	TArray<FString> MessageNotificationsToDisplay;
#endif
};

namespace Zkz
{
#if NO_LOGGING
void LogToScreenAndConsole(const FNoLoggingCategory& Category, ELogVerbosity::Type Verbosity, const FString& Message);
#endif

ZAKAZANEUTILITIES_API
void LogToScreenAndConsole(const FLogCategoryBase& Category, ELogVerbosity::Type Verbosity, const FString& Message);

/// Logs a user configuration-related error message to the console, screen, and message log.
///
/// This function displays error notifications in both the message log and
/// as a system notification (if applicable). It helps provide contextual
/// information about the error by optionally associating it with a specific
/// UObject. Additionally, it will create a user-facing notification for prominent visibility.
///
/// @param LogCategory The logging category to which the error message belongs.
///					   This category should be also registered for MessageLog with the same name, so the log shows the in correct category
/// @param Severity
/// @param MessageStr String of the error message to be logged.
/// @param ContextObject Optional UObject used to provide context or as a reference
///                      in logs. The associated object will be linked in the message for users.
/// @param bTryPointToSourceObject If true, links to the editor counterpart of ContextObject; if false,
///                                            associates the error directly with the ContextObject.
ZAKAZANEUTILITIES_API
void LogUserError(
	const FLogCategoryBase& LogCategory,
	const EMessageSeverity::Type Severity,
	const FString& MessageStr,
	const UObject* ContextObject = nullptr,
	const bool bTryPointToSourceObject = true);

#if NO_LOGGING
ZAKAZANEUTILITIES_API void LogUserError(
	const FNoLoggingCategory& LogCategory,
	const EMessageSeverity::Type Severity,
	const FString& MessageStr,
	const UObject* ContextObject = nullptr,
	const bool bTryPointToSourceObject = true);
#endif

/// LogUserError variant allowing to link multiple context objects.
ZAKAZANEUTILITIES_API
void LogUserError(
	const FLogCategoryBase& LogCategory,
	const EMessageSeverity::Type Severity,
	const FString& MessageStr,
	TArrayView<const UObject*> ContextObjects,
	const bool bTryPointToSourceObject = true);

#if NO_LOGGING
ZAKAZANEUTILITIES_API void LogUserError(
	const FNoLoggingCategory& LogCategory,
	const EMessageSeverity::Type Severity,
	const FString& MessageStr,
	TArrayView<const UObject*> ContextObjects,
	const bool bTryPointToSourceObject = true);
#endif
}  // namespace Zkz