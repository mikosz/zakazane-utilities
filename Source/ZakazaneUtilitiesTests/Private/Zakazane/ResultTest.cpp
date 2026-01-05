#include "Zakazane/Monostate.h"
#include "Zakazane/Result.h"
#include "Zakazane/Test/ConstructionReportingType.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::Test
{

ZKZ_BEGIN_AUTOMATION_TEST(
	FResultTest,
	"Zakazane.ZakazaneUtilities.Result",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(Construction)
{
	{
		struct FDefaultInt
		{
			int Value = 42;
		};

		constexpr TResult<FDefaultInt, int> DefaultValue;
		static_assert(DefaultValue.HasValue());
		static_assert(DefaultValue->Value == 42);
	}

	constexpr TResult<int, int> InPlaceValue{InPlace, 1};
	static_assert(InPlaceValue.HasValue());
	static_assert(InPlaceValue.GetValue() == 1);

	constexpr TResult<int, int> DirectValue{12};
	static_assert(DirectValue.HasValue());
	static_assert(DirectValue.GetValue() == 12);

	constexpr TResult<int, int> ConvertedValue{3.14f};
	static_assert(ConvertedValue.HasValue());
	static_assert(ConvertedValue.GetValue() == 3);

	constexpr TResult<int, int> ImplicitlyConvertedValue = []() -> TResult<int, int> { return 12; }();
	static_assert(ImplicitlyConvertedValue.HasValue());
	static_assert(ImplicitlyConvertedValue.GetValue() == 12);

	constexpr TResult<int, int> InPlaceError{Unexpect, 7};
	static_assert(InPlaceError.HasError());
	static_assert(InPlaceError.GetError() == 7);

	constexpr TResult<int, int> DirectError{TUnexpected{1}};
	static_assert(DirectError.HasError());
	static_assert(DirectError.GetError() == 1);

	constexpr TResult<int, int> ConvertedError{TUnexpected{char{2}}};
	static_assert(ConvertedError.HasError());
	static_assert(ConvertedError.GetError() == 2);

	constexpr TResult<int, int> ImplicitlyConvertedError = []() -> TResult<int, int> { return TUnexpected{char{3}}; }();
	static_assert(ImplicitlyConvertedError.HasError());
	static_assert(ImplicitlyConvertedError.GetError() == 3);

	constexpr TResult<int, int> MoveConstructedValue{TResult<int, int>{27}};
	static_assert(MoveConstructedValue.HasValue());
	static_assert(MoveConstructedValue.GetValue() == 27);

	{
		constexpr TResult<int, int> CopySource{72};
		constexpr TResult<int, int> CopyConstructedValue{CopySource};
		static_assert(CopyConstructedValue.HasValue());
		static_assert(CopyConstructedValue.GetValue() == 72);
	}

	{
		TResult<int, int> MoveSource{128};
		const TResult<int, int> MoveAssignedValue{MoveTemp(MoveSource)};
		TestTrue("Move-assigned value has value", MoveAssignedValue.HasValue());
		TestEqual("Move-assigned value equals source value", MoveAssignedValue.GetValue(), 128);
	}

	{
		// ReSharper disable once CppVariableCanBeMadeConstexpr
		const TResult<int, int> CopySource{256};
		TResult<int, int> CopyAssignedValue{CopySource};
		TestTrue("Copy-assigned value has value", CopyAssignedValue.HasValue());
		TestEqual("Copy-assigned value equals source value", CopyAssignedValue.GetValue(), 256);
	}

	TestTrue("Compile-time functionality", true);
}

ZKZ_ADD_TEST(VoidSpecializationConstruction)
{
	{
		struct FDefaultInt
		{
			int Value = 42;
		};

		constexpr TResult<void, int> DefaultValue;
		static_assert(DefaultValue.HasValue());
	}

	constexpr TResult<void, int> InPlaceError{Unexpect, 7};
	static_assert(InPlaceError.HasError());
	static_assert(InPlaceError.GetError() == 7);

	constexpr TResult<void, int> DirectError{TUnexpected{1}};
	static_assert(DirectError.HasError());
	static_assert(DirectError.GetError() == 1);

	constexpr TResult<void, int> ConvertedError{TUnexpected{char{2}}};
	static_assert(ConvertedError.HasError());
	static_assert(ConvertedError.GetError() == 2);

	constexpr TResult<void, int> ImplicitlyConvertedError = []() -> TResult<void, int>
	{ return TUnexpected{char{3}}; }();
	static_assert(ImplicitlyConvertedError.HasError());
	static_assert(ImplicitlyConvertedError.GetError() == 3);

	TestTrue("Compile-time functionality", true);
}

ZKZ_ADD_TEST(OkErrConstruction)
{
	constexpr TResult<int, char> OkConstructedValue = Ok(3.3);

	static_assert(OkConstructedValue.HasValue());
	static_assert(OkConstructedValue.GetValue() == 3);

	constexpr TResult<int, char> ErrConstructedValue = Err('c');

	static_assert(ErrConstructedValue.HasError());
	static_assert(ErrConstructedValue.GetError() == 'c');

	constexpr TResult<void, char> VoidOkConstructedValue = Ok();

	static_assert(VoidOkConstructedValue.HasValue());

	constexpr TResult<void, char> VoidErrConstructedValue = Err('c');

	static_assert(VoidErrConstructedValue.HasError());
	static_assert(VoidErrConstructedValue.GetError() == 'c');
}

ZKZ_ADD_TEST(MoveCopyValue)
{
	{
		FConstructionReport ConstructionReport;
		{
			[[maybe_unused]] TResult<FConstructionReportingType, int> InPlaceValue{InPlace, ConstructionReport};
		}
		ConstructionReport.TestConstructedInPlace(*this);
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		TResult<FConstructionReportingType, int> Source{InPlace, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<FConstructionReportingType, int> MoveConstructedValue{MoveTemp(Source)};
			Source->TestMovedFrom(*this, *MoveConstructedValue, TEXT("Copyable/moveable: "));
		}
		TargetConstructionReport.TestMoveConstructed(*this, TEXT("Copyable/moveable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		TResult<FConstructionReportingNoncopyable, int>
			Source{InPlace, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<FConstructionReportingNoncopyable, int> MoveConstructedValue{MoveTemp(Source)};
			Source->TestMovedFrom(*this, *MoveConstructedValue, TEXT("Noncopyable: "));
		}
		TargetConstructionReport.TestMoveConstructed(*this, TEXT("Noncopyable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		const TResult<FConstructionReportingType, int>
			Source{InPlace, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<FConstructionReportingType, int> CopyConstructedValue{Source};
			Source->TestCopiedFrom(*this, *CopyConstructedValue, TEXT("Copyable/moveable: "));
		}
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Copyable/moveable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		const TResult<FConstructionReportingNonmoveable, int>
			Source{InPlace, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<FConstructionReportingNonmoveable, int> CopyConstructedValue{Source};
			Source->TestCopiedFrom(*this, *CopyConstructedValue, TEXT("Non-moveable: "));
		}
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Non-moveable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		TResult<FConstructionReportingType, int> Source{InPlace, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<FConstructionReportingType, int>
				MoveAssignedValue{InPlace, TargetConstructionReport};
			MoveAssignedValue = MoveTemp(Source);
			Source->TestMovedFrom(*this, *MoveAssignedValue, TEXT("Copyable/moveable: "));
		}
		TargetConstructionReport.TestMoveAssigned(*this, TEXT("Copyable/moveable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		TResult<FConstructionReportingNoncopyable, int>
			Source{InPlace, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<FConstructionReportingNoncopyable, int>
				MoveAssignedValue{InPlace, TargetConstructionReport};
			MoveAssignedValue = MoveTemp(Source);
			Source->TestMovedFrom(*this, *MoveAssignedValue, TEXT("Noncopyable: "));
		}
		TargetConstructionReport.TestMoveAssigned(*this);
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		const TResult<FConstructionReportingType, int>
			Source{InPlace, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<FConstructionReportingType, int>
				CopyAssignedValue{InPlace, TargetConstructionReport};
			CopyAssignedValue = Source;
			Source->TestCopiedFrom(*this, *CopyAssignedValue, TEXT("Copyable/moveable: "));
		}
		TargetConstructionReport.TestCopyAssigned(*this, TEXT("Copyable/moveable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		const TResult<FConstructionReportingNonmoveable, int>
			Source{InPlace, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<FConstructionReportingNonmoveable, int>
				CopyAssignedValue{InPlace, TargetConstructionReport};
			CopyAssignedValue = Source;
			Source->TestCopiedFrom(*this, *CopyAssignedValue, TEXT("Non-moveable: "));
		}
		TargetConstructionReport.TestCopyAssigned(*this, TEXT("Non-moveable: "));
	}
}

ZKZ_ADD_TEST(MoveCopyError)
{
	{
		FConstructionReport ConstructionReport;
		{
			[[maybe_unused]] TResult<int, FConstructionReportingType> InPlaceError{Unexpect, ConstructionReport};
		}
		ConstructionReport.TestConstructedInPlace(*this);
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		TResult<int, FConstructionReportingType> Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<int, FConstructionReportingType> MoveConstructedError{MoveTemp(Source)};
			Source.GetError()
				.TestMovedFrom(*this, MoveConstructedError.GetError(), TEXT("Copyable/moveable constructed: "));
		}
		TargetConstructionReport.TestMoveConstructed(*this, TEXT("Copyable/moveable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		TResult<int, FConstructionReportingNoncopyable>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<int, FConstructionReportingNoncopyable> MoveConstructedError{MoveTemp(Source)};
			Source.GetError().TestMovedFrom(*this, MoveConstructedError.GetError(), TEXT("Noncopyable constructed: "));
		}
		TargetConstructionReport.TestMoveConstructed(*this, TEXT("Noncopyable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		const TResult<int, FConstructionReportingType>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<int, FConstructionReportingType> CopyConstructedError{Source};
			Source.GetError()
				.TestCopiedFrom(*this, CopyConstructedError.GetError(), TEXT("Copyable/moveable constructed: "));
		}
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Copyable/moveable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		const TResult<int, FConstructionReportingNonmoveable>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<int, FConstructionReportingNonmoveable> CopyConstructedError{Source};
			Source.GetError()
				.TestCopiedFrom(*this, CopyConstructedError.GetError(), TEXT("Non-moveable constructed: "));
		}
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Non-moveable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		TResult<int, FConstructionReportingType> Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<int, FConstructionReportingType>
				MoveAssignedError{Unexpect, TargetConstructionReport};
			MoveAssignedError = MoveTemp(Source);
			Source.GetError().TestMovedFrom(*this, MoveAssignedError.GetError(), TEXT("Copyable/moveable assigned: "));
		}
		TargetConstructionReport.TestMoveAssigned(*this);
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		TResult<int, FConstructionReportingNoncopyable>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<int, FConstructionReportingNoncopyable>
				MoveAssignedError{Unexpect, TargetConstructionReport};
			MoveAssignedError = MoveTemp(Source);
			Source.GetError().TestMovedFrom(*this, MoveAssignedError.GetError(), TEXT("Noncopyable assigned: "));
		}
		TargetConstructionReport.TestMoveAssigned(*this, TEXT("Noncopyable"));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		const TResult<int, FConstructionReportingType>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<int, FConstructionReportingType>
				CopyAssignedError{Unexpect, TargetConstructionReport};
			CopyAssignedError = Source;
			Source.GetError().TestCopiedFrom(*this, CopyAssignedError.GetError(), TEXT("Copyable/moveable assigned: "));
		}
		TargetConstructionReport.TestCopyAssigned(*this, TEXT("Copyable/moveable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		const TResult<int, FConstructionReportingNonmoveable>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<int, FConstructionReportingNonmoveable>
				CopyAssignedError{Unexpect, TargetConstructionReport};
			CopyAssignedError = Source;
			Source.GetError().TestCopiedFrom(*this, CopyAssignedError.GetError(), TEXT("Non-moveable assigned: "));
		}
		TargetConstructionReport.TestCopyAssigned(*this, TEXT("Non-moveable: "));
	}
}

ZKZ_ADD_TEST(ValueAccessorsMoveOrGetReference)
{
	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		TResult<FConstructionReportingType, int> Source{InPlace, SourceConstructionReport, TargetConstructionReport};

		const FConstructionReportingType ValueRef{Source.GetValue()};
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Non-const: "));
		SourceConstructionReport.TestCopiedFrom(*this, TEXT("Non-const: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		TResult<FConstructionReportingType, int> Source{InPlace, SourceConstructionReport, TargetConstructionReport};

		const FConstructionReportingType ValueMoved{MoveTemp(Source).GetValue()};
		Source->TestMovedFrom(*this, ValueMoved, TEXT("Non-const: "));
		TargetConstructionReport.TestMoveConstructed(*this, TEXT("Non-const: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		const TResult<FConstructionReportingType, int>
			Source{InPlace, SourceConstructionReport, TargetConstructionReport};

		const FConstructionReportingType ValueRef{Source.GetValue()};
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Const: "));
		SourceConstructionReport.TestCopiedFrom(*this, TEXT("Const: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		const TResult<FConstructionReportingType, int>
			Source{InPlace, SourceConstructionReport, TargetConstructionReport};

		const FConstructionReportingType ValueMoved{std::move(Source).GetValue()};
		Source->TestCopiedFrom(*this, ValueMoved, TEXT("Const: "));
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Const: "));
	}
}

ZKZ_ADD_TEST(ErrorAccessorsMoveOrGetReference)
{
	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		TResult<int, FConstructionReportingType> Source{Unexpect, SourceConstructionReport, TargetConstructionReport};

		const FConstructionReportingType ErrorRef{Source.GetError()};
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Non-const: "));
		SourceConstructionReport.TestCopiedFrom(*this, TEXT("Non-const: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		TResult<int, FConstructionReportingType> Source{Unexpect, SourceConstructionReport, TargetConstructionReport};

		const FConstructionReportingType ErrorMoved{MoveTemp(Source).GetError()};
		Source.GetError().TestMovedFrom(*this, ErrorMoved, TEXT("Non-const: "));
		TargetConstructionReport.TestMoveConstructed(*this, TEXT("Non-const: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		const TResult<int, FConstructionReportingType>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};

		const FConstructionReportingType ErrorRef{Source.GetError()};
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Const: "));
		SourceConstructionReport.TestCopiedFrom(*this, TEXT("Const: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		const TResult<int, FConstructionReportingType>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};

		const FConstructionReportingType ErrorMoved{std::move(Source).GetError()};
		Source.GetError().TestCopiedFrom(*this, ErrorMoved, TEXT("Const: "));
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Const: "));
	}
}

ZKZ_ADD_TEST(VoidSpecializationErrorAccessorsMoveOrGetReference)
{
	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		TResult<void, FConstructionReportingType> Source{Unexpect, SourceConstructionReport, TargetConstructionReport};

		const FConstructionReportingType ErrorRef{Source.GetError()};
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Non-const: "));
		SourceConstructionReport.TestCopiedFrom(*this, TEXT("Non-const: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		TResult<void, FConstructionReportingType> Source{Unexpect, SourceConstructionReport, TargetConstructionReport};

		const FConstructionReportingType ErrorMoved{MoveTemp(Source).GetError()};
		Source.GetError().TestMovedFrom(*this, ErrorMoved, TEXT("Non-const: "));
		TargetConstructionReport.TestMoveConstructed(*this, TEXT("Non-const: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		const TResult<void, FConstructionReportingType>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};

		const FConstructionReportingType ErrorRef{Source.GetError()};
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Const: "));
		SourceConstructionReport.TestCopiedFrom(*this, TEXT("Const: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		const TResult<void, FConstructionReportingType>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};

		const FConstructionReportingType ErrorMoved{std::move(Source).GetError()};
		Source.GetError().TestCopiedFrom(*this, ErrorMoved, TEXT("Const: "));
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Const: "));
	}
}

ZKZ_ADD_TEST(VoidSpecializationMoveCopyError)
{
	{
		FConstructionReport ConstructionReport;
		{
			[[maybe_unused]] TResult<void, FConstructionReportingType> InPlaceError{Unexpect, ConstructionReport};
		}
		ConstructionReport.TestConstructedInPlace(*this);
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		TResult<void, FConstructionReportingType> Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<void, FConstructionReportingType> MoveConstructedError{MoveTemp(Source)};
			Source.GetError().TestMovedFrom(*this, MoveConstructedError.GetError(), TEXT("Copyable/moveable: "));
		}
		TargetConstructionReport.TestMoveConstructed(*this, TEXT("Copyable/moveable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		TResult<void, FConstructionReportingNoncopyable>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<void, FConstructionReportingNoncopyable> MoveConstructedError{MoveTemp(Source)};
			Source.GetError().TestMovedFrom(*this, MoveConstructedError.GetError(), TEXT("Noncopyable: "));
		}
		TargetConstructionReport.TestMoveConstructed(*this, TEXT("Noncopyable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		const TResult<void, FConstructionReportingType>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<void, FConstructionReportingType> CopyConstructedError{Source};
			Source.GetError().TestCopiedFrom(*this, CopyConstructedError.GetError(), TEXT("Copyable/moveable: "));
		}
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Copyable/moveable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		const TResult<void, FConstructionReportingNonmoveable>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<void, FConstructionReportingNonmoveable> CopyConstructedError{Source};
			Source.GetError().TestCopiedFrom(*this, CopyConstructedError.GetError(), TEXT("Non-moveable: "));
		}
		TargetConstructionReport.TestCopyConstructed(*this, TEXT("Non-moveable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		TResult<void, FConstructionReportingType> Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<void, FConstructionReportingType>
				MoveAssignedError{Unexpect, TargetConstructionReport};
			MoveAssignedError = MoveTemp(Source);
			Source.GetError().TestMovedFrom(*this, MoveAssignedError.GetError(), TEXT("Copyable/moveable: "));
		}
		TargetConstructionReport.TestMoveAssigned(*this, TEXT("Copyable/moveable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		TResult<void, FConstructionReportingNoncopyable>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<void, FConstructionReportingNoncopyable>
				MoveAssignedError{Unexpect, TargetConstructionReport};
			MoveAssignedError = MoveTemp(Source);
			Source.GetError().TestMovedFrom(*this, MoveAssignedError.GetError(), TEXT("Noncopyable: "));
		}
		TargetConstructionReport.TestMoveAssigned(*this, TEXT("Noncopyable: "));
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		const TResult<void, FConstructionReportingType>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<void, FConstructionReportingType>
				CopyAssignedError{Unexpect, TargetConstructionReport};
			CopyAssignedError = Source;
			Source.GetError().TestCopiedFrom(*this, CopyAssignedError.GetError(), TEXT("Copyable/moveable: "));
		}
		TargetConstructionReport.TestCopyAssigned(*this);
	}

	{
		FConstructionReport SourceConstructionReport, TargetConstructionReport;
		// ReSharper disable once CppTooWideScope : need to not be destructed before test
		const TResult<void, FConstructionReportingNonmoveable>
			Source{Unexpect, SourceConstructionReport, TargetConstructionReport};
		{
			[[maybe_unused]] TResult<void, FConstructionReportingNonmoveable>
				CopyAssignedError{Unexpect, TargetConstructionReport};
			CopyAssignedError = Source;
			Source.GetError().TestCopiedFrom(*this, CopyAssignedError.GetError(), TEXT("Non-moveable: "));
		}
		TargetConstructionReport.TestCopyAssigned(*this, TEXT("Non-moveable: "));
	}
}

ZKZ_ADD_TEST(AndThen)
{
	{
		bool bCalled = false;
		const TResult<int, FMonostate> AndThenResult = TResult<int, FMonostate>{char{3}}.AndThen(
			[this, &bCalled](const int Value)
			{
				bCalled = true;
				TestEqual("AndThen Function gets value as argument", Value, 3);
				return TResult<int, FMonostate>{12};
			});

		TestTrue("AndThen Function called for value", bCalled);
		TestTrue("AndThen returns Function result when called for value - HasValue", AndThenResult.HasValue());
		TestEqual("AndThen returns Function result when called for value - GetError", AndThenResult.GetValue(), 12);
	}

	{
		constexpr TResult<FMonostate, int> AndThenResult =
			TResult<FMonostate, char>{Unexpect, char{3}}.AndThen([](FMonostate) { return Err(-1); });

		static_assert(AndThenResult.HasError());
		static_assert(AndThenResult.GetError() == 3);
	}
}

ZKZ_ADD_TEST(VoidSpecializationAndThen)
{
	{
		bool bCalled = false;
		const TResult<int, int> AndThenResult = TResult<void, char>{}.AndThen(
			[this, &bCalled]
			{
				bCalled = true;
				return TResult<int, int>{Unexpect, 12};
			});

		TestTrue("AndThen Function called for value", bCalled);
		TestTrue("AndThen returns Function result when called for value - HasError", AndThenResult.HasError());
		TestEqual("AndThen returns Function result when called for value - GetError", AndThenResult.GetError(), 12);
	}

	{
		bool bCalled = false;
		// ReSharper disable once CppVariableCanBeMadeConstexpr
		const TResult<int, int> AndThenResult = TResult<void, char>{Unexpect, char{3}}.AndThen(
			[this, &bCalled]
			{
				bCalled = true;
				return TResult<int, int>{Unexpect, -1};
			});

		TestTrue("AndThen Function not called for value", !bCalled);
		TestTrue("AndThen returns original error when called for error - HasError", AndThenResult.HasError());
		TestEqual("AndThen returns original error when called for error - GetError", AndThenResult.GetError(), 3);
	}
}

ZKZ_ADD_TEST(OrElse)
{
	{
		bool bCalled = false;
		const TResult<FMonostate, int> OrElseResult = TResult<FMonostate, char>{Unexpect, char{3}}.OrElse(
			[this, &bCalled](const char Error)
			{
				bCalled = true;
				return TResult<FMonostate, int>{Unexpect, 42};
			});

		TestTrue("OrElse Function called for error", bCalled);
		TestTrue("OrElse returns Function result when called for error - HasError", OrElseResult.HasError());
		TestEqual("OrElse returns Function result when called for error - GetError", OrElseResult.GetError(), 42);
	}

	{
		constexpr TResult<int, FMonostate> OrElseResult =
			TResult<int, FMonostate>{char{3}}.OrElse([](FMonostate) { return Ok(-1); });

		static_assert(OrElseResult.HasValue());
		static_assert(OrElseResult.GetValue() == 3);
	}
}

ZKZ_ADD_TEST(VoidSpecializationOrElse)
{
	{
		bool bCalled = false;
		// ReSharper disable once CppVariableCanBeMadeConstexpr
		const TResult<int, int> OrElseResult = TResult<void, char>{}.OrElse(
			[this, &bCalled](const char Error)
			{
				bCalled = true;
				return TResult<int, int>{Unexpect, -1};
			});

		TestTrue("OrElse Function called for value", !bCalled);
		TestTrue("OrElse returns Function result when called for value - HasValue", OrElseResult.HasValue());
	}

	{
		bool bCalled = false;
		// ReSharper disable once CppVariableCanBeMadeConstexpr
		const TResult<int, int> OrElseResult = TResult<void, char>{Unexpect, char{3}}.OrElse(
			[this, &bCalled](const char Error)
			{
				bCalled = true;
				TestEqual("OrElse Function gets error as argument", Error, 3);
				return TResult<int, int>{Unexpect, 12};
			});

		TestTrue("OrElse Function not called for value", bCalled);
		TestTrue("OrElse returns original error when called for error - HasError", OrElseResult.HasError());
		TestEqual("OrElse returns original error when called for error - GetError", OrElseResult.GetError(), 12);
	}
}

ZKZ_ADD_TEST(OperatorBoolAndDereference)
{
	struct FInt
	{
		int I = 0;

		explicit constexpr FInt(const int InI) : I(InI)
		{
		}

		constexpr int GetI() const
		{
			return I;
		}
	};

	constexpr TResult<FInt, int> Value{InPlace, 1};
	static_assert(Value);
	static_assert(!!Value);
	// ReSharper disable once CppRedundantDereferencingAndTakingAddress
	static_assert((*Value).I == 1);
	static_assert(Value->I == 1);
	static_assert(Value->GetI() == 1);

	constexpr TResult<int, int> Error{Unexpect, 7};
	static_assert(!Error);
}

ZKZ_ADD_TEST(CollapseNestedResults)
{
	struct FOuterError
	{
		int OuterErrorValue;
	};

	struct FInnerError
	{
		int InnerErrorValue;

		explicit constexpr FInnerError(const int InInnerErrorValue) : InnerErrorValue{InInnerErrorValue}
		{
		}
	};

	struct FInnerErrorImplicit
	{
		int DoubleOuterErrorValue;

		// ReSharper disable once CppNonExplicitConvertingConstructor
		constexpr FInnerErrorImplicit(const FOuterError Outer) : DoubleOuterErrorValue{Outer.OuterErrorValue * 2}
		{
		}
	};

	// Uses const TResult& version
	constexpr TResult<TResult<int, FInnerError>, FOuterError> NestedNonConvertibleResult{Unexpect, 3};
	constexpr TResult<int, FInnerError> CollapsedNonConvertibleResult =
		CollapseNestedResults(NestedNonConvertibleResult, &FOuterError::OuterErrorValue);
	static_assert(CollapsedNonConvertibleResult.HasError());
	static_assert(CollapsedNonConvertibleResult.GetError().InnerErrorValue == 3);

	// Uses TResult&& version
	constexpr TResult<int, FInnerErrorImplicit> ConvertibleResult =
		CollapseNestedResults(TResult<TResult<int, FInnerErrorImplicit>, FOuterError>{Unexpect, 12});
	static_assert(ConvertibleResult.HasError());
	static_assert(ConvertibleResult.GetError().DoubleOuterErrorValue == 24);

	TestTrue("Compile-time tests", true);
}

ZKZ_END_AUTOMATION_TEST(FResultTest);

}  // namespace Zkz::Test
