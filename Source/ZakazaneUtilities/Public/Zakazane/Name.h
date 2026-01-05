// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Zkz
{

/// Compares FNames in alphabetical order. Use only if necessary (e.g. for sorting in UI presentation), otherwise
/// prefer GetComparisonIndex or GetDisplayIndex.
/// Note that this is required if you want to collect a unique array of FNames, as GetComparisonIndex and
/// GetDisplayIndex both ignore Number.
/// NOTE: Comparison is case-insensitive
ZAKAZANEUTILITIES_API bool AlphabeticalLess(FName Lhs, FName Rhs);

}  // namespace Zkz
