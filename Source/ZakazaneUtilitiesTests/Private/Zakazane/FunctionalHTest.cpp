#include "Zakazane/Functional.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::Test
{

namespace FunctionalHPrivate
{

struct FIntContainer
{
	int I;
};

}  // namespace FunctionalHPrivate

ZKZ_BEGIN_AUTOMATION_TEST(
	FFunctionalHTest,
	"Zakazane.ZakazaneUtilities.FunctionalH",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(ComposeCallsFunctionsInOrder)
{
	using namespace FunctionalHPrivate;

	//  FIntContainer | FIdentityFunctor
	{
		const auto Composed = TCompose{&FIntContainer::I, FSum{}};
		TestEqual("Composed function returns accumulated result", Composed(FIntContainer{42}), 42);
	}

	// FSum | FIdentityFunctor | FSum
	{
		const auto Composed = TCompose{FSum{}, FIdentityFunctor{}, FSum{}};
		TestEqual("Composed function returns accumulated result", Composed(3, 4), 7);
	}
}

ZKZ_END_AUTOMATION_TEST(FFunctionalHTest);

}  // namespace Zkz::Test
