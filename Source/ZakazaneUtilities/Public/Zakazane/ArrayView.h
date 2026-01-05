// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Templates/IsPointer.h"

#include <type_traits>

namespace Zkz
{

/// Creates a const pointer array view, making this work:
/// @code
/// TArray<int*> ArrayOfIntPtr;
/// TArrayView<const int*> View = MakeConstPtrArrayView(ArrayOfIntPtr);
/// @endcode
///
/// @c MakeConstArrayView already exists, but doing this on a TArray<Type*> creates
/// a view of Type* const rather than const Type*.
template <
	class OtherRangeType,
	class CVUnqualifiedOtherRangeType = std::remove_cv_t<std::remove_reference_t<OtherRangeType>>,
	class OtherRangeElementType = typename CVUnqualifiedOtherRangeType::ElementType UE_REQUIRES(
		TIsContiguousContainer<CVUnqualifiedOtherRangeType>::Value
		&& !TIsTArrayView_V<CVUnqualifiedOtherRangeType> && TIsPointer<OtherRangeElementType>::Value)>
auto MakeConstPtrArrayView(OtherRangeType&& Container UE_LIFETIMEBOUND)
{
	// OtherRangeElementType is SomeType*
	// OtherRangeType is TArray<OtherRangeElementType>, so TArray<SomeType*>

	using PointerValueType = typename TRemovePointer<OtherRangeElementType>::Type;

	// PointerValueType is therefore SomeType
	// Container.GetData returns OtherRangeElementType* so SomeType**

	// T** doesn't convert to const T**, so need to const_cast

	const PointerValueType** ConstPtr = const_cast<const PointerValueType**>(Container.GetData());

	return MakeArrayView(ConstPtr, Container.Num());
}

}  // namespace Zkz
