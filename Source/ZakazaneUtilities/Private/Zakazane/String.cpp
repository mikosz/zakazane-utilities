#include "Zakazane/String.h"

#include "GameplayTagContainer.h"
#include "Zakazane/ReturnIfMacros.h"

namespace Zkz::String
{

FStringView GetRightmostSegments(const FStringView& String, const TCHAR Delimiter, const int32 Elems)
{
	FStringView ParentTagStringView = String;

	// Invariants:
	// - ParentTagString view contains the tag after removing Elems - Elem elements (sans the . at the end)
	for (int32 Elem = Elems; Elem > 0; --Elem)
	{
		int32 DelimiterIdx;
		const bool bResult = ParentTagStringView.FindLastChar(Delimiter, DelimiterIdx);
		if (!bResult)
		{
			ParentTagStringView.Reset();
			break;
		}

		ParentTagStringView = ParentTagStringView.Left(DelimiterIdx);
	}

	// Result is the tag string suffix after removing the ParentTagStringView
	return ParentTagStringView.IsEmpty() ? String : String.RightChop(ParentTagStringView.Len() + 1);
}

FString GetTagSuffix(const FGameplayTag& Tag, const int32 Elems)
{
	const FString TagString = Tag.ToString();
	return FString{GetRightmostSegments(TagString, TCHAR{'.'}, Elems)};
}

FString BoolToString(const bool bValue)
{
	if (bValue)
	{
		return TEXT("True");
	}
	return TEXT("False");
}

FString IntToString(int32 Value, bool AddSign)
{
	ZKZ_RETURN_IF(!AddSign,FString::FromInt(Value));
	ZKZ_RETURN_IF(Value < 0,FString::FromInt(Value));
	return FString::Format(TEXT("+{0}"),{Value});
}

}  // namespace Zkz::String
