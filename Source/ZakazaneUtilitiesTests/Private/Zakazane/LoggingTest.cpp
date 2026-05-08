#include "Zakazane/Logging.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::Test
{

DECLARE_LOG_CATEGORY_EXTERN(LogLoggingTest, Display, All);
DEFINE_LOG_CATEGORY(LogLoggingTest);

ZKZ_BEGIN_AUTOMATION_TEST(
	FLoggingTest,
	"Zakazane.ZakazaneUtilities.Logging",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(LogCategoryRefCanBeUsedWithUE_LOG)
{
	AddExpectedError(TEXT("GREAT SUCCESS"), EAutomationExpectedErrorFlags::Exact, 1);

	TLogCategoryRef LogCategory{LogLoggingTest};
	UE_LOG(LogCategory, Error, TEXT("GREAT SUCCESS"));
}

ZKZ_END_AUTOMATION_TEST(FLoggingTest);

}  // namespace Zkz::Test
