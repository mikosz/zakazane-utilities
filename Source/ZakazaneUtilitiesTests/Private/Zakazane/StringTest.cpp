#include "Zakazane/String.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::String::Test
{

ZKZ_BEGIN_AUTOMATION_TEST(
	FStringTest,
	"Zakazane.ZakazaneUtilities.String",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(GetRightmostSegmentsTest)
{
	TestEqual("Empty", GetRightmostSegments(FStringView{}, TCHAR{'.'}, 3), FStringView{});

	TestEqual("0", GetRightmostSegments(TEXT("abc.def.ghi"), TCHAR{'.'}, 0), FStringView{});
	TestEqual("1", GetRightmostSegments(TEXT("abc.def.ghi"), TCHAR{'.'}, 1), FStringView{TEXT("ghi")});
	TestEqual("2", GetRightmostSegments(TEXT("abc.def.ghi"), TCHAR{'.'}, 2), FStringView{TEXT("def.ghi")});
	TestEqual("3", GetRightmostSegments(TEXT("abc.def.ghi"), TCHAR{'.'}, 3), FStringView{TEXT("abc.def.ghi")});
	TestEqual("4", GetRightmostSegments(TEXT("abc.def.ghi"), TCHAR{'.'}, 4), FStringView{TEXT("abc.def.ghi")});
}

ZKZ_ADD_TEST(AbbreviateTest)
{
	{
		const FString S{"I dream'd a dream to-night!"};
		TestEqual("FString unabbreviated", Abbreviate(S, S.Len(), TEXT("[...]")), S);
		TestEqual("FString abbreviated", Abbreviate(S, 16, TEXT("[...]")), TEXT("I dream'd a[...]"));
		TestEqual("FString ending only", Abbreviate(S.Left(3), 2, TEXT("[...]")), TEXT("[...]"));
	}

	{
		const FAnsiString S{"I dream'd a dream to-night!"};
		TestEqual("FAnsiString unabbreviated", Abbreviate(S, S.Len(), "[...]"), S);
		TestEqual("FAnsiString abbreviated", Abbreviate(S, 16, "[...]"), TEXT("I dream'd a[...]"));
		TestEqual("FAnsiString ending only", Abbreviate(S.Left(3), 2, "[...]"), "[...]");
	}
}

ZKZ_END_AUTOMATION_TEST(FStringTest);

}  // namespace Zkz::String::Test
