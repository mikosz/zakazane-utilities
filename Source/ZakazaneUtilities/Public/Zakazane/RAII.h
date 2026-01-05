// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "ReturnIfMacros.h"

namespace Zkz
{

class FScopedExecution;

void Swap(FScopedExecution& Lhs, FScopedExecution& Rhs);

/// Binds a function execution at scope exit. Move constructible / move assignable, so allows to extend the lifetime
/// of the assignment outside the scope.
/// This can be achieved using ON_SCOPE_EXIT, however that is not move constructible.
///
/// Note that any objects bound to the function object must be valid when the function is called.
class ZAKAZANEUTILITIES_API FScopedExecution
{
public:
	FScopedExecution() = default;

	explicit FScopedExecution(TFunction<void()> InFunc);

	~FScopedExecution();

	// Copy-and-swap idiom (https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom)
	FScopedExecution(FScopedExecution&& Other);

	FScopedExecution& operator=(FScopedExecution Other);

	/// Unbinds the function without executing it.
	void Reset();

	/// Triggers the function if bound, and clears the scoped execution object
	void Trigger();

	/// Clears the scoped execution object and returns the bound function (if any).
	TFunction<void()> Release();

private:
	TFunction<void()> Func;

	friend void Zkz::Swap(FScopedExecution& Lhs, FScopedExecution& Rhs);
};

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
	TScopedAssignment(T& InTarget, T NewValue) : Target{&InTarget}, Restore{MoveTemp(InTarget)}
	{
		InTarget = MoveTemp(NewValue);
	}

	~TScopedAssignment()
	{
		ZKZ_RETURN_IF(Target == nullptr);
		using ::Swap;
		Swap(*Target, Restore);
	}

	TScopedAssignment(TScopedAssignment&& Other) : Target{MoveTemp(Other.Target)}, Restore{MoveTemp(Other.Restore)}
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

	T Restore;

	template <class U>
	friend void Zkz::Swap(TScopedAssignment<U>& Lhs, TScopedAssignment<U>& Rhs);
};

// -- template implementations

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
