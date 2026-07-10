// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Zkz
{
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

template <class T>
constexpr bool IsSoftObjectPtr = false;

template <class T>
constexpr bool IsSoftObjectPtr<TSoftObjectPtr<T>> = true;

template <class T>
concept CTypeOfSoftObjectPtr = IsSoftObjectPtr<T>;

template <class T>
constexpr bool IsSoftClassPtr = false;

template <class T>
constexpr bool IsSoftClassPtr<TSoftClassPtr<T>> = true;

template <class T>
concept CTypeOfSoftClassPtr = IsSoftClassPtr<T>;

/// Soft pointers (TSoftObjectPtr, TSoftClassPtr) need special handling in the common pointer interface,
/// because their IsValid semantics differ from other pointers: IsValid checks whether the *loaded* object is
/// valid, and so returns false both for an unset (null) soft pointer and for a set-but-not-yet-loaded one.
/// See Zkz::Pointer::IsValid, IsLoadedObjectValid and IsNull below.
template <class T>
concept CTypeOfSoftPtr = CTypeOfSoftObjectPtr<T> || CTypeOfSoftClassPtr<T>;

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

namespace Pointer
{
/// Implements a common IsValid function for all types of pointers, to allow a common interface for checking
/// pointer validity.
/// * for smart pointers calls the member IsValid function
/// * for raw pointers to UObject calls the global IsValid function
/// * for raw pointers to non-UObject checks nullptr equality
///
/// Soft pointers (TSoftObjectPtr, TSoftClassPtr) are deliberately excluded and instead static_assert, because
/// IsValid is ambiguous for them: it reports whether the *loaded* object is valid, conflating an unset soft
/// pointer with a set-but-unloaded one. Use IsLoadedObjectValid or IsNull instead.
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
	requires(!CTypeOfSoftPtr<T>)
bool IsValid(const T& Ptr)
{
	return Ptr.IsValid();
}

template <class T>
concept CGlobalIsValidFunctionDefinedForType = requires(const T Ptr) {
	{ ::IsValid(Ptr) } -> std::convertible_to<bool>;
};

template <CGlobalIsValidFunctionDefinedForType T>
	requires(!CTypeOfSoftPtr<T>)
bool IsValid(const T& Ptr)
{
	return ::IsValid(Ptr);
}

template <class T>
bool IsValid(const TSharedRef<T>& SharedRef)
{
	return true;
}

/// Soft pointers intentionally do not participate in the common IsValid interface, because IsValid is
/// ambiguous for them (see Zkz::CTypeOfSoftPtr). Use IsLoadedObjectValid to check whether the loaded object is
/// valid, or IsNull to check whether the soft pointer is unset.
template <CTypeOfSoftPtr T>
bool IsValid(const T&)
{
	static_assert(
		sizeof(T) == 0,
		"Do not call Zkz::Pointer::IsValid on a soft pointer: it is ambiguous. "
		"Use Zkz::Pointer::IsLoadedObjectValid to check whether the loaded object is valid, "
		"or Zkz::Pointer::IsNull to check whether the soft pointer is unset.");
	return false;
}

/// Returns whether the object referenced by the soft pointer is currently loaded and valid. Equivalent to the
/// soft pointer's own IsValid member function; named explicitly to disambiguate from IsNull.
template <CTypeOfSoftPtr T>
bool IsLoadedObjectValid(const T& Ptr)
{
	return Ptr.IsValid();
}

/// Returns whether the soft pointer is unset (references no object). This is independent of load state: a
/// set-but-unloaded soft pointer is not null.
template <CTypeOfSoftPtr T>
bool IsNull(const T& Ptr)
{
	return Ptr.IsNull();
}

/// Functor calling IsValid. Useful for example, when using IsValid as an algorithm predicate.
/// <pre>
///   // won't work, because &Zkz::Pointer::IsValid is overloaded
///   Zkz::TransformIf(Container, &Zkz::Pointer::IsValid, []() {...});
///
///   // works just fine
///   Zkz::TransformIf(Container, Zkz::Pointer::FIsValid{}, []() {...});
/// </pre>
struct FIsValid
{
	template <class T>
	bool operator()(T&& Arg) const
	{
		return Pointer::IsValid(Forward<T>(Arg));
	}
};

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

}  // namespace Zkz
