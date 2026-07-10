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
	ZAKAZANEUTILITIES_API
	void LogUserError(
		const FLogCategoryBase& LogCategory,
		const EMessageSeverity::Type Severity,
		const FString& MessageStr,
		TConstArrayView<const UObject*> ContextObjects = {},
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

DECLARE_LOG_CATEGORY_EXTERN(LogZkzBlueprints, Log, All)

/// Exposes EMessageSeverity::Type to blueprints
UENUM(BlueprintType)
enum class EZkzMessageSeverity : uint8
{
	// Expected to match EMessageSeverity::Type

	CriticalError UE_DEPRECATED(
		5.1, "CriticalError was removed because it can't trigger an assert at the callsite. Use 'checkf' instead.") = 0,
	Error = 1,
	PerformanceWarning = 2,
	Warning = 3,
	Info = 4,  // Should be last
};

UCLASS(ClassGroup = "Zakazane|Logging")
class ZAKAZANEUTILITIES_API UZkzLoggingBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/// Logs a user configuration-related error message to the console, screen, and message log.
	///
	/// This function displays error notifications in both the message log and
	/// as a system notification (if applicable). It helps provide contextual
	/// information about the error by optionally associating it with a specific
	/// UObject. Additionally, it will create a user-facing notification for prominent visibility.
	///
	/// @param Severity
	/// @param MessageStr String of the error message to be logged.
	/// @param ContextObject Optional UObject used to provide context or as a reference
	///                      in logs. The associated object will be linked in the message for users.
	/// @param bTryPointToSourceObject If true, links to the editor counterpart of ContextObject; if false,
	///                                            associates the error directly with the ContextObject.
	UFUNCTION(BlueprintCallable, Category = "Zakazane|Logging")
	static void LogUserError(
		EZkzMessageSeverity Severity,
		const FString& MessageStr,
		const UObject* ContextObject = nullptr,
		const bool bTryPointToSourceObject = true);
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
	TConstArrayView<const UObject*> ContextObjects,
	const bool bTryPointToSourceObject = true);

#if NO_LOGGING
ZAKAZANEUTILITIES_API void LogUserError(
	const FNoLoggingCategory& LogCategory,
	const EMessageSeverity::Type Severity,
	const FString& MessageStr,
	TArrayView<const UObject*> ContextObjects,
	const bool bTryPointToSourceObject = true);
#endif

template <class T>
struct TIsLogCategory
{
#if NO_LOGGING
	static constexpr bool Value = std::is_same_v<std::remove_cv_t<T>, FNoLoggingCategory>;
#else
private:
	// There's no simple way in c++ to make a template match all template implementations of a given class
	// (i.e., you can match "FLogCategory<ELogVerbosity::Warning, ELogVerbosity::All>", but not
	// "FLogCategory"). The trick with the overloaded Test function is that calling it for a FLogCategory
	// implementation will match the version returning true_type, and for any other type will match the
	// version returning false_type. Value is then evaluated based on the return type of the matched function.

	template <ELogVerbosity::Type DefaultVerbosity, ELogVerbosity::Type CompileTimeVerbosity>
	// ReSharper disable once CppFunctionIsNotImplemented
	static std::true_type Test(const FLogCategory<DefaultVerbosity, CompileTimeVerbosity>*);

	// ReSharper disable once CppFunctionIsNotImplemented
	static std::false_type Test(...);

public:
	static constexpr bool Value = decltype(Test(std::declval<T*>()))::value;
#endif
};

template <class T>
concept CLogCategory = TIsLogCategory<T>::Value;

#if NO_LOGGING

template <CLogCategory LogCategoryType>
constexpr ELogVerbosity::Type GetCompileTimeVerbosity()
{
	return ELogVerbosity::Fatal;
}

constexpr ELogVerbosity::Type GetDefaultVerbosity(const FNoLoggingCategory* const LogCategory)
{
	return ELogVerbosity::Fatal;
}

inline FName GetLogCategoryName(const FNoLoggingCategory& LogCategory)
{
	return FName{"NO_LOGGING"};
}

#else

template <CLogCategory LogCategoryType>
constexpr ELogVerbosity::Type GetCompileTimeVerbosity()
{
	return LogCategoryType::CompileTimeVerbosity;
}
template <ELogVerbosity::Type InDefaultVerbosity, ELogVerbosity::Type InCompileTimeVerbosity>
constexpr ELogVerbosity::Type GetDefaultVerbosity(
	const FLogCategory<InDefaultVerbosity, InCompileTimeVerbosity>* const LogCategory)
{
	return InDefaultVerbosity;
}

template <CLogCategory LogCategoryType>
constexpr decltype(auto) GetLogCategoryName(const LogCategoryType& LogCategory)
{
	return LogCategory.GetCategoryName();
}

#endif

/// Allows referencing user-provided log categories in a way compatible with UE_LOG.
template <CLogCategory InLogCategoryType>
class TLogCategoryRef : public FLogCategory<
							GetDefaultVerbosity(reinterpret_cast<const InLogCategoryType*>(nullptr)),
							Zkz::GetCompileTimeVerbosity<InLogCategoryType>()>
{
public:
	using LogCategoryType = InLogCategoryType;

	static constexpr ELogVerbosity::Type GetCompileTimeVerbosity();

	explicit TLogCategoryRef(const LogCategoryType& InLogCategory);

	// ReSharper disable once CppNonExplicitConversionOperator
	operator const LogCategoryType&() const;

private:
	TNonNullPtr<const LogCategoryType> LogCategory;
};

// -- template implementations

template <CLogCategory InLogCategoryType>
constexpr ELogVerbosity::Type TLogCategoryRef<InLogCategoryType>::GetCompileTimeVerbosity()
{
	return Zkz::GetCompileTimeVerbosity<InLogCategoryType>();
}

template <CLogCategory InLogCategoryType>
TLogCategoryRef<InLogCategoryType>::TLogCategoryRef(const LogCategoryType& InLogCategory)
	: FLogCategory<
		  GetDefaultVerbosity(reinterpret_cast<const InLogCategoryType*>(nullptr)),
		  Zkz::GetCompileTimeVerbosity<InLogCategoryType>()>{GetLogCategoryName(InLogCategory)}
	, LogCategory{&InLogCategory}
{
}

template <CLogCategory InLogCategoryType>
TLogCategoryRef<InLogCategoryType>::operator const InLogCategoryType&() const
{
	return *LogCategory;
}

#if NO_LOGGING

#define ZKZ_DECLARE_LOG_CATEGORY_CLASS(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
	static constexpr FNoLoggingCategory CategoryName;

#else

/// Same as DECLARE_LOG_CATEGORY_CLASS but introduces a static member FNoLoggingCategory CategoryName for non-logging
/// contexts, to be able to reference it in code without #if NO_LOGGING.
#define ZKZ_DECLARE_LOG_CATEGORY_CLASS(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
	DECLARE_LOG_CATEGORY_CLASS(CategoryName, DefaultVerbosity, CompileTimeVerbosity)

#endif

}  // namespace Zkz
