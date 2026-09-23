// Copyright Mikolaj Radwan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "LifetimeClient.generated.h"

DECLARE_DELEGATE_OneParam(FZkzLifetimeClientDelegate, bool /* bActive */);

namespace Zkz::Lifetime
{

class FProviderData;

struct FLifetimeClient
{
	using FDelegate = FZkzLifetimeClientDelegate;

	static constexpr int32 InlineLifetimes = 4;
	using FSubscribedLifetimes = TArray<FName, TInlineAllocator<InlineLifetimes>>;

	FName ClientName = NAME_None;

	FSubscribedLifetimes SubscribedLifetimes;

	FDelegate Delegate;

	bool IsValid() const;
};

}  // namespace Zkz::Lifetime

USTRUCT(BlueprintType)
struct [[nodiscard]] ZAKAZANEUTILITIES_API FZkzLifetimeClientHandle
{
	GENERATED_BODY()

public:
	FZkzLifetimeClientHandle() = default;
	explicit FZkzLifetimeClientHandle(FName InClientId);

	void Reset();

	bool IsValid() const;

private:
	UPROPERTY()
	FName ClientId = NAME_None;

	friend class Zkz::Lifetime::FProviderData;
};
