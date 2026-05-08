#include "Zakazane/Budgeting/Budget.h"
#include "Zakazane/Budgeting/EnqueueBudgetedTask.h"
#include "Zakazane/Test/Test.h"

#include <chrono>
#include <thread>

namespace Zkz::Budgeting::Test
{

namespace Private
{

struct FWait20ms
{
	int32* CalledTimesPtr = nullptr;

	void operator()() const
	{
		using namespace std::chrono_literals;

		std::this_thread::sleep_for(20ms);

		ZKZ_RETURN_IF_ENSUREALWAYS(CalledTimesPtr == nullptr);
		++*CalledTimesPtr;
	}
};

}  // namespace Private

DECLARE_LOG_CATEGORY_EXTERN(LogZkzTickableBudgetTest, Display, All);
DEFINE_LOG_CATEGORY(LogZkzTickableBudgetTest);

ZKZ_BEGIN_AUTOMATION_TEST(
	FBudgetTest,
	"Zakazane.ZakazaneUtilities.Budgeting.Budget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(ExecutesTaskIfTimeLeft)
{
	auto Budget = MakeBudget(FTimespan::FromMilliseconds(50), LogZkzTickableBudgetTest);

	int32 CalledTimes = 0;
	Private::FWait20ms Wait20ms{&CalledTimes};

	Budget.EnqueueTask(Wait20ms);
	Budget.EnqueueTask(Wait20ms);
	Budget.EnqueueTask(Wait20ms);
	Budget.EnqueueTask(Wait20ms);

	TestEqual("Before first tick completed 0 Wait20ms", CalledTimes, 0);

	Budget.Tick();

	TestEqual("After first tick completed 3 Wait20ms", CalledTimes, 3);

	Budget.Tick();

	TestEqual("After second tick completed 4 Wait20ms", CalledTimes, 4);

	Budget.EnqueueTask(Wait20ms);

	TestEqual("5TH Wait20ms completed immediately after enqueue", CalledTimes, 5);
}

ZKZ_ADD_TEST(ExecutionGraphIntegrationWorks)
{
	const auto BudgetPtr = MakeShared<TBudget<decltype(LogZkzTickableBudgetTest)>>(
		FTimespan::FromMilliseconds(30), LogZkzTickableBudgetTest);

	ExecutionGraph::TScheduler Scheduler{LogZkzTickableBudgetTest};

	int32 CalledTimes = 0;
	Private::FWait20ms Wait20ms{&CalledTimes};

	using JobIdReferenceType = ExecutionGraph::TScheduler<decltype(LogZkzTickableBudgetTest)>::JobIdReferenceType;

	const auto Result1 =
		EnqueueBudgetedTask(Scheduler, Scheduler.MakeJobIdFromString(TEXTVIEW("a")), {}, BudgetPtr, Wait20ms);
	ZKZ_RETURN_IF(!TestTrue("Result1 successful", !Result1.HasError()));

	const auto Result2 =
		EnqueueBudgetedTask(Scheduler, Scheduler.MakeJobIdFromString(TEXTVIEW("b")), {}, BudgetPtr, Wait20ms);
	ZKZ_RETURN_IF(!TestTrue("Result2 successful", !Result2.HasError()));

	const auto Result3 = EnqueueBudgetedTask(
		Scheduler,
		Scheduler.MakeJobIdFromString(TEXTVIEW("c")),
		{JobIdReferenceType{"a"}, JobIdReferenceType{"b"}},
		BudgetPtr,
		Wait20ms);
	ZKZ_RETURN_IF(!TestTrue("Result3 successful", !Result3.HasError()));

	TestEqual("Before first tick 0 jobs completed", CalledTimes, 0);

	BudgetPtr->Tick();

	TestEqual("After first tick 2 jobs completed", CalledTimes, 2);

	BudgetPtr->Tick();

	TestEqual("After second tick 3 jobs completed", CalledTimes, 3);
}

ZKZ_END_AUTOMATION_TEST(FBudgetTest);

}  // namespace Zkz::Budgeting::Test
