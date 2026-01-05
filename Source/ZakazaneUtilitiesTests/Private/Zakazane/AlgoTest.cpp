#include "Algo/Find.h"
#include "Templates/Greater.h"
#include "Zakazane/Algo.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::Test
{

ZKZ_BEGIN_AUTOMATION_TEST(
	FAlgoTest,
	"Zakazane.ZakazaneUtilities.Algo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(PointerToIndex)
{
	const TArray ArrayOfInts = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	const int CArrayOfInts[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	TestEqual("TArray PointerToIndex", PointerToIndex(ArrayOfInts, Algo::Find(ArrayOfInts, 3)), 3);
	TestEqual(
		"CArray PointerToIndex",
		PointerToIndex(MakeArrayView(CArrayOfInts), Algo::Find(MakeArrayView(CArrayOfInts), 3)),
		3);
}

ZKZ_ADD_TEST(MinBy)
{
	const TArray<int> EmptyArrayOfInts;
	const TArray ArrayOfInts = {0, 1, 2, 3, 4, -90, 6, 7, 8, 9};

	TestTrue("Min of empty ints", Min(EmptyArrayOfInts) == NullOpt);

	TestEqual("Min", Min(ArrayOfInts).Get(0), -90);
	TestEqual("Min with comparator", Min(ArrayOfInts, TGreater<>{}).Get(0), 9);
	TestEqual("MinBy", MinBy(ArrayOfInts, [](const auto V) { return -V; }).Get(0), -9);
	TestEqual("MinBy with comparator", MinBy(ArrayOfInts, [](const auto V) { return -V; }, TGreater<>{}).Get(0), 90);
}

ZKZ_END_AUTOMATION_TEST(FAlgoTest);

}  // namespace Zkz::Test
