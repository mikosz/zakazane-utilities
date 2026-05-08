// ReSharper disable CppRedundantQualifier : want to ensure Zkz::Pointer:: functions are used (as opposed to
// other functions being picked up by argument dependent lookup, or from the global namespace)

#include "PointerTest.h"

#include "Zakazane/Pointer.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::Pointer::Test
{
namespace Private
{

template <class T, bool Result>
struct TAssertTypeLifetimeTracking
{
	static_assert(CLifetimeTrackingPtr<T> == Result);
	static_assert(CLifetimeTrackingPtr<const T> == Result);
};

}  // namespace Private

struct FFakeSmartPtr
{
	mutable bool bIsValidCalled = false;
	mutable bool bGetCalled = false;

	bool IsValid() const
	{
		bIsValidCalled = true;
		return true;
	}

	int* Get() const
	{
		static int I = 42;
		bGetCalled = true;
		return &I;
	}
};

ZKZ_BEGIN_AUTOMATION_TEST(
	FPointerTest,
	"Zakazane.ZakazaneUtilities.Pointer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(IsValidIsNullptrComparisonForRawPtrs)
{
	int A;
	int* APtr = &A;
	int* const ACPtr = &A;
	const int* CAPtr = &A;
	const int* const CACPtr = &A;

	int* N = nullptr;

	TestTrue("int*", Zkz::Pointer::IsValid(APtr));
	TestTrue("int* const", Zkz::Pointer::IsValid(ACPtr));
	TestTrue("const int*", Zkz::Pointer::IsValid(CAPtr));
	TestTrue("const int* const", Zkz::Pointer::IsValid(CACPtr));

	TestFalse("nullptr", Zkz::Pointer::IsValid(N));
}

ZKZ_ADD_TEST(IsValidCallsIsValidIfMemberFunction)
{
	constexpr FFakeSmartPtr SmartPtr;

	TestTrue("IsValid returns true", Zkz::Pointer::IsValid(SmartPtr));
	TestTrue("Member function called", SmartPtr.bIsValidCalled);
}

ZKZ_ADD_TEST(IsValidIsTrueForSharedRef)
{
	const TSharedRef<int> SharedRef = MakeShared<int>(42);

	TestTrue("IsValid returns true", Zkz::Pointer::IsValid(SharedRef));
}

ZKZ_ADD_TEST(IsValidCallsUObjectIsValidForUObjectPtr)
{
	UObject* const Obj = NewObject<UZkzPointerTestObject>();
	const TObjectPtr<UObject> ObjPtr{Obj};
	const TObjectPtr<const UObject> ConstObjPtr{Obj};
	TWeakObjectPtr WeakObj{Obj};

	TestTrue("Sanity check: global returns true", ::IsValid(Obj));
	TestTrue("IsValid returns true", Zkz::Pointer::IsValid(Obj));
	TestTrue("IsValid returns true for WeakObjectPtr", Zkz::Pointer::IsValid(WeakObj));
	TestTrue("IsValid returns true for TObjectPtr", Zkz::Pointer::IsValid(ObjPtr));
	TestTrue("IsValid returns true for TObjectPtr to const", Zkz::Pointer::IsValid(ConstObjPtr));

	Obj->MarkAsGarbage();

	TestFalse("Sanity check: global returns false", ::IsValid(Obj));
	TestFalse("IsValid returns false", Zkz::Pointer::IsValid(Obj));
	TestFalse("IsValid returns false for WeakObjectPtr", Zkz::Pointer::IsValid(WeakObj));
	TestFalse("IsValid returns false for TObjectPtr", Zkz::Pointer::IsValid(ObjPtr));
	TestFalse("IsValid returns false for TObjectPtr to const", Zkz::Pointer::IsValid(ConstObjPtr));
}

ZKZ_ADD_TEST(GetIsNullptrComparisonForRawPtrs)
{
	int A;
	int* APtr = &A;
	int* const ACPtr = &A;
	const int* CAPtr = &A;
	const int* const CACPtr = &A;

	TestEqual("int*", Get(APtr), APtr);
	TestEqual("int* const", Get(ACPtr), ACPtr);
	TestEqual("const int*", Get(CAPtr), CAPtr);
	TestEqual("const int* const", Get(CACPtr), CACPtr);
}

ZKZ_ADD_TEST(GetCallsGetIfMemberFunction)
{
	constexpr FFakeSmartPtr SmartPtr;

	TestEqual("Get returns ptr", Get(SmartPtr), SmartPtr.Get());
	TestTrue("Member function called", SmartPtr.bGetCalled);
}

ZKZ_ADD_TEST(GetReturnsPtrFromSharedRef)
{
	const TSharedRef<int> SharedRef = MakeShared<int>(42);

	TestEqual("Get returns ptr", Get(SharedRef), &SharedRef.Get());
}

ZKZ_ADD_TEST(CLifetimeTrackingPtrMatchesLifetimeTrackingPointers)
{
	using namespace Private;

	TAssertTypeLifetimeTracking<TWeakObjectPtr<UZkzPointerTestObject>, true>();
	TAssertTypeLifetimeTracking<TWeakObjectPtr<const UZkzPointerTestObject>, true>();

	TAssertTypeLifetimeTracking<TStrongObjectPtr<UZkzPointerTestObject>, true>();
	TAssertTypeLifetimeTracking<TStrongObjectPtr<const UZkzPointerTestObject>, true>();

	TAssertTypeLifetimeTracking<TWeakPtr<int>, true>();
	TAssertTypeLifetimeTracking<TWeakPtr<const int>, true>();
	TAssertTypeLifetimeTracking<TWeakPtr<int, ESPMode::NotThreadSafe>, true>();

	TAssertTypeLifetimeTracking<TUniquePtr<int>, true>();
	TAssertTypeLifetimeTracking<TUniquePtr<const int>, true>();
	TAssertTypeLifetimeTracking<TUniquePtr<int, TIdentity<int*>>, true>();
	TAssertTypeLifetimeTracking<TUniquePtr<int[]>, true>();

	TAssertTypeLifetimeTracking<TSharedPtr<int>, true>();
	TAssertTypeLifetimeTracking<TSharedPtr<const int>, true>();
	TAssertTypeLifetimeTracking<TSharedPtr<int, ESPMode::NotThreadSafe>, true>();
	TAssertTypeLifetimeTracking<TSharedPtr<int[]>, true>();

	TAssertTypeLifetimeTracking<TSharedRef<int>, true>();
	TAssertTypeLifetimeTracking<TSharedRef<const int>, true>();
	TAssertTypeLifetimeTracking<TSharedRef<int, ESPMode::NotThreadSafe>, true>();
	TAssertTypeLifetimeTracking<TSharedRef<int[]>, true>();

	TAssertTypeLifetimeTracking<TObjectPtr<UZkzPointerTestObject>, false>();
	TAssertTypeLifetimeTracking<TObjectPtr<const UZkzPointerTestObject>, false>();

	TAssertTypeLifetimeTracking<UZkzPointerTestObject*, false>();
	TAssertTypeLifetimeTracking<const UZkzPointerTestObject*, false>();
}

ZKZ_END_AUTOMATION_TEST(FPointerTest);

}  // namespace Zkz::Pointer::Test
