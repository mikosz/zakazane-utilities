#include "Zakazane/MultisourceValue.h"

namespace Zkz
{

namespace MultisourceValuePrivate
{

static bool IdEquals(auto Lhs, auto Rhs)
{
	return false;
}

static bool IdEquals(auto* LhsPtr, auto* RhsPtr)
{
	return LhsPtr == RhsPtr;
}

static bool IdEquals(FName Lhs, FName Rhs)
{
	return Lhs == Rhs;
}

static FName GetName(const FName Name)
{
	return Name;
}

static FName GetName(const UObject* const Object)
{
	ZKZ_RETURN_IF_INVALID(Object, NAME_None);
	return Object->GetFName();
}

static FName GetName(const void* const Ptr)
{
	return NAME_None;
}

}  // namespace MultisourceValuePrivate

uint32 GetTypeHash(const FMultisourceIdType& MultisourceId)
{
	using ::GetTypeHash;
	return Visit([](const auto V) -> uint32 { return GetTypeHash(V); }, MultisourceId.Id);
}

bool operator==(const FMultisourceIdType& Lhs, const FMultisourceIdType& Rhs)
{
	using namespace MultisourceValuePrivate;
	return Visit([](auto LhsV, auto RhsV) { return IdEquals(LhsV, RhsV); }, Lhs.Id, Rhs.Id);
}

FMultisourceIdType::FMultisourceIdType(const void* Ptr) : Id{TInPlaceType<const void*>{}, Ptr}
{
}

FMultisourceIdType::FMultisourceIdType(const UObject* Object) : Id{TInPlaceType<const UObject*>{}, Object}
{
}

FMultisourceIdType::FMultisourceIdType(const FName Name) : Id{TInPlaceType<FName>{}, Name}
{
}

FName FMultisourceIdType::GetName() const
{
	return Visit([](const auto V) { return MultisourceValuePrivate::GetName(V); }, Id);
}

}  // namespace Zkz
