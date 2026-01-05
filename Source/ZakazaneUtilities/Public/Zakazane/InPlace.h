// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Misc/TVariant.h"

namespace Zkz
{

/// Port of std::in_place
struct FInPlace
{
};

constexpr FInPlace InPlace;

/// Imported TInPlaceType with InPlaceType constant
template <class T>
using TInPlaceType = ::TInPlaceType<T>;

template <class T>
constexpr TInPlaceType<T> InPlaceType;

/// Type trait true for any specialization of TInPlaceType
template <class T>
constexpr bool IsInPlaceType = false;

template <class T>
constexpr bool IsInPlaceType<TInPlaceType<T>> = true;

}  // namespace Zkz
