// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "ReturnIfMacros.h"

namespace Zkz
{

template <class T>
concept TScopedAssignableValue = std::is_move_constructible_v<T> && std::is_move_assignable_v<T>;

template <TScopedAssignableValue T>
class TScopedAssignment;

template <class T>
void Swap(TScopedAssignment<T>& Lhs, TScopedAssignment<T>& Rhs);

/// Assigns a new value to a referenced value and restores it to the original on destruction.
/// Move constructible /  move assignable, so allows to extend the lifetime of the assignment outside the scope.
/// @tparam T must be move-constructible and move-assignable.
template <TScopedAssignableValue T>
class TScopedAssignment
{
public:
	TScopedAssignment(T& InTarget, T NewValue) : Target{&InTarget}, Value{MoveTemp(InTarget)}
	{
		InTarget = MoveTemp(NewValue);
	}

	~TScopedAssignment()
	{
		ZKZ_RETURN_IF(Target == nullptr);
		using ::Swap;
		Swap(*Target, Value);
	}

	TScopedAssignment(TScopedAssignment&& Other) : Target{MoveTemp(Other.Target)}, Value{MoveTemp(Other.Value)}
	{
		Other.Target = nullptr;
	}

	// Copy-and-swap idiom (https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom)
	TScopedAssignment& operator=(TScopedAssignment Other)
	{
		Swap(*this, Other);
		return *this;
	}

private:
	T* Target = nullptr;

	T Value;

	template <class U>
	friend void Zkz::Swap(TScopedAssignment<U>& Lhs, TScopedAssignment<U>& Rhs);
};

template <class T, class U>
TScopedAssignment(T&, U&&) -> TScopedAssignment<T>;

template <class T>
void Swap(TScopedAssignment<T>& Lhs, TScopedAssignment<T>& Rhs)
{
	using ::Swap;
	Swap(Lhs.Target, Rhs.Target);
	Swap(Lhs.Value, Rhs.Value);
}

}  // namespace Zkz
