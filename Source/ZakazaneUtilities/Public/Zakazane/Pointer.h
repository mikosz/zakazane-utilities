// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Zkz
{
namespace Pointer
{
/// Implements a common IsValid function for all types of pointers, to allow a common interface for checking
/// pointer validity.
/// * for smart pointers calls the member IsValid function
/// * for raw pointers to UObject calls the global IsValid function
/// * for raw pointers to non-UObject checks nullptr equality
template <class T>
	requires(!std::is_base_of_v<UObject, T>)
bool IsValid(T* const Ptr)
{
	return Ptr != nullptr;
}

template <class T>
	requires(std::is_base_of_v<UObject, T>)
bool IsValid(T* const Ptr)
{
	return ::IsValid(Ptr);
}

template <class T>
concept CHasIsValidMemberFunction = requires(const T Ptr) {
	{ Ptr.IsValid() } -> std::convertible_to<bool>;
};

template <CHasIsValidMemberFunction T>
bool IsValid(const T& Ptr)
{
	return Ptr.IsValid();
}

template <class T>
concept CGlobalIsValidFunctionDefinedForType = requires(const T Ptr) {
	{ ::IsValid(Ptr) } -> std::convertible_to<bool>;
};

template <CGlobalIsValidFunctionDefinedForType T>
bool IsValid(const T& Ptr)
{
	return ::IsValid(Ptr);
}

template <class T>
bool IsValid(const TSharedRef<T>& SharedRef)
{
	return true;
}

/// Implements a common Get function for all types of pointers, to allow a common interface for retrieving the
/// underlying pointer.
template <class T>
T* Get(T* const Ptr)
{
	return Ptr;
}

template <class T>
concept CHasGetMemberFunction = requires(const T Ptr) { requires std::is_pointer_v<decltype(Ptr.Get())>; };

template <CHasGetMemberFunction T>
auto* Get(const T& Ptr)
{
	return Ptr.Get();
}

template <class T>
T* Get(const TSharedRef<T>& SharedRef)
{
	return &SharedRef.Get();
}

}  // namespace Pointer

/// Identity Traits

// types

template <class T>
constexpr bool IsWeakPtr = false;

template <class T, ESPMode Mode>
constexpr bool IsWeakPtr<TWeakPtr<T, Mode>> = true;

template <class T>
concept CTypeOfWeakPtr = IsWeakPtr<T>;

template <class T>
constexpr bool IsSharedPtr = false;

template <class T, ESPMode Mode>
constexpr bool IsSharedPtr<TSharedPtr<T, Mode>> = true;

template <class T>
concept CTypeOfSharedPtr = IsSharedPtr<T>;

template <class T>
constexpr bool IsSharedRef = false;

template <class T, ESPMode Mode>
constexpr bool IsSharedRef<TSharedRef<T, Mode>> = true;

template <class T>
concept CTypeOfSharedRef = IsSharedRef<T>;

template <class T>
constexpr bool IsUniquePtr = false;

template <class T, class Deleter>
constexpr bool IsUniquePtr<TUniquePtr<T, Deleter>> = true;

template <class T>
concept CTypeOfUniquePtr = IsUniquePtr<T>;

template <class T>
constexpr bool IsWeakObjectPtr = false;

template <class T, class TWeakObjectPtrBase>
constexpr bool IsWeakObjectPtr<TWeakObjectPtr<T, TWeakObjectPtrBase>> = true;

template <class T>
concept CTypeOfWeakObjectPtr = IsWeakObjectPtr<T>;

template <class T>
constexpr bool IsStrongObjectPtr = false;

template <class T, class ReferencerNameProvider>
constexpr bool IsStrongObjectPtr<TStrongObjectPtr<T, ReferencerNameProvider>> = true;

template <class T>
concept CTypeOfStrongObjectPtr = IsStrongObjectPtr<T>;

// categories

template <class T>
concept CTypeOfOwningPtr =
	CTypeOfUniquePtr<T> || CTypeOfSharedPtr<T> || CTypeOfSharedRef<T> || CTypeOfStrongObjectPtr<T>;

template <class T>
concept CTypeOfWeakOwningPtr = CTypeOfWeakPtr<T> || CTypeOfWeakObjectPtr<T>;

/// TLifetimeTrackingPtrTraits is a traits type for all pointers that track the lifetime of the pointed object.
/// Both owning and non-owning pointers are fine for this, but raw pointers are not.

template <class T>
struct TLifetimeTrackingPtrTraits
{
	constexpr static bool IsLifetimeTrackingPtr = false;
};

template <CTypeOfWeakOwningPtr TypeOfWeakPtr>
struct TLifetimeTrackingPtrTraits<TypeOfWeakPtr>
{
	constexpr static bool IsLifetimeTrackingPtr = true;
};

template <CTypeOfOwningPtr TypeOfOwningPtr>
struct TLifetimeTrackingPtrTraits<TypeOfOwningPtr>
{
	constexpr static bool IsLifetimeTrackingPtr = true;
};

template <class T>
concept CLifetimeTrackingPtr = TLifetimeTrackingPtrTraits<std::remove_cv_t<T>>::IsLifetimeTrackingPtr;

}  // namespace Zkz
