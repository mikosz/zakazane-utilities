#include "Zakazane/Monostate.h"
#include "Zakazane/RAII.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::Test
{

namespace RAIITestPrivate
{

struct FVoidCallable
{
	explicit FVoidCallable(int& InCalledTimes) : CalledTimes{InCalledTimes}
	{
	}

	void operator()() const
	{
		++CalledTimes;
	}

	int& CalledTimes;
};

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

ZKZ_ADD_TEST(ScopedExecutionCallsBoundFunctionWhenBound)
{
	using namespace RAIITestPrivate;

	// Simple call on scope exit
	{
		int CalledTimes = 0;

		{
			FScopedExecution ScopedExecution{FVoidCallable{CalledTimes}};
		}

		TestEqual("Function called at scope exit", CalledTimes, 1);
	}

	// Call can be delayed by moving the scoped execution...
	{
		int CalledTimes = 0;
		FScopedExecution Outer;

		{
			FScopedExecution Inner{FVoidCallable{CalledTimes}};
			Outer = MoveTemp(Inner);
		}

		TestEqual("Moved function not called at scope exit", CalledTimes, 0);

		// ... and triggered manually

		Outer.Trigger();
		TestEqual("Moved function called after trigger", CalledTimes, 1);
	}
}

ZKZ_ADD_TEST(ScopedExecutionDoesntCallIfReleasedOrReset)
{
	using namespace RAIITestPrivate;

	{
		int CalledTimes = 0;

		{
			FScopedExecution ScopedExecution{FVoidCallable{CalledTimes}};
			ScopedExecution.Reset();
		}

		TestEqual("Function not called at scope exit when reset", CalledTimes, 0);
	}

	{
		int CalledTimes = 0;

		{
			FScopedExecution ScopedExecution{FVoidCallable{CalledTimes}};

			const TUniqueFunction<void()> Func = ScopedExecution.Release();
			TestTrue("Release returns bound function", Func.IsSet());
		}

		TestEqual("Function not called at scope exit when released", CalledTimes, 0);
	}
}

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
