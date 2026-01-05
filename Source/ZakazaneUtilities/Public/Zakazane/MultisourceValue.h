// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Algo/Accumulate.h"

namespace Zkz
{

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

	using ResolverType::GetValue;
	using ResolverType::PopValue;
	using ResolverType::PushValue;

private:
	ValueType DefaultValue;

	friend class ResolverType;	// need to befriend resolver so that it may static_cast to TMultisourceValue
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
	using SourceType = const void*;

	void PushValue(SourceType Source)
	{
		Sources.Emplace(Source);
	}

	void PopValue(const SourceType Source)
	{
		Sources.Remove(Source);
	}

	bool GetValue() const
	{
		const bool bDefaultValue = static_cast<const MultisourceValueType&>(*this).GetDefaultValue();
		return Sources.IsEmpty() ? bDefaultValue : !bDefaultValue;
	}

private:
	TSet<SourceType> Sources;
};


/// Value sources are added with a value. The minimum value from the sources or the default value is returned. 
/// @tparam InValueType - automatically filled in by TMultisourceValue: this is the value type to be resolved
/// @tparam MultisourceValueType - child TMultisourceValue (CRTP)
template <class InValueType, class MultisourceValueType>
class TMinimumResolver
{
public:
	using ValueType = InValueType;
	using SourceType = const void*;
	
	void PushValue(SourceType Source, ValueType Value)
	{
		Sources.Emplace(Source, Value);
	}

	void PopValue(SourceType Source)
	{
		Sources.RemoveAllSwap([Source](const TPair<SourceType,ValueType>& Entry) { return Entry.Key == Source; });
	}

	ValueType GetValue() const
	{
		ValueType DefaultValue =  static_cast<const MultisourceValueType&>(*this).GetDefaultValue();
		
		const TPair<SourceType,ValueType>* SourceEntryPtr = Algo::MinElementBy(Sources,&TPair<SourceType,ValueType>::Value);
		if (SourceEntryPtr == nullptr)
		{
			return DefaultValue;
		}
		
		return FMath::Min(DefaultValue, SourceEntryPtr->Value);
	}

private:
	TArray<TPair<SourceType,ValueType>> Sources;
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

private:
	struct FSourceEntry
	{
		SourceIdType SourceId = -1;

		PriorityType Priority = -1;

		ValueType Value;
	};

	SourceIdType LastSourceId = 0;

	TArray<FSourceEntry> Sources;
};

// #TODO #MultisourceValue: resolvers need to have a type parameter for source id type or at
// least use a variant for int32 / fname / pointer
/// All value sources are summed up.
/// @tparam InValueType - automatically filled in by TMultisourceValue: this is the value type to be resolved
/// @tparam MultisourceValueType - child TMultisourceValue (CRTP)
template <class InValueType, class MultisourceValueType>
class TSumResolver
{
public:
	using ValueType = InValueType;
	using SourceIdType = TObjectPtr<const UObject>;

	void PushValue(SourceIdType SourceId, ValueType Value)
	{
		Sources.Emplace(FSourceEntry{SourceId, Value});
	}

	void PopValue(SourceIdType SourceId)
	{
		Sources.RemoveAllSwap([SourceId](const FSourceEntry& Source) { return Source.SourceId == SourceId; });
	}

	ValueType GetValue() const
	{
		return Algo::TransformAccumulate(Sources, &FSourceEntry::Value, ValueType{0});
	}

private:
	struct FSourceEntry
	{
		SourceIdType SourceId = nullptr;

		ValueType Value;
	};

	TArray<FSourceEntry> Sources;
};

}  // namespace MultisourceValue

// -- Aliases

template <class ValueType>
using TPriorityBasedMultisourceValue = TMultisourceValue<ValueType, MultisourceValue::TPriorityBasedResolver>;

using FIfAnyMultisourceValue = TMultisourceValue<bool, MultisourceValue::TIfAnyResolver>;
using FMinMultisourceValue = TMultisourceValue<float, MultisourceValue::TMinimumResolver>;

}  // namespace Zkz
