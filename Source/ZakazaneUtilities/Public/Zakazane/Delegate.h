// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Zkz
{

namespace Private
{

template <class T>
struct THasRemoveDelegateHandleMemberFunction
{
private:
	template <class U>
	static auto Test(U* Ptr) -> decltype(Ptr->Remove(FDelegateHandle{}), std::true_type{});

	template <class>
	static std::false_type Test(...);

public:
	static constexpr bool Value = decltype(Test<T>(nullptr))::value;
};

template <class T>
constexpr bool THasRemoveDelegateHandleMemberFunctionV = THasRemoveDelegateHandleMemberFunction<T>::Value;

}  // namespace Private

/// RAII type for delegate handles. Automatically unbinds the handle from the delegate at destruction.
template <class DelegateT>
class [[nodiscard]] TScopedDelegateHandle
{
public:
	using DelegateType = DelegateT;

	TScopedDelegateHandle() = default;

	/// Takes ownership of the provided delegate handle
	TScopedDelegateHandle(DelegateType& InDelegate, FDelegateHandle&& InDelegateHandle)
		: Delegate{&InDelegate}, DelegateHandle{InDelegateHandle}
	{
		InDelegateHandle.Reset();
	}

	TScopedDelegateHandle(const TScopedDelegateHandle& InDelegateHandle) = delete;

	TScopedDelegateHandle(TScopedDelegateHandle&& InDelegateHandle) noexcept
		: Delegate{InDelegateHandle.Delegate}, DelegateHandle{InDelegateHandle.Release()}
	{
	}

	~TScopedDelegateHandle()
	{
		Reset();
	}

	/// Copy-and-swap idiom makes any assignment work correctly
	TScopedDelegateHandle& operator=(TScopedDelegateHandle Other)
	{
		Swap(Other);
		return *this;
	}

	/// Releases ownership of the held delegate handle
	FDelegateHandle Release()
	{
		FDelegateHandle Copy = DelegateHandle;
		Delegate = nullptr;
		DelegateHandle.Reset();
		return Copy;
	}

	/// Clears this handle and unbinds the held handle from the delegate
	void Reset()
	{
		if (Delegate && DelegateHandle.IsValid())
		{
			if constexpr (Private::THasRemoveDelegateHandleMemberFunctionV<DelegateType>)
			{
				Delegate->Remove(DelegateHandle);
			}
			else
			{
				Delegate->Unbind();
			}

			Delegate = nullptr;
			DelegateHandle.Reset();
		}
	}

	bool IsValid() const
	{
		return DelegateHandle.IsValid();
	}

	void Swap(TScopedDelegateHandle& Other)
	{
		using std::swap;
		swap(Delegate, Other.Delegate);
		swap(DelegateHandle, Other.DelegateHandle);
	}

private:
	DelegateType* Delegate = nullptr;
	FDelegateHandle DelegateHandle;
};

}  // namespace Zkz
