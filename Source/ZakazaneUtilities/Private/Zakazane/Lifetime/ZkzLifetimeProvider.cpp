// Copyright Mikolaj Radwan. All Rights Reserved.

#include "Zakazane/Lifetime/ZkzLifetimeProvider.h"

#include "Algo/AllOf.h"
#include "Zakazane/Algo.h"
#include "Zakazane/ReturnIfMacros.h"

namespace Zkz::Lifetime
{

FProviderData::FProviderData(FString InLogName, TConstArrayView<FName> InBuiltinLifetimes)
	: LogName{MoveTemp(InLogName)}, BuiltinLifetimes{MoveTemp(InBuiltinLifetimes)}
{
}

void FProviderData::NotifyLifetimeStarted(FName LifetimeId)
{
	ZKZ_RETURN_IF_ENSUREALWAYS(!IsSupportedLifetime(LifetimeId));
	ZKZ_RETURN_IF(IsLifetimeActive(LifetimeId));

	ActiveLifetimes.Emplace(LifetimeId);

	static constexpr auto MaxExpectedClients = 4;

	// Delaying notify for situations where delegate unregisters the client
	const auto ClientsToNotify = TransformIf<TInlineAllocator<MaxExpectedClients>>(
		ClientsByName,
		[this, LifetimeId](const auto& Entry)
		{
			const auto& [Handle, Client] = Entry;

			return Client.SubscribedLifetimes.Contains(LifetimeId)
				   && (Client.SubscribedLifetimes.Num() == 1 || AllActive(Client.SubscribedLifetimes));
		},
		[](const auto& Entry)
		{
			const auto& [Handle, Client] = Entry;

			return Handle;
		});

	UE_LOG(
		Log,
		Log,
		TEXT("[%s] Lifetime %s started, notifying %d of %d clients"),
		*LogName,
		*LifetimeId.ToString(),
		ClientsToNotify.Num(),
		ClientsByName.Num());

	for (auto& ClientToNotify : ClientsToNotify)
	{
		NotifyClient(ClientToNotify, true);
	}
}

void FProviderData::NotifyLifetimeEnded(FName LifetimeId)
{
	ZKZ_RETURN_IF_ENSUREALWAYS(!IsSupportedLifetime(LifetimeId));
	ZKZ_RETURN_IF(!IsLifetimeActive(LifetimeId));

	const auto NumRemoved = ActiveLifetimes.RemoveSingleSwap(LifetimeId);

	ZKZ_RETURN_IF_ENSUREALWAYS(NumRemoved == 0);

	static constexpr auto MaxExpectedClients = 4;

	// Delaying notify for situations where delegate unregisters the client
	const auto ClientsToNotify = TransformIf<TInlineAllocator<MaxExpectedClients>>(
		ClientsByName,
		[this, LifetimeId](const auto& Entry)
		{
			const auto& [Handle, Client] = Entry;

			return Client.SubscribedLifetimes.Contains(LifetimeId)
				   && (Client.SubscribedLifetimes.Num() == 1 || AllActive(Client.SubscribedLifetimes, LifetimeId));
		},
		[](const auto& Entry)
		{
			const auto& [Handle, Client] = Entry;

			return Handle;
		});

	UE_LOG(
		Log,
		Log,
		TEXT("[%s] Lifetime %s ended, notifying %d of %d clients"),
		*LogName,
		*LifetimeId.ToString(),
		ClientsToNotify.Num(),
		ClientsByName.Num());

	for (auto& ClientToNotify : ClientsToNotify)
	{
		NotifyClient(ClientToNotify, false);
	}
}

void FProviderData::RegisterBuiltinLifetime(const FName LifetimeId)
{
	ZKZ_RETURN_IF_ENSUREALWAYS(IsSupportedLifetime(LifetimeId));

	BuiltinLifetimes.Emplace(LifetimeId);
}

FZkzLifetimeClientHandle FProviderData::RegisterClient(FLifetimeClient Client)
{
	ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(Client, {});

	const auto ClientName = MakeUniqueClientName(Client.ClientName);
	auto& Entry = ClientsByName.Emplace(ClientName, MoveTemp(Client));

	if (AllActive(Entry.SubscribedLifetimes))
	{
		UE_LOG(
			Log,
			Log,
			TEXT("[%s] Registered client for active lifetimes {%s}, notifying immediately"),
			*LogName,
			*FString::JoinBy(Entry.SubscribedLifetimes, TEXT(", "), [](const FName Name) { return Name.ToString(); }));

		NotifyClient(ClientName, true);
	}

	return FZkzLifetimeClientHandle{ClientName};
}

void FProviderData::UnregisterClient(FZkzLifetimeClientHandle& Handle)
{
	ZKZ_RETURN_IF_INVALID(Handle);

	ClientsByName.Remove(Handle.ClientId);
	Handle.Reset();
}

void FProviderData::RegisterClientLifetime(const FName LifetimeId)
{
	ZKZ_RETURN_IF_ENSUREALWAYS(IsSupportedLifetime(LifetimeId));

	ClientLifetimes.Emplace(LifetimeId);
}

void FProviderData::UnregisterClientLifetime(const FName LifetimeId)
{
	const auto NumRemoved = ClientLifetimes.Remove(LifetimeId);
	ensureAlways(NumRemoved == 1);
}

void FProviderData::NotifyClientLifetimeStarted(const FName LifetimeId, const bool bCreate)
{
	if (bCreate)
	{
		RegisterClientLifetime(LifetimeId);
	}

	NotifyLifetimeStarted(LifetimeId);
}

void FProviderData::NotifyClientLifetimeEnded(const FName LifetimeId)
{
	ZKZ_RETURN_IF_ENSUREALWAYS(!IsSupportedLifetime(LifetimeId));

	NotifyLifetimeEnded(LifetimeId);
}

bool FProviderData::IsSupportedLifetime(const FName LifetimeId) const
{
	return BuiltinLifetimes.Contains(LifetimeId) || ClientLifetimes.Contains(LifetimeId);
}

bool FProviderData::IsLifetimeActive(const FName LifetimeId) const
{
	return ActiveLifetimes.Contains(LifetimeId);
}

bool FProviderData::AllActive(const TConstArrayView<FName> LifetimeIds, const FName DontCheckId) const
{
	return Algo::AllOf(
		LifetimeIds,
		[this, DontCheckId](const FName LifetimeId)
		{ return IsLifetimeActive(LifetimeId) || LifetimeId == DontCheckId; });
}

void FProviderData::NotifyClient(const FName ClientName, const bool bActive) const
{
	const auto* const Client = ClientsByName.Find(ClientName);
	ZKZ_RETURN_IF_INVALID(Client);

	UE_LOG(
		Log,
		Verbose,
		TEXT("\tNotifying %s registered for lifetimes {%s}"),
		*ClientName.ToString(),
		*FString::JoinBy(Client->SubscribedLifetimes, TEXT(", "), [](const FName Name) { return Name.ToString(); }));

	Client->Delegate.Execute(bActive);
}

FName FProviderData::MakeUniqueClientName(const FName InBaseName) const
{
	auto Result = InBaseName;

	if (Result.GetNumber() == NAME_NO_NUMBER_INTERNAL)
	{
		Result.SetNumber(0);
	}

	while (ClientsByName.Contains(Result))
	{
		Result.SetNumber(Result.GetNumber() + 1);
	}

	return Result;
}

DEFINE_LOG_CATEGORY_CLASS(FProviderData, Log);

}  // namespace Zkz::Lifetime

FZkzLifetimeClientHandle IZkzLifetimeProvider::RegisterClient(Zkz::Lifetime::FLifetimeClient Client)
{
	return GetLifetimeProviderData().RegisterClient(MoveTemp(Client));
}

void IZkzLifetimeProvider::UnregisterClient(FZkzLifetimeClientHandle& Handle)
{
	return GetLifetimeProviderData().UnregisterClient(Handle);
}

void IZkzLifetimeProvider::RegisterClientLifetime(FName LifetimeId)
{
	return GetLifetimeProviderData().RegisterClientLifetime(MoveTemp(LifetimeId));
}

void IZkzLifetimeProvider::UnregisterClientLifetime(FName LifetimeId)
{
	return GetLifetimeProviderData().UnregisterClientLifetime(MoveTemp(LifetimeId));
}

void IZkzLifetimeProvider::NotifyClientLifetimeStarted(FName LifetimeId, const bool bCreate)
{
	return GetLifetimeProviderData().NotifyClientLifetimeStarted(MoveTemp(LifetimeId), bCreate);
}

void IZkzLifetimeProvider::NotifyClientLifetimeEnded(FName LifetimeId)
{
	return GetLifetimeProviderData().NotifyClientLifetimeEnded(MoveTemp(LifetimeId));
}

bool IZkzLifetimeProvider::IsLifetimeActive(FName LifetimeId) const
{
	return GetLifetimeProviderData().IsLifetimeActive(MoveTemp(LifetimeId));
}

const Zkz::Lifetime::FProviderData& IZkzLifetimeProvider::GetLifetimeProviderData() const
{
	return const_cast<IZkzLifetimeProvider&>(*this).GetLifetimeProviderData();
}

FZkzLifetimeClientHandle UZkzLifetimeProviderBlueprintFunctionLibrary::RegisterLifetimeClient(
	const TScriptInterface<IZkzLifetimeProvider> LifetimeProvider,
	FName Name,
	TArray<FName> LifetimeIds,
	FZkzBPLifetimeClientDelegate Delegate)
{
	using namespace Zkz::Lifetime;

	auto* const LifetimeProviderPtr = LifetimeProvider.GetInterface();
	ZKZ_RETURN_IF_INVALID(LifetimeProviderPtr, {});

	return LifetimeProviderPtr->RegisterClient(
		FLifetimeClient{
			MoveTemp(Name),
			FLifetimeClient::FSubscribedLifetimes{MoveTemp(LifetimeIds)},
			FZkzLifetimeClientDelegate::CreateLambda([Delegate = MoveTemp(Delegate)](const bool bActive)
													 { Delegate.ExecuteIfBound(bActive); })});
}

void UZkzLifetimeProviderBlueprintFunctionLibrary::UnregisterLifetimeClient(
	const TScriptInterface<IZkzLifetimeProvider> LifetimeProvider, FZkzLifetimeClientHandle& Handle)
{
	auto* const LifetimeProviderPtr = LifetimeProvider.GetInterface();
	ZKZ_RETURN_IF_INVALID(LifetimeProviderPtr);

	LifetimeProviderPtr->UnregisterClient(Handle);
}
