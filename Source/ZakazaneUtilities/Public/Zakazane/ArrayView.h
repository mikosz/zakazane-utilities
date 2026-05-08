// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "ReturnIfMacros.h"
#include "Templates/IsPointer.h"

#include <type_traits>

namespace Zkz
{

/// Creates a const pointer array view, making this work:
/// @code
/// TArray<int*> ArrayOfIntPtr;
/// TArrayView<const int*> View = MakePtrToConstArrayView(ArrayOfIntPtr);
/// @endcode
///
/// @c MakeConstArrayView already exists, but doing this on a TArray<Type*> creates
/// a view of Type* const rather than const Type*.
template <
	class OtherRangeType,
	class CVUnqualifiedOtherRangeType = std::remove_cv_t<std::remove_reference_t<OtherRangeType>>,
	class OtherRangeElementType = CVUnqualifiedOtherRangeType::ElementType UE_REQUIRES(
		TIsContiguousContainer<CVUnqualifiedOtherRangeType>::Value
		&& !TIsTArrayView_V<CVUnqualifiedOtherRangeType> && TIsPointer<OtherRangeElementType>::Value)>
auto MakePtrToConstArrayView(OtherRangeType&& Container UE_LIFETIMEBOUND)
{
	// OtherRangeElementType is SomeType*
	// OtherRangeType is TArray<OtherRangeElementType>, so TArray<SomeType*>

	using PointerValueType = TRemovePointer<OtherRangeElementType>::Type;

	// PointerValueType is therefore SomeType
	// Container.GetData returns OtherRangeElementType* so SomeType**

	// T** doesn't convert to const T**, so need to const_cast

	const PointerValueType** ConstPtr = const_cast<const PointerValueType**>(Container.GetData());

	return MakeArrayView(ConstPtr, Container.Num());
}

/// Creates a const pointer array view, making this work:
/// @code
/// int* IntPtr1;
/// int* IntPtr2;
/// TArrayView<const int*> View = MakePtrToConstArrayView({IntPtr1, IntPtr2});
/// @endcode
///
/// @c MakeConstArrayView already exists, but doing this on a std::initializer_list<Type*> creates
/// a view of Type* const rather than const Type*.
template <class ElementType UE_REQUIRES(TIsPointer<ElementType>::Value)>
auto MakePtrToConstArrayView(std::initializer_list<ElementType> Elements UE_LIFETIMEBOUND)
{
	// ElementType is SomeType*

	using PointerValueType = TRemovePointer<ElementType>::Type;

	// PointerValueType is SomeType
	// GetData(Elements) returns ElementType* so SomeType**

	// T** doesn't convert to const T**, so need to const_cast

	const PointerValueType** ConstPtr = const_cast<const PointerValueType**>(GetData(Elements));

	return MakeArrayView(ConstPtr, GetNum(Elements));
}

/// Copied from GetTypeHash(const TArray<InElementType, InAllocatorType>& A)
template <class ElementType, class SizeType>
auto GetArrayViewTypeHash(const TArrayView<ElementType, SizeType> ArrayView)
{
	uint32 Hash = 0;
	for (const ElementType& V : ArrayView)
	{
		Hash = HashCombineFast(Hash, GetTypeHash(V));
	}
	return Hash;
}

/// Equality function for array views.
template <class LhsArrayViewElementType, class RhsArrayViewElementType>
	requires std::equality_comparable_with<LhsArrayViewElementType, RhsArrayViewElementType>
bool Equal(const TConstArrayView<LhsArrayViewElementType> Lhs, const TConstArrayView<RhsArrayViewElementType> Rhs)
{
	ZKZ_RETURN_IF(Lhs.Num() != Rhs.Num(), false);

	for (int32 Index = 0; Index < Lhs.Num(); ++Index)
	{
		ZKZ_RETURN_IF(Lhs[Index] != Rhs[Index], false);
	}

	return true;
}

}  // namespace Zkz
