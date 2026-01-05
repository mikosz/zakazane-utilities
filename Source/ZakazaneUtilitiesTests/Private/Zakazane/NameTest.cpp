#include "Zakazane/Name.h"
#include "Zakazane/Test/Test.h"

#include <array>

namespace Zkz::Math::Test
{

ZKZ_BEGIN_AUTOMATION_TEST(
	FNameTest,
	"Zakazane.ZakazaneUtilities.Name",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(AlphabeticalLessOrdersNumbers)
{
	const FName Aardvark{"Aardvark"};
	const FName Betelgeuse{"Betelgeuse"};
	const FName Betelgeuse_2{"Betelgeuse_2"};
	const FName Betelgeuse_Lower{"betelgeuse"};

	// Verify that the function is actually needed
	ensure(Betelgeuse.GetComparisonIndex() == Betelgeuse_2.GetComparisonIndex());
	ensure(Betelgeuse.GetNumber() != Betelgeuse_2.GetNumber());
	ensure(Betelgeuse_Lower.GetComparisonIndex() == Betelgeuse.GetComparisonIndex());

	TestTrue("Aardvark < Betelgeuse", AlphabeticalLess(Aardvark, Betelgeuse));
	TestTrue("Betelgeuse < Betelgeuse2", AlphabeticalLess(Betelgeuse, Betelgeuse_2));
	TestFalse("Betelgeuse < betelgeuse", AlphabeticalLess(Betelgeuse, Betelgeuse_Lower));
	TestFalse("betelgeuse < Betelgeuse", AlphabeticalLess(Betelgeuse_Lower, Betelgeuse));
}

ZKZ_END_AUTOMATION_TEST(FNameTest);

}  // namespace Zkz::Math::Test
