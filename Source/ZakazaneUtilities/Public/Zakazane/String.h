// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GameplayTagContainer.h"
#include "Templates/Invoke.h"

struct FGameplayTag;

namespace Zkz::String
{

enum class ESkipEmptySegments
{
	Yes,
	No
};

/// Iterates over segments of the given string delimited by the given character. Calls Func for each segment.
template <class CharType, class FuncType>
void ForEachSegment(
	TStringView<CharType> String,
	CharType Delimiter,
	FuncType&& Func,
	ESkipEmptySegments SkipEmptySegments = ESkipEmptySegments::Yes);

/// Splits the given string using the given delimiter and puts the segments into the given range.
template <class RangeType, class CharType>
RangeType ParseInto(
	TStringView<CharType> String, CharType Delimiter, ESkipEmptySegments SkipEmptySegments = ESkipEmptySegments::Yes);

/// @return Elems rightmost segments of the given string delimited by the given character. E.g. for x.y.z
/// * Elems == 1 will return "z"
/// * Elems == 2 will return "y.z"
/// * Elems == 3 will return "x.y.z"
/// * Elems == 4 will return "x.y.z" (as there are only 3 segments)
ZAKAZANEUTILITIES_API FStringView GetRightmostSegments(const FStringView& String, TCHAR Delimiter, int32 Elems);

/// @returns Elems rightmost segments of the given tag. E.g. for x.y.z
/// * Elems == 1 will return "z"
/// * Elems == 2 will return "y.z"
/// * Elems == 3 will return "x.y.z"
/// * Elems == 4 will return "x.y.z" (as there are only 3 segments)
ZAKAZANEUTILITIES_API FString GetTagSuffix(const FGameplayTag& Tag, int32 Elems = 2);

/// @returns "True" or "False" depending on bValue
ZAKAZANEUTILITIES_API FString BoolToString(bool bValue);

/// @returns "True" or "False" depending on bValue
ZAKAZANEUTILITIES_API FString IntToString(int32 Value, bool AddSign = false);

template <class CharType>
const TStringView<CharType> Ellipsis;

template <>
constexpr TStringView<ANSICHAR> Ellipsis<ANSICHAR>{"..."};

template <>
const TStringView<UTF8CHAR> Ellipsis<UTF8CHAR> = UTF8TEXTVIEW("...");

template <>
constexpr TStringView<TCHAR> Ellipsis<TCHAR>{TEXT("...")};

// #TODO #String: would be nice to use a concept for StringType so that it enforces use of
// a valid concrete string. Also to template over character type, not over string type,
// which would make it obvious what kind of argument we want. This would also allow to call
// with a string view argument, which would implicitly convert to a string type.

/// Abbreviates String to the given length. Note that this function attempts to shorten to MaxLength, i.e.
///	the ending length is also subtracted from string when abbreviating. Ending is never shortened, so
///	if MaxLength is less than Ending.Len(), Ending is returned.
template <class StringType>
StringType Abbreviate(
	StringType String,
	int32 MaxLength,
	TStringView<typename StringType::ElementType> Ending = Ellipsis<typename StringType::ElementType>);

}  // namespace Zkz::String

// -- template implementations

namespace Zkz::String
{

template <class CharType, class FuncType>
void ForEachSegment(
	TStringView<CharType> String, const CharType Delimiter, FuncType&& Func, const ESkipEmptySegments SkipEmptySegments)
{
	for (;;)
	{
		int32 Index;
		if (!String.FindChar(Delimiter, Index))
		{
			if (!String.IsEmpty() || SkipEmptySegments == ESkipEmptySegments::No)
			{
				::Invoke(Func, String);
			}
			return;
		}

		if (Index > 0 || SkipEmptySegments == ESkipEmptySegments::No)
		{
			::Invoke(Func, String.Left(Index));
		}

		String = String.RightChop(Index + 1);
	}
}

template <class RangeType, class CharType>
RangeType ParseInto(
	const TStringView<CharType> String, const CharType Delimiter, const ESkipEmptySegments SkipEmptySegments)
{
	RangeType Range;
	ForEachSegment(String, Delimiter, [&Range](const auto& Segment) { Range.Emplace(Segment); }, SkipEmptySegments);
	return Range;
}

template <class StringType>
StringType Abbreviate(
	StringType String, const int32 MaxLength, const TStringView<typename StringType::ElementType> Ending)
{
	if (String.Len() <= MaxLength)
	{
		return String;
	}

	if (Ending.Len() > MaxLength)
	{
		return StringType::ConstructFromPtrSize(Ending.GetData(), Ending.Len());
	}

	return String.Left(MaxLength - Ending.Len()) + Ending;
}

}  // namespace Zkz::String
