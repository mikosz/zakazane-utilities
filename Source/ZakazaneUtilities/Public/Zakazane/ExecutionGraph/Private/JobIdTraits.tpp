// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "String/Join.h"
#include "Zakazane/ArrayView.h"
#include "Zakazane/ExecutionGraph/JobIdTraits.h"
#include "Zakazane/Name.h"
#include "Zakazane/String.h"

namespace Zkz::ExecutionGraph
{

template <class InAllocatorType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TDefaultSchedulerJobIdTraits<InAllocatorType>::FromString(const FStringView IdString, const TCHAR Separator)
	-> JobIdType
{
	auto IdArray = String::ParseInto<JobIdType>(IdString, Separator, String::ESkipEmptySegments::Yes);
	check(!IdArray.IsEmpty());

	return IdArray;
}

template <class InAllocatorType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TDefaultSchedulerJobIdTraits<InAllocatorType>::FromReference(JobIdReferenceType JobIdReference) -> JobIdType
{
	return JobIdType{JobIdReference};
}

template <class InAllocatorType>
void TDefaultSchedulerJobIdTraits<InAllocatorType>::MakeIncrementable(JobIdType& JobId)
{
	// We want repeating names that are made unique using Increment displayed in the same way. FName::ToString
	// does not show a _# prefix for the default number (as this would print -1, due to internal -> external conversion).
	// For this reason we increment it to 1. Thanks to this, if we use e.g. Initialization.SpawnActor and make this
	// unique, we'll get SpawnActor_0, SpawnActor_1, ... instead of SpawnActor, SpawnActor_0, SpawnActor_1, ...
	if (JobId.Last().GetNumber() == NAME_NO_NUMBER_INTERNAL)
	{
		Increment(JobId);
	}
}

template <class InAllocatorType>
void TDefaultSchedulerJobIdTraits<InAllocatorType>::Increment(JobIdType& JobId)
{
	check(!JobId.IsEmpty());

	FName& Last = JobId.Last();
	Last.SetNumber(Last.GetNumber() + 1);
}

template <class InAllocatorType>
bool TDefaultSchedulerJobIdTraits<InAllocatorType>::Less(JobIdReferenceType Lhs, JobIdReferenceType Rhs)
{
	using std::begin;
	using std::end;

	return std::lexicographical_compare(
		begin(Lhs),
		end(Lhs),
		begin(Rhs),
		end(Rhs),
		[](const FName Lhs, const FName Rhs) { return AlphabeticalLess(Lhs, Rhs); });
}

template <class InAllocatorType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TDefaultSchedulerJobIdTraits<InAllocatorType>::Append(
	JobIdReferenceType ParentJobId, JobIdReferenceType ChildJobId) -> JobIdType
{
	JobIdType JobId{ParentJobId};
	JobId.Append(ChildJobId);

	return JobId;
}

template <class InAllocatorType>
int32 TDefaultSchedulerJobIdTraits<InAllocatorType>::GetTypeHash(const JobIdReferenceType JobId)
{
	return GetArrayViewTypeHash(JobId);
}

template <class InAllocatorType>
FString TDefaultSchedulerJobIdTraits<InAllocatorType>::ToString(JobIdReferenceType JobId)
{
	constexpr int32 BufferSize = 128;

	return *WriteToString<BufferSize>(UE::String::Join(JobId, TEXT(".")));
}

template <class InAllocatorType>
// ReSharper disable once CppEnforceFunctionDeclarationStyle
auto TDefaultSchedulerJobIdTraits<InAllocatorType>::GetSubId(
	const JobIdReferenceType JobId, const int32 StartIdx, const int32 NumSegments) -> JobIdReferenceType
{
	return JobId.RightChop(StartIdx).Left(NumSegments);
}

template <class InAllocatorType>
int32 TDefaultSchedulerJobIdTraits<InAllocatorType>::GetNumSegments(const JobIdReferenceType JobId)
{
	return JobId.Num();
}

template <class InAllocatorType>
bool TDefaultSchedulerJobIdTraits<InAllocatorType>::Equals(const JobIdReferenceType Lhs, const JobIdReferenceType Rhs)
{
	return Equal(Lhs, Rhs);
}

namespace JobIdUtilities
{

template <CJobIdTraits JobIdTraitsType>
bool IsEmpty(typename JobIdTraitsType::JobIdReferenceType JobId)
{
	return JobIdTraitsType::GetNumSegments(JobId) == 0;
}

template <CJobIdTraits JobIdTraitsType>
JobIdTraitsType::JobIdReferenceType GetParent(const typename JobIdTraitsType::JobIdReferenceType JobId)
{
	const int32 NumSegments = JobIdTraitsType::GetNumSegments(JobId);
	check(NumSegments != 0);

	return JobIdTraitsType::GetSubId(JobId, 0, NumSegments - 1);
}

template <CJobIdTraits JobIdTraitsType>
bool IsChild(
	const typename JobIdTraitsType::JobIdReferenceType ParentJobId,
	const typename JobIdTraitsType::JobIdReferenceType ChildJobId)
{
	return JobIdTraitsType::Equals(ParentJobId, GetParent<JobIdTraitsType>(ChildJobId));
}

template <CJobIdTraits JobIdTraitsType>
JobIdTraitsType::JobIdReferenceType GetLeaf(const typename JobIdTraitsType::JobIdReferenceType JobId)
{
	const int32 NumSegments = JobIdTraitsType::GetNumSegments(JobId);
	ZKZ_RETURN_IF(NumSegments == 0, {});

	return JobIdTraitsType::GetSubId(JobId, NumSegments - 1, 1);
}

template <CJobIdTraits JobIdTraitsType>
FString PredecessorsToString(const TConstArrayView<typename JobIdTraitsType::JobIdReferenceType> Predecessors)
{
	constexpr int32 BufferSize = 256;

	using JobIdReferenceType = JobIdTraitsType::JobIdReferenceType;

	return *WriteToString<BufferSize>(UE::String::JoinBy(
		Predecessors,
		[](JobIdReferenceType JobIdRef) { return JobIdTraitsType::ToString(MoveTemp(JobIdRef)); },
		TEXT(", ")));
}

}  // namespace JobIdUtilities

}  // namespace Zkz::ExecutionGraph
