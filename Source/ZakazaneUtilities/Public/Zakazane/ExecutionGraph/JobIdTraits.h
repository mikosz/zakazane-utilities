// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Containers/ArrayView.h"

namespace Zkz::ExecutionGraph
{

template <class T>
concept CJobIdTraits = requires {
	typename T::JobIdType;
	typename T::JobIdReferenceType;

	{ T::FromString(FStringView{}, TCHAR{}) } -> std::convertible_to<typename T::JobIdType>;
	{ T::FromReference(std::declval<typename T::JobIdReferenceType>()) } -> std::convertible_to<typename T::JobIdType>;
	{ T::MakeIncrementable(std::declval<typename T::JobIdType&>()) };
	{ T::Increment(std::declval<typename T::JobIdType&>()) };

	{ T::ToString(std::declval<typename T::JobIdReferenceType&>()) } -> std::convertible_to<FString>;
	{
		T::GetSubId(std::declval<typename T::JobIdReferenceType>(), int32{} /* start idx */, int32{} /* num segments */)
	} -> std::convertible_to<typename T::JobIdReferenceType>;

	{
		T::Append(
			std::declval<typename T::JobIdReferenceType>() /* parent */,
			std::declval<typename T::JobIdReferenceType>() /* child */)
	} -> std::convertible_to<typename T::JobIdType>;

	{ T::GetNumSegments(std::declval<typename T::JobIdReferenceType>()) } -> std::convertible_to<int32>;

	{
		T::Equals(std::declval<typename T::JobIdReferenceType>(), std::declval<typename T::JobIdReferenceType>())
	} -> std::convertible_to<bool>;
	{
		T::Less(std::declval<typename T::JobIdReferenceType>(), std::declval<typename T::JobIdReferenceType>())
	} -> std::convertible_to<bool>;

	{ T::GetTypeHash(std::declval<typename T::JobIdReferenceType&>()) } -> std::convertible_to<int32>;
};

inline constexpr uint32 DefaultSchedulerJobIdMaxSegments = 3;

template <class InAllocatorType = TFixedAllocator<DefaultSchedulerJobIdMaxSegments>>
class TDefaultSchedulerJobIdTraits
{
public:
	using JobIdType = TArray<FName, InAllocatorType>;
	using JobIdReferenceType = TConstArrayView<FName>;

	static JobIdType FromString(const FStringView IdString, TCHAR Separator = TCHAR{'.'});
	static JobIdType FromReference(JobIdReferenceType JobIdReference);

	static void MakeIncrementable(JobIdType& JobId);
	static void Increment(JobIdType& JobId);

	static FString ToString(JobIdReferenceType JobId);
	static JobIdReferenceType GetSubId(JobIdReferenceType JobId, int32 StartIdx, int32 NumSegments);

	static JobIdType Append(JobIdReferenceType ParentJobId, JobIdReferenceType ChildJobId);

	static int32 GetNumSegments(JobIdReferenceType JobId);

	static bool Equals(JobIdReferenceType Lhs, JobIdReferenceType Rhs);
	static bool Less(JobIdReferenceType Lhs, JobIdReferenceType Rhs);

	static int32 GetTypeHash(JobIdReferenceType JobId);
};

namespace JobIdUtilities
{

template <CJobIdTraits JobIdTraitsType>
bool IsEmpty(typename JobIdTraitsType::JobIdReferenceType JobId);

template <CJobIdTraits JobIdTraitsType>
JobIdTraitsType::JobIdReferenceType GetParent(typename JobIdTraitsType::JobIdReferenceType JobId);

template <CJobIdTraits JobIdTraitsType>
bool IsChild(
	typename JobIdTraitsType::JobIdReferenceType ParentJobId, typename JobIdTraitsType::JobIdReferenceType ChildJobId);

template <CJobIdTraits JobIdTraitsType>
JobIdTraitsType::JobIdReferenceType GetLeaf(const typename JobIdTraitsType::JobIdReferenceType JobId);

template <CJobIdTraits JobIdTraitsType>
FString PredecessorsToString(TConstArrayView<typename JobIdTraitsType::JobIdReferenceType> Predecessors);

}  // namespace JobIdUtilities

}  // namespace Zkz::ExecutionGraph

#include "Private/JobIdTraits.tpp"
