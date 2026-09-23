// Copyright Mikolaj Radwan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "LifetimeClient.h"
#include "UObject/Interface.h"

#include "ZkzLifetimeProvider.generated.h"

namespace Zkz::Lifetime
{

class ZAKAZANEUTILITIES_API FProviderData
{
public:
	explicit FProviderData(FString InLogName, TConstArrayView<FName> InBuiltinLifetimes);

	void NotifyLifetimeStarted(FName LifetimeId);
	void NotifyLifetimeEnded(FName LifetimeId);

	void RegisterBuiltinLifetime(FName LifetimeId);

	FZkzLifetimeClientHandle RegisterClient(FLifetimeClient Client);
	void UnregisterClient(FZkzLifetimeClientHandle& Handle);

	void RegisterClientLifetime(FName LifetimeId);
	void UnregisterClientLifetime(FName LifetimeId);
	void NotifyClientLifetimeStarted(FName LifetimeId, bool bCreate);
	void NotifyClientLifetimeEnded(FName LifetimeId);

	bool IsSupportedLifetime(FName LifetimeId) const;
	bool IsLifetimeActive(FName LifetimeId) const;

private:
	static constexpr int32 InlineLifetimes = 4;

	using FLifetimes = TArray<FName, TInlineAllocator<InlineLifetimes>>;

#if !NO_LOGGING
	FString LogName;
#endif

	FLifetimes BuiltinLifetimes;

	FLifetimes ClientLifetimes;

	FLifetimes ActiveLifetimes;

	TMap<FName, FLifetimeClient> ClientsByName;

	bool AllActive(TConstArrayView<FName> LifetimeIds, FName DontCheckId = NAME_None) const;
	void NotifyClient(FName ClientName, bool bActive) const;
	FName MakeUniqueClientName(const FName InBaseName) const;

	DECLARE_LOG_CATEGORY_CLASS(Log, Verbose, All);	// #TODO_dontcommit: probably Log not verbose
};

// #TODO_dontcommit: needs a way to poll whether a given lifetime is supported
// #TODO_dontcommit: needs tests verifying lifetime behaves correctly after unregister / destruction of underlying objects etc:
//   i.e., doesn't crash, cleans up clients in RemoveInvalidClients.

}  // namespace Zkz::Lifetime

UINTERFACE()
class UZkzLifetimeProvider : public UInterface
{
	GENERATED_BODY()
};

/// Implemented by classes providing lifetime callbacks for some objects / states / facts. The elements whose
/// lifetimes are tracked are identified by an FName and can therefore be of any abstract type. Clients can
/// register to be notified about the start and end of these lifetimes.
class ZAKAZANEUTILITIES_API IZkzLifetimeProvider
{
	GENERATED_BODY()

public:
	FZkzLifetimeClientHandle RegisterClient(Zkz::Lifetime::FLifetimeClient Client);
	void UnregisterClient(FZkzLifetimeClientHandle& Handle);

	void RegisterClientLifetime(FName LifetimeId);
	void UnregisterClientLifetime(FName LifetimeId);
	void NotifyClientLifetimeStarted(FName LifetimeId, bool bCreate = false);
	void NotifyClientLifetimeEnded(FName LifetimeId);

	bool IsLifetimeActive(FName LifetimeId) const;

protected:
	virtual Zkz::Lifetime::FProviderData& GetLifetimeProviderData() = 0;
	const Zkz::Lifetime::FProviderData& GetLifetimeProviderData() const;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FZkzBPLifetimeClientDelegate, bool, bActive);

UCLASS(MinimalAPI)
class UZkzLifetimeProviderBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Zakazane|Lifetime", meta = (DefaultToSelf = "LifetimeProvider"))
	static FZkzLifetimeClientHandle RegisterLifetimeClient(
		TScriptInterface<IZkzLifetimeProvider> LifetimeProvider,
		FName Name,
		TArray<FName> LifetimeIds,
		FZkzBPLifetimeClientDelegate Delegate);

	UFUNCTION(BlueprintCallable, Category = "Zakazane|Lifetime", meta = (DefaultToSelf = "LifetimeProvider"))
	static void UnregisterLifetimeClient(
		TScriptInterface<IZkzLifetimeProvider> LifetimeProvider, UPARAM(ref) FZkzLifetimeClientHandle& Handle);
};
