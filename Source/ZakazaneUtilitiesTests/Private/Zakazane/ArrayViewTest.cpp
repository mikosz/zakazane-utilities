#include "Algo/Find.h"
#include "Zakazane/ArrayView.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::Test
{

ZKZ_BEGIN_AUTOMATION_TEST(
	FArrayViewTest,
	"Zakazane.ZakazaneUtilities.ArrayView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(MakePtrToConstArrayViewConstifiesPointer)
{
	{
		TArray<int*> ArrayOfPointers;
		[[maybe_unused]] decltype(auto) Result = MakePtrToConstArrayView(ArrayOfPointers);

		static_assert(std::is_same_v<decltype(Result), TArrayView<const int*>>);
	}

	{
		int* IntPtr1 = nullptr;
		int* IntPtr2 = nullptr;
		[[maybe_unused]] decltype(auto) Result = MakePtrToConstArrayView({IntPtr1, IntPtr2});

		static_assert(std::is_same_v<decltype(Result), TArrayView<const int*>>);
	}
}

ZKZ_ADD_TEST(ArrayViewGetTypeHashEqualsArray)
{
	const TArray ArrOfInts = {0, 1, 2, 3};
	const TConstArrayView<int> View = ArrOfInts;

	TestEqual("View hash type equals array", GetArrayViewTypeHash(View), GetTypeHash(ArrOfInts));

	const TConstArrayView PartialView = View.LeftChop(1).RightChop(1);

	TestEqual(
		"Partial View hash type equals array",
		GetArrayViewTypeHash(PartialView),
		GetTypeHash(TArray<int>{PartialView}));
}

ZKZ_END_AUTOMATION_TEST(FArrayViewTest);

}  // namespace Zkz::Test
