// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FAutomationTestBase;

namespace Zkz::Test
{

/// Stores information about construction / destruction / assignment events for a construction reporting type
struct ZAKAZANETESTUTILITIES_API FConstructionReport
{
	int32 ConstructorCalls = 0;
	int32 DestructorCalls = 0;
	int32 MoveConstructorCalls = 0;
	int32 CopyConstructorCalls = 0;
	int32 MoveAssignmentCalls = 0;
	int32 CopyAssignmentCalls = 0;
	int32 CopiedFrom = 0;
	int32 MovedFrom = 0;

	bool TestUnused(FAutomationTestBase& Test, FStringView MessagePrefix = {}) const;
	bool TestConstructedInPlace(FAutomationTestBase& Test, FStringView MessagePrefix = {}) const;
	bool TestMoveConstructed(FAutomationTestBase& Test, FStringView MessagePrefix = {}) const;
	bool TestCopyConstructed(FAutomationTestBase& Test, FStringView MessagePrefix = {}) const;
	bool TestMoveAssigned(FAutomationTestBase& Test, FStringView MessagePrefix = {}) const;
	bool TestCopyAssigned(FAutomationTestBase& Test, FStringView MessagePrefix = {}) const;
	bool TestMovedFrom(FAutomationTestBase& Test, FStringView MessagePrefix = {}) const;
	bool TestCopiedFrom(FAutomationTestBase& Test, FStringView MessagePrefix = {}) const;
};

/// Type that stores all construction / destruction / assignment operations in a given report object. Useful for
/// testing of correct behavior of wrappers.
struct ZAKAZANETESTUTILITIES_API FConstructionReportingType
{
	const FConstructionReportingType* Data = this;
	FConstructionReport& ConstructionReport;
	FConstructionReport& ChildConstructionReport;

	explicit FConstructionReportingType(FConstructionReport& InConstructionReport);

	explicit FConstructionReportingType(
		FConstructionReport& InConstructionReport, FConstructionReport& InChildConstructionReport);

	~FConstructionReportingType();

	FConstructionReportingType(const FConstructionReportingType& Other);

	FConstructionReportingType(FConstructionReportingType&& Other);

	FConstructionReportingType& operator=(const FConstructionReportingType& Other);

	FConstructionReportingType& operator=(FConstructionReportingType&& Other);

	bool TestMovedFrom(
		FAutomationTestBase& Test, const FConstructionReportingType& To, const FStringView MessagePrefix = {}) const;

	bool TestCopiedFrom(
		FAutomationTestBase& Test, const FConstructionReportingType& To, const FStringView MessagePrefix = {}) const;
};

struct ZAKAZANETESTUTILITIES_API FConstructionReportingNoncopyable : FConstructionReportingType
{
	using FConstructionReportingType::FConstructionReportingType;

	~FConstructionReportingNoncopyable() = default;
	FConstructionReportingNoncopyable(const FConstructionReportingNoncopyable&) = delete;
	FConstructionReportingNoncopyable(FConstructionReportingNoncopyable&&) = default;
	FConstructionReportingNoncopyable& operator=(const FConstructionReportingNoncopyable&) = delete;
	FConstructionReportingNoncopyable& operator=(FConstructionReportingNoncopyable&&) = default;
};

struct ZAKAZANETESTUTILITIES_API FConstructionReportingNonmoveable : FConstructionReportingType
{
	using FConstructionReportingType::FConstructionReportingType;

	~FConstructionReportingNonmoveable() = default;
	FConstructionReportingNonmoveable(const FConstructionReportingNonmoveable&) = default;
	FConstructionReportingNonmoveable(FConstructionReportingNonmoveable&&) = delete;
	FConstructionReportingNonmoveable& operator=(const FConstructionReportingNonmoveable&) = default;
	FConstructionReportingNonmoveable& operator=(FConstructionReportingNonmoveable&&) = delete;
};

struct ZAKAZANETESTUTILITIES_API FConstructionReportingNoMoveOrCopy : FConstructionReportingType
{
	using FConstructionReportingType::FConstructionReportingType;

	~FConstructionReportingNoMoveOrCopy() = default;
	FConstructionReportingNoMoveOrCopy(const FConstructionReportingNoMoveOrCopy&) = delete;
	FConstructionReportingNoMoveOrCopy(FConstructionReportingNoMoveOrCopy&&) = delete;
	FConstructionReportingNoMoveOrCopy& operator=(const FConstructionReportingNoMoveOrCopy&) = delete;
	FConstructionReportingNoMoveOrCopy& operator=(FConstructionReportingNoMoveOrCopy&&) = delete;
};

}  // namespace Zkz::Test
