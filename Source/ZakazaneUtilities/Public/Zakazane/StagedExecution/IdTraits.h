// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Zakazane/Enum.h"

namespace Zkz::StagedExecution
{

/// Specialize for your id type if necessary
template <class InIdType>
class TIdTraits
{
public:
	using IdType = InIdType;

	static FString GetLogString(const IdType& Id)
	{
		if constexpr (TIsUEnumClass<InIdType>::Value)
		{
			return Enum::GetDisplayNameAsString(Id);
		}
		else
		{
			return LexToString(Id);
		}
	}
};

}  // namespace Zkz::StagedExecution
