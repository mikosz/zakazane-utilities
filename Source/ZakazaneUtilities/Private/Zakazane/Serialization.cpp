#include "Zakazane/Serialization.h"

#include "JsonObjectConverter.h"
#include "Misc/ScopeExit.h"
#include "Misc/SlowTask.h"
#include "Misc/StringOutputDevice.h"
#include "Serialization/Csv/CsvParser.h"
#include "UObject/Field.h"
#include "UObject/TextProperty.h"
#include "Zakazane/ContinueIfMacros.h"
#include "Zakazane/Logging.h"
#include "Zakazane/OutputDeviceStatsWrapper.h"
#include "Zakazane/ReturnIfMacros.h"

#define LOCTEXT_NAMESPACE "ZakazaneUtilitiesSerialization"

DEFINE_LOG_CATEGORY(LogZkzSerialization);

namespace Zkz::Serialization
{

FString GetSerializedString(const FProperty& Property, const void* Object)
{
	if (const FTextProperty* const TextProperty = ExactCastField<const FTextProperty>(&Property))
	{
		FText Text;
		TextProperty->GetValue_InContainer(Object, &Text);

		return Text.ToString();
	}
	else if (const FStrProperty* const StrProperty = ExactCastField<const FStrProperty>(&Property))
	{
		return StrProperty->GetPropertyValue_InContainer(Object);
	}
	else
	{
		FString String;
		Property.ExportText_Direct(String, Object, nullptr, nullptr, 0);

		return String;
	}
}

FString GetCSVSanitizedString(FString String)
{
	String = MoveTemp(String).Replace(TEXT(R"(")"), TEXT(R"("")"));	 // " -> ""

	return String;
}

FString ExportToCSV(const UStruct& Struct, const TArrayView<const void*> Objects)
{
	FStringBuilderBase Builder;

	for (TFieldIterator<FProperty> FieldIt{&Struct}; FieldIt; ++FieldIt)
	{
		const FProperty* const Property = *FieldIt;
		ZKZ_CONTINUE_IF_ENSUREALWAYS(Property == nullptr);

		Builder.Append(FString{*Property->GetName()}).Append(",");
	}

	Builder.Append("\n");

	for (const void* Object : Objects)
	{
		for (TFieldIterator<FProperty> FieldIt{&Struct}; FieldIt; ++FieldIt)
		{
			const FProperty* const Property = *FieldIt;
			ZKZ_CONTINUE_IF_ENSUREALWAYS(Property == nullptr);

			FString SerializedString = GetSerializedString(*Property, Object);
			Builder.Append(R"(")").Append(GetCSVSanitizedString(MoveTemp(SerializedString))).Append(R"(",)");
		}

		Builder.Append("\n");
	}

	return Builder.ToString();
}

EImportResult ImportFromCSV(
	const UStruct& Struct,
	const FString& CSV,
	const TFunction<void(const void*)>& LineCallback,
	FOutputDevice* const Output,
	FSlowTask* const SlowTask)
{
	const FCsvParser Parser{CSV};

	if (Parser.GetRows().IsEmpty())
	{
		if (Output != nullptr)
		{
			Output->Log(
				ELogVerbosity::Error, "Failed to parse CSV. Parser returned empty result. Is the imported file empty?");
		}
		return EImportResult::Error;
	}

	void* const Object = FMemory::Malloc(Struct.GetStructureSize(), Struct.GetMinAlignment());
	check(Object);

	ON_SCOPE_EXIT
	{
		FMemory::Free(Object);
	};

	const auto TickSlowTask = [SlowTask, Work = 1.0f / (Parser.GetRows().Num())]
	{
		if (SlowTask != nullptr)
		{
			SlowTask->EnterProgressFrame(Work);
			ZKZ_RETURN_IF(SlowTask->ShouldCancel(), false);
		}

		return true;
	};

	TArray<const FProperty*, TInlineAllocator<16>> Properties;

	ZKZ_RETURN_IF(!TickSlowTask(), EImportResult::CancelledByUser);
	for (const TCHAR* const Cell : Parser.GetRows()[0])
	{
		if (FStringView{Cell}.IsEmpty())
		{
			Properties.Emplace(nullptr);
			continue;
		}

		const FProperty* const Property = Struct.FindPropertyByName(Cell);
		if (Property == nullptr)
		{
			if (Output != nullptr)
			{
				Output->Log(
					ELogVerbosity::Error,
					FString::Format(TEXT("Invalid column name: {0}. Property not found."), {Cell}));
			}
			return EImportResult::Error;
		}

		Properties.Emplace(Property);
	}

	EImportResult AggregatedResult = EImportResult::Success;
	FTextBuilder AggregatedMessageBuilder;

	for (int32 RowIdx = 1; RowIdx < Parser.GetRows().Num(); ++RowIdx)
	{
		const TArray<const TCHAR*>& Row = Parser.GetRows()[RowIdx];

		ZKZ_RETURN_IF(!TickSlowTask(), EImportResult::CancelledByUser);

		Struct.InitializeStruct(Object);

		ON_SCOPE_EXIT
		{
			Struct.DestroyStruct(Object);
		};

		const int32 NumCells = Row.Num();
		if (NumCells != Properties.Num())
		{
			AggregatedResult = EImportResult::Warning;
			if (Output != nullptr)
			{
				Output->Log(
					ELogVerbosity::Warning,
					FString::Format(
						TEXT(
							"Warning in line {0}: invalid number of columns. Expected {1} but got {2}. Line ignored.\n"),
						{RowIdx + 1, NumCells, Properties.Num()}));
			}
			continue;
		}

		const bool bRowImportSuccessful = [&]
		{
			for (int32 CellIdx = 0; CellIdx < NumCells; CellIdx++)
			{
				const FProperty* const Property = Properties[CellIdx];
				const FStringView CellText = Row[CellIdx];

				if (Property == nullptr)  // column empty
				{
					if (!CellText.IsEmpty())
					{
						AggregatedResult = EImportResult::Warning;
						if (Output != nullptr)
						{
							Output->Log(
								ELogVerbosity::Warning,
								FString::Format(
									TEXT("Warning in line {0}, column {1}: expected empty column. Line ignored.\n"),
									{RowIdx + 1, CellIdx + 1}));
						}

						return false;
					}

					continue;
				}

				if (const FTextProperty* const TextProperty = ExactCastField<const FTextProperty>(Property))
				{
					const FText Text = FText::FromStringView(CellText);
					TextProperty->SetValue_InContainer(Object, Text);
				}
				else if (const FStrProperty* const StrProperty = ExactCastField<const FStrProperty>(Property))
				{
					StrProperty->SetValue_InContainer(Object, FString{CellText});
				}
				else
				{
					FStringOutputDevice PropertyImportOutput;
					FOutputDeviceStatsWrapper StatsWrapperOutput{&PropertyImportOutput};
					const FStringView RemainingCellText =
						Property->ImportText_Direct(CellText.GetData(), Object, nullptr, 0, &StatsWrapperOutput);

					// Property import logs on Log
					if (StatsWrapperOutput.GetNumMessagesWorseThan(ELogVerbosity::Log) > 0)
					{
						AggregatedResult = EImportResult::Warning;
						if (Output != nullptr)
						{
							Output->Log(
								ELogVerbosity::Warning,
								FString::Format(
									TEXT(
										"Warning in line {0}, column {1}: failed to import property. Line ignored. Detailed message: {2}\n"),
									{RowIdx + 1, CellIdx + 1, *PropertyImportOutput}));
						}
						return false;
					}
					else if (!RemainingCellText.IsEmpty())
					{
						AggregatedResult = EImportResult::Warning;
						if (Output != nullptr)
						{
							Output->Log(
								ELogVerbosity::Warning,
								FString::Format(
									TEXT("Warning in line {0}, column {1}: unexpected trailing text in value: {2}\n"),
									{RowIdx + 1, CellIdx + 1, RemainingCellText}));
						}
						return false;
					}
				}
			}

			return true;
		}();

		ZKZ_CONTINUE_IF(!bRowImportSuccessful);

		LineCallback(Object);
	}

	return AggregatedResult;
}

TResult<FString, FString> ExportToJSON(
	const UStruct& Struct, const TArrayView<const void*> Objects, int64 CheckFlags, int64 SkipFlags, bool bStopOnError)
{
	TArray<TSharedPtr<FJsonValue>> JsonValueArray;
	JsonValueArray.Reserve(Objects.Num());

	for (int32 Index = 0; Index < Objects.Num(); ++Index)
	{
		const void* const ObjPtr = Objects[Index];

		if (ObjPtr == nullptr)
		{
			if (bStopOnError)
			{
				return Err(FString::Format(TEXT("Export aborted: Object at index {0} is a null pointer."), {Index}));
			}

			LogUserError(
				LogZkzSerialization,
				EMessageSeverity::Warning,
				FString::Format(TEXT("ExportToJSON: Object at index {0} is a null pointer. Skipping."), {Index}));
			continue;
		}

		TSharedRef<FJsonObject> JsonObj = MakeShared<FJsonObject>();

		const bool bSuccess =
			FJsonObjectConverter::UStructToJsonObject(&Struct, ObjPtr, JsonObj, CheckFlags, SkipFlags);
		if (!bSuccess)
		{
			if (bStopOnError)
			{
				return Err(
					FString::Format(
						TEXT("Export aborted: FJsonObjectConverter failed to serialize object at index {0}."),
						{Index}));
			}

			LogUserError(
				LogZkzSerialization,
				EMessageSeverity::Warning,
				FString::Format(TEXT("ExportToJSON: FJsonObjectConverter failed to serialize object at index {0}. Skipping."), {Index}));
			continue;
		}

		JsonValueArray.Emplace(MakeShared<FJsonValueObject>(MoveTemp(JsonObj)));
	}

	FString OutputJsonString;
	const bool bSerializeSuccess =
		FJsonSerializer::Serialize(JsonValueArray, TJsonWriterFactory<>::Create(&OutputJsonString));

	if (!bSerializeSuccess)
	{
		return Err(TEXT("Export aborted: FJsonSerializer::Serialize failed to construct the final JSON string."));
	}

	return Ok(OutputJsonString);
}

}  // namespace Zkz::Serialization

#undef LOCTEXT_NAMESPACE
