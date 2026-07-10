// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Algo/Accumulate.h"
#include "ReturnIfMacros.h"

namespace Zkz
{

class FMultisourceIdType;

ZAKAZANEUTILITIES_API uint32 GetTypeHash(const FMultisourceIdType& MultisourceId);
ZAKAZANEUTILITIES_API bool operator==(const FMultisourceIdType& Lhs, const FMultisourceIdType& Rhs);

// #TODO #Multisource: it would be convienient to be able to allow counted entries, perhaps without any additional id.
// E.g., one system adds three entries setting an "if any" value, then another removes them, without an id. The value
// changes when the number of overrides is 0.

/// Variant type for multisource value source id types.
class ZAKAZANEUTILITIES_API FMultisourceIdType
{
public:
	// #TODO #Multisource: implicit cast here is quite ugly. Would be better to have explicit constructors and maye
	// even force some static function "FromAnyPointer", "FromObjectPointer" or something to avoid accidental
	// passing of a void* instead of a UObject*.

	/*implicit*/ FMultisourceIdType(const void* Ptr);

	/*implicit*/ FMultisourceIdType(const UObject* Object UE_LIFETIMEBOUND);

	/*implicit*/ FMultisourceIdType(const FName Name UE_LIFETIMEBOUND);

	template <class StringType>
	StringType ToString() const;

	/// Works only for const UObject* and FName sources, for const void* returns Name_NONE.
	FName GetName() const;

private:
	using IdType = TVariant<const void*, const UObject*, FName>;

	IdType Id;

	friend uint32 Zkz::GetTypeHash(const FMultisourceIdType& MultisourceId);

	friend bool Zkz::operator==(const FMultisourceIdType& Lhs, const FMultisourceIdType& Rhs);

	template <class StringType>
	static StringType IdToString(const void* Ptr);

	template <class StringType>
	static StringType IdToString(const UObject* Object);

	template <class StringType>
	static StringType IdToString(const FName Name);
};

/// TMultisourceValue is a solution for values that may be modified by many different sources and need some sort
/// of custom conflict resolution when multiple sources are active.
/// E.g. movement speed - this may be modified by various rules, such as - move slower indoors, move slower in
/// buildings, move slower when wounded, move faster in combat or in a dialogue. In this case we'd probably want
/// some sort of priority based stack, where modifiers would be pushed onto it, but only the ones with the
/// highest priority would be applied.
template <class InValueType, template <class, class, class...> class InResolverType, class... ResolverArgs>
class TMultisourceValue : InResolverType<InValueType, TMultisourceValue<InValueType, InResolverType>, ResolverArgs...>
{
public:
	using ValueType = InValueType;
	using ResolverType = InResolverType<InValueType, TMultisourceValue<InValueType, InResolverType>, ResolverArgs...>;

	TMultisourceValue() = default;

	explicit TMultisourceValue(ValueType InDefaultValue) : DefaultValue{MoveTemp(InDefaultValue)}
	{
	}

	void SetDefaultValue(ValueType InDefaultValue)
	{
		DefaultValue = MoveTemp(InDefaultValue);
	}

	const ValueType& GetDefaultValue() const
	{
		return DefaultValue;
	}

	using ResolverType::GetSources;
	using ResolverType::GetValue;
	using ResolverType::PopValue;
	using ResolverType::PushValue;
	using ResolverType::Reset;

private:
	ValueType DefaultValue;

	friend ResolverType;  // need to befriend resolver so that it may static_cast to TMultisourceValue
};

// -- Resolvers

namespace MultisourceValue
{

/// Returns DefaultValue if there are no sources added, !DefaultValue otherwise. E.g. we want to know
/// whether an object is highlighted. DefaultValue is false, then if anyone does PushValue() we return
/// true.
template <class InValueType, class MultisourceValueType UE_REQUIRES(std::is_same_v<InValueType, bool>)>
class TIfAnyResolver
{
public:
	using ValueType = bool;

	void Reset()
	{
		Sources.Reset();
	}

	void PushValue(FMultisourceIdType Source)
	{
		Sources.Emplace(MoveTemp(Source));
	}

	void PopValue(const FMultisourceIdType Source)
	{
		Sources.Remove(Source);
	}

	bool GetValue() const
	{
		const bool bDefaultValue = static_cast<const MultisourceValueType&>(*this).GetDefaultValue();
		return Sources.IsEmpty() ? bDefaultValue : !bDefaultValue;
	}

	const TSet<FMultisourceIdType>& GetSources() const
	{
		return Sources;
	}

private:
	TSet<FMultisourceIdType> Sources;
};

/// Value sources are added with a value. The minimum value from the sources or the default value is returned.
/// @tparam InValueType - automatically filled in by TMultisourceValue: this is the value type to be resolved
/// @tparam MultisourceValueType - child TMultisourceValue (CRTP)
template <class InValueType, class MultisourceValueType>
class TMinimumResolver
{
public:
	using ValueType = InValueType;

	void Reset()
	{
		Sources.Reset();
	}

	void PushValue(FMultisourceIdType Source, ValueType Value)
	{
		Sources.Emplace(Source, Value);
	}

	void PopValue(FMultisourceIdType Source)
	{
		Sources.RemoveAllSwap([Source](const TPair<FMultisourceIdType, ValueType>& Entry)
							  { return Entry.Key == Source; });
	}

	ValueType GetValue() const
	{
		ValueType DefaultValue = static_cast<const MultisourceValueType&>(*this).GetDefaultValue();

		const TPair<FMultisourceIdType, ValueType>* SourceEntryPtr =
			Algo::MinElementBy(Sources, &TPair<FMultisourceIdType, ValueType>::Value);
		if (SourceEntryPtr == nullptr)
		{
			return DefaultValue;
		}

		return FMath::Min(DefaultValue, SourceEntryPtr->Value);
	}

	const TArray<TPair<FMultisourceIdType, ValueType>>& GetSources() const
	{
		return Sources;
	}

private:
	TArray<TPair<FMultisourceIdType, ValueType>> Sources;
};

/// Value sources are added with a value. The last value from the sources or the default value is returned.
/// @tparam InValueType - automatically filled in by TMultisourceValue: this is the value type to be resolved
/// @tparam MultisourceValueType - child TMultisourceValue (CRTP)
template <class InValueType, class MultisourceValueType>
class TLastResolver
{
public:
	using ValueType = InValueType;

	void Reset()
	{
		Sources.Reset();
	}

	void PushValue(FMultisourceIdType Source, ValueType Value)
	{
		Sources.Emplace(Source, Value);
	}

	void PopValue(FMultisourceIdType Source)
	{
		Sources.RemoveAllSwap([Source](const TPair<FMultisourceIdType, ValueType>& Entry)
							  { return Entry.Key == Source; });
	}

	ValueType GetValue() const
	{
		if (Sources.IsEmpty())
		{
			return static_cast<const MultisourceValueType&>(*this).GetDefaultValue();
		}

		return Sources.Last().Value;
	}

	const TArray<TPair<FMultisourceIdType, ValueType>>& GetSources() const
	{
		return Sources;
	}

private:
	TArray<TPair<FMultisourceIdType, ValueType>> Sources;
};

/// Value sources are added with a priority value. Value from the source with the highest priority is returned. If
/// multiple sources have the same priority, the one added the most recently wins.
/// @tparam InValueType - automatically filled in by TMultisourceValue: this is the value type to be resolved
/// @tparam MultisourceValueType - child TMultisourceValue (CRTP)
template <class InValueType, class MultisourceValueType>
class TPriorityBasedResolver
{
public:
	using ValueType = InValueType;
	using SourceIdType = int32;
	using PriorityType = int32;

	// #TODO #Multisource: shouldn't priority based resolver take id and priority instead of generating?
	struct FSourceEntry
	{
		SourceIdType SourceId = -1;

		PriorityType Priority = -1;

		ValueType Value;
	};

	void Reset()
	{
		Sources.Reset();
	}

	/// Pushes the given value onto the stack with the given priority.
	/// @tparam PriorityArgType - arbitrary priority value type, must be static_cast-able to PriorityType (int32).
	///		This is templated to allow seamless use of enum priorities.
	template <class PriorityArgType>
	SourceIdType PushValue(ValueType Value, PriorityArgType Priority)
	{
		SourceIdType SourceId = ++LastSourceId;
		Sources.Emplace(FSourceEntry{SourceId, static_cast<PriorityType>(Priority), MoveTemp(Value)});
		return SourceId;
	}

	void PopValue(SourceIdType SourceId)
	{
		Sources.RemoveAllSwap([SourceId](const FSourceEntry& Source) { return Source.SourceId == SourceId; });
	}

	ValueType GetValue() const
	{
		if (Sources.IsEmpty())
		{
			return static_cast<const MultisourceValueType&>(*this).GetDefaultValue();
		}

		const FSourceEntry* MaxPriorityEntry = nullptr;
		for (const FSourceEntry& Entry : Sources)
		{
			if (MaxPriorityEntry == nullptr || Entry.Priority > MaxPriorityEntry->Priority
				|| (Entry.Priority == MaxPriorityEntry->Priority && Entry.SourceId > MaxPriorityEntry->SourceId))
			{
				MaxPriorityEntry = &Entry;
			}
		}

		return MaxPriorityEntry->Value;
	}

	const TArray<FSourceEntry>& GetSources() const
	{
		return Sources;
	}

private:
	SourceIdType LastSourceId = 0;

	TArray<FSourceEntry> Sources;
};

/// All value sources are summed up.
/// @tparam InValueType - automatically filled in by TMultisourceValue: this is the value type to be resolved
/// @tparam MultisourceValueType - child TMultisourceValue (CRTP)
template <class InValueType, class MultisourceValueType>
class TSumResolver
{
public:
	using ValueType = InValueType;

	struct FSourceEntry
	{
		FMultisourceIdType SourceId;

		ValueType Value;
	};

	void Reset()
	{
		Sources.Reset();
	}

	void PushValue(FMultisourceIdType SourceId, ValueType Value)
	{
		Sources.Emplace(FSourceEntry{SourceId, Value});
	}

	void PopValue(FMultisourceIdType SourceId)
	{
		Sources.RemoveAllSwap([SourceId](const FSourceEntry& Source) { return Source.SourceId == SourceId; });
	}

	ValueType GetValue() const
	{
		return Algo::TransformAccumulate(Sources, &FSourceEntry::Value, ValueType{0});
	}

	const TArray<FSourceEntry>& GetSources() const
	{
		return Sources;
	}

private:
	TArray<FSourceEntry> Sources;
};

}  // namespace MultisourceValue

// -- Aliases

template <class ValueType>
using TPriorityBasedMultisourceValue = TMultisourceValue<ValueType, MultisourceValue::TPriorityBasedResolver>;

using FIfAnyMultisourceValue = TMultisourceValue<bool, MultisourceValue::TIfAnyResolver>;
using FMinMultisourceValue = TMultisourceValue<float, MultisourceValue::TMinimumResolver>;
using FLastMultisourceValue = TMultisourceValue<float, MultisourceValue::TLastResolver>;

// -- Template definitions

template <class StringType>
StringType FMultisourceIdType::ToString() const
{
	return Visit([](auto V) { return IdToString<StringType>(V); }, Id);
}

template <class StringType>
StringType FMultisourceIdType::IdToString(const void* Ptr)
{
	if constexpr (std::is_same_v<StringType, FString>)
	{
		return FString::Printf(TEXT("0x%p"), Ptr);
	}
	else if constexpr (std::is_same_v<StringType, FUtf8String>)
	{
		return FUtf8String::Printf("0x%p", Ptr);
	}
	else
	{
		static_assert(
			std::is_same_v<StringType, FString> || std::is_same_v<StringType, FUtf8String>, "Unsupported string type");
		return {};
	}
}

template <class StringType>
StringType FMultisourceIdType::IdToString(const UObject* Object)
{
	if constexpr (std::is_same_v<StringType, FString>)
	{
		ZKZ_RETURN_IF_INVALID(Object, TEXT("INVALID OBJECT"));
		return Object->GetName();
	}
	else if constexpr (std::is_same_v<StringType, FUtf8String>)
	{
		ZKZ_RETURN_IF_INVALID(Object, TEXT("INVALID OBJECT"));
		return Object->GetFName().ToUtf8String();
	}
	else
	{
		static_assert(
			std::is_same_v<StringType, FString> || std::is_same_v<StringType, FUtf8String>, "Unsupported string type");
		return {};
	}
}

template <class StringType>
StringType FMultisourceIdType::IdToString(const FName Name)
{
	if constexpr (std::is_same_v<StringType, FString>)
	{
		return Name.ToString();
	}
	else if constexpr (std::is_same_v<StringType, FUtf8String>)
	{
		return Name.ToUtf8String();
	}
	else
	{
		static_assert(
			std::is_same_v<StringType, FString> || std::is_same_v<StringType, FUtf8String>, "Unsupported string type");
		return {};
	}
}

}  // namespace Zkz
