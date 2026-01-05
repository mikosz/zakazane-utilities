#include "Zakazane/Test/ConstructionReportingType.h"

#include "Misc/AutomationTest.h"

namespace Zkz::Test
{

bool FConstructionReport::TestUnused(FAutomationTestBase& Test, FStringView MessagePrefix) const
{
	bool bAllSuccessful = true;

	bAllSuccessful =
		Test.TestEqual(MessagePrefix + FString("Unused object has no constructor calls"), ConstructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(MessagePrefix + FString("Unused object has no destructor calls"), DestructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(MessagePrefix + FString("Unused object has no move constructor calls"), MoveConstructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(MessagePrefix + FString("Unused object has no copy constructor calls"), CopyConstructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(MessagePrefix + FString("Unused object has no move assignment calls"), MoveAssignmentCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(MessagePrefix + FString("Unused object has no copy assignment calls"), CopyAssignmentCalls, 0)
		&& bAllSuccessful;

	return bAllSuccessful;
}

bool FConstructionReport::TestConstructedInPlace(
	FAutomationTestBase& Test, const FStringView MessagePrefix) const
{
	bool bAllSuccessful = true;

	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("In-place constructed type has a single constructor call"), ConstructorCalls, 1)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("In-place constructed type has no move constructor calls"), MoveConstructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("In-place constructed type has no copy constructor calls"), CopyConstructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("In-place constructed type has no move assignment calls"), MoveAssignmentCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("In-place constructed type has no copy assignment calls"), CopyAssignmentCalls, 0)
		&& bAllSuccessful;

	return bAllSuccessful;
}

bool FConstructionReport::TestMoveConstructed(FAutomationTestBase& Test, FStringView MessagePrefix) const
{
	bool bAllSuccessful = true;

	bAllSuccessful =
		Test.TestEqual(MessagePrefix + FString("Move constructed type has no constructor calls"), ConstructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful = Test.TestEqual(
						 MessagePrefix + FString("Move constructed type has a single move constructor call"),
						 MoveConstructorCalls,
						 1)
					 && bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Move constructed type has no copy constructor calls"), CopyConstructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Move constructed type has no move assignment calls"), MoveAssignmentCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Move constructed type has no copy assignment calls"), CopyAssignmentCalls, 0)
		&& bAllSuccessful;

	return bAllSuccessful;
}

bool FConstructionReport::TestCopyConstructed(FAutomationTestBase& Test, FStringView MessagePrefix) const
{
	bool bAllSuccessful = true;

	bAllSuccessful =
		Test.TestEqual(MessagePrefix + FString("Copy constructed type has no constructor calls"), ConstructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Copy constructed type has no move constructor calls"), MoveConstructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful = Test.TestEqual(
						 MessagePrefix + FString("Copy constructed type has a single copy constructor call"),
						 CopyConstructorCalls,
						 1)
					 && bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Copy constructed type has no move assignment calls"), MoveAssignmentCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Copy constructed type has no copy assignment calls"), CopyAssignmentCalls, 0)
		&& bAllSuccessful;

	return bAllSuccessful;
}

bool FConstructionReport::TestMoveAssigned(FAutomationTestBase& Test, FStringView MessagePrefix) const
{
	bool bAllSuccessful = true;

	bAllSuccessful =
		Test.TestEqual(MessagePrefix + FString("Move assigned type has a single constructor call"), ConstructorCalls, 1)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Move assigned type has no move constructor calls"), MoveConstructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Move assigned type has no copy constructor calls"), CopyConstructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Move assigned type has a single move assignment call"), MoveAssignmentCalls, 1)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Move assigned type has no copy assignment calls"), CopyAssignmentCalls, 0)
		&& bAllSuccessful;

	return bAllSuccessful;
}

bool FConstructionReport::TestCopyAssigned(FAutomationTestBase& Test, FStringView MessagePrefix) const
{
	bool bAllSuccessful = true;

	bAllSuccessful =
		Test.TestEqual(MessagePrefix + FString("Copy Assigned type has a single constructor call"), ConstructorCalls, 1)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Copy assigned type has no move constructor calls"), MoveConstructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Copy assigned type has no copy constructor calls"), CopyConstructorCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Copy assigned type has no move assignment calls"), MoveAssignmentCalls, 0)
		&& bAllSuccessful;
	bAllSuccessful =
		Test.TestEqual(
			MessagePrefix + FString("Copy assigned type has a single copy assignment call"), CopyAssignmentCalls, 1)
		&& bAllSuccessful;

	return bAllSuccessful;
}

bool FConstructionReport::TestMovedFrom(FAutomationTestBase& Test, FStringView MessagePrefix) const
{
	bool bAllSuccessful = true;

	bAllSuccessful = Test.TestEqual(MessagePrefix + FString{"Object moved from"}, MovedFrom, 1) && bAllSuccessful;
	bAllSuccessful = Test.TestEqual(MessagePrefix + FString{"Object not copied from"}, CopiedFrom, 0) && bAllSuccessful;

	return bAllSuccessful;
}

bool FConstructionReport::TestCopiedFrom(FAutomationTestBase& Test, FStringView MessagePrefix) const
{
	bool bAllSuccessful = true;

	bAllSuccessful = Test.TestEqual(MessagePrefix + FString{"Object not moved from"}, MovedFrom, 0) && bAllSuccessful;
	bAllSuccessful = Test.TestEqual(MessagePrefix + FString{"Object copied from"}, CopiedFrom, 1) && bAllSuccessful;

	return bAllSuccessful;
}

FConstructionReportingType::FConstructionReportingType(FConstructionReport& InConstructionReport)
	: ConstructionReport(InConstructionReport), ChildConstructionReport{InConstructionReport}
{
	++ConstructionReport.ConstructorCalls;
}

FConstructionReportingType::FConstructionReportingType(
	FConstructionReport& InConstructionReport, FConstructionReport& InChildConstructionReport)
	: ConstructionReport(InConstructionReport), ChildConstructionReport{InChildConstructionReport}
{
	++ConstructionReport.ConstructorCalls;
}

FConstructionReportingType::~FConstructionReportingType()
{
	++ConstructionReport.DestructorCalls;
}

FConstructionReportingType::FConstructionReportingType(const FConstructionReportingType& Other)
	: Data{&Other}, ConstructionReport{Other.ChildConstructionReport}, ChildConstructionReport{ConstructionReport}
{
	++ConstructionReport.CopyConstructorCalls;
	++Other.ConstructionReport.CopiedFrom;
}

FConstructionReportingType::FConstructionReportingType(FConstructionReportingType&& Other)
	: Data{Other.Data}, ConstructionReport{Other.ChildConstructionReport}, ChildConstructionReport{ConstructionReport}
{
	++ConstructionReport.MoveConstructorCalls;
	Other.Data = nullptr;
	++Other.ConstructionReport.MovedFrom;
}

FConstructionReportingType& FConstructionReportingType::operator=(const FConstructionReportingType& Other)
{
	Data = Other.Data;
	++ConstructionReport.CopyAssignmentCalls;
	++Other.ConstructionReport.CopiedFrom;
	return *this;
}

FConstructionReportingType& FConstructionReportingType::operator=(FConstructionReportingType&& Other)
{
	Data = Other.Data;
	Other.Data = nullptr;
	++ConstructionReport.MoveAssignmentCalls;
	++Other.ConstructionReport.MovedFrom;
	return *this;
}

bool FConstructionReportingType::TestMovedFrom(
	FAutomationTestBase& Test, const FConstructionReportingType& To, const FStringView MessagePrefix) const
{
	bool bAllSuccessful = true;

	bAllSuccessful = Test.TestTrue(MessagePrefix + FString{"Move-constructed value has original data"}, To.Data == this)
					 && bAllSuccessful;
	bAllSuccessful =
		Test.TestTrue(MessagePrefix + FString{"Move-constructed-from value has cleared data"}, Data == nullptr)
		&& bAllSuccessful;
	bAllSuccessful = ConstructionReport.TestMovedFrom(Test, MessagePrefix) && bAllSuccessful;

	return bAllSuccessful;
}

bool FConstructionReportingType::TestCopiedFrom(
	FAutomationTestBase& Test, const FConstructionReportingType& To, const FStringView MessagePrefix) const
{
	bool bAllSuccessful = true;

	bAllSuccessful = Test.TestTrue(MessagePrefix + FString{"Copy-constructed value has original data"}, To.Data == this)
					 && bAllSuccessful;
	bAllSuccessful =
		Test.TestTrue(MessagePrefix + FString{"Copy-constructed-from value has original data"}, Data == this)
		&& bAllSuccessful;
	bAllSuccessful = ConstructionReport.TestCopiedFrom(Test, MessagePrefix) && bAllSuccessful;

	return bAllSuccessful;
}

}  // namespace Zkz::Test
