#include "Zakazane/ExecutionGraph/JobIdTraits.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::ExecutionGraph::Test
{

ZKZ_BEGIN_AUTOMATION_TEST(
	FDefaultSchedulerJobIdTest,
	"Zakazane.ZakazaneUtilities.ExecutionGraph.DefaultSchedulerJobIdTraits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(ParsesString)
{
	using FTestedTraits = TDefaultSchedulerJobIdTraits<TFixedAllocator<3>>;

	const auto JobId = FTestedTraits::FromString(TEXTVIEW("Grand.Parent.Job"));

	const auto Parent0 = JobIdUtilities::GetParent<FTestedTraits>(JobId);
	TestEqual("Parent[0]", FTestedTraits::ToString(Parent0), FString{"Grand.Parent"});

	const auto Parent1 = JobIdUtilities::GetParent<FTestedTraits>(Parent0);
	TestEqual("Parent[1]", FTestedTraits::ToString(Parent1), FString{"Grand"});

	const auto Parent2 = JobIdUtilities::GetParent<FTestedTraits>(Parent1);
	TestEqual("Parent[2]", FTestedTraits::ToString(Parent2), FString{""});
}

ZKZ_END_AUTOMATION_TEST(FDefaultSchedulerJobIdTest)

}  // namespace Zkz::ExecutionGraph::Test
