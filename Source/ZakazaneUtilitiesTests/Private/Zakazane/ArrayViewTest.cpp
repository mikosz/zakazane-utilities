#include "Algo/Find.h"
#include "Zakazane/ArrayView.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::Test
{

ZKZ_BEGIN_AUTOMATION_TEST(
	FArrayViewTest,
	"Zakazane.ZakazaneUtilities.ArrayView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(MakeConstPtrArrayViewConstifiesPointer)
{
	{
		TArray<int*> ArrayOfPointers;
		[[maybe_unused]] decltype(auto) Result = MakeConstPtrArrayView(ArrayOfPointers);

		static_assert(std::is_same_v<decltype(Result), TArrayView<const int*>>);
	}

	{
		int* IntPtr1 = nullptr;
		int* IntPtr2 = nullptr;
		[[maybe_unused]] decltype(auto) Result = MakeConstPtrArrayView({IntPtr1, IntPtr2});

		static_assert(std::is_same_v<decltype(Result), TArrayView<const int*>>);
	}
}

ZKZ_END_AUTOMATION_TEST(FArrayViewTest);

}  // namespace Zkz::Test
