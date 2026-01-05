#include "Zakazane/Monostate.h"
#include "Zakazane/RAII.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::Test
{

namespace RAIITestPrivate
{

struct FMoveableOnly
{
	explicit FMoveableOnly(FMonostate)	// hides default constructor
	{
	}
	FMoveableOnly(FMoveableOnly&&) = default;
	FMoveableOnly& operator=(FMoveableOnly&&) = default;
};

}  // namespace RAIITestPrivate

ZKZ_BEGIN_AUTOMATION_TEST(
	FRAIITest,
	"Zakazane.ZakazaneUtilities.RAII",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(ScopedAssignmentRestoresOriginalValue)
{
	int I = 42;

	{
		TScopedAssignment Assignment{I, 666};

		TestEqual("Replaced value", I, 666);
	}

	TestEqual("Restored value", I, 42);
}

ZKZ_ADD_TEST(ScopedAssignmentMoveAssigns)
{
	// ReSharper disable once CppTooWideScope
	RAIITestPrivate::FMoveableOnly OuterValue{FMonostate{}};

	{
		TScopedAssignment Assignment{OuterValue, RAIITestPrivate::FMoveableOnly{FMonostate{}}};
	}

	TestTrue("Test successful", true);
}

ZKZ_ADD_TEST(ScopedAssignmentIsMoveable)
{
	int I = 42;

	{
		const auto Assignment = [this, &I]()
		{
			TScopedAssignment InnerAssignment{I, 666};

			TestEqual("Replaced value", I, 666);

			return MoveTemp(InnerAssignment);
		}();

		TestEqual("Replaced value survives after move", I, 666);
	}

	TestEqual("Restored value", I, 42);
}

ZKZ_ADD_TEST(ScopedAssignmentMoveAssigns)
{
	// ReSharper disable once CppTooWideScope
	RAIITestPrivate::FMoveableOnly OuterValue{FMonostate{}};

	{
		const auto Assignment = [this, &OuterValue]()
		{
			TScopedAssignment InnerAssignment{OuterValue, RAIITestPrivate::FMoveableOnly{FMonostate{}}};
			return MoveTemp(InnerAssignment);
		}();
	}

	TestTrue("Test successful", true);
}

ZKZ_END_AUTOMATION_TEST(FRAIITest);

}  // namespace Zkz::Test
