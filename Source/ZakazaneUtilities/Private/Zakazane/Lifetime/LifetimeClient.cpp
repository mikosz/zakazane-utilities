// Copyright Mikolaj Radwan. All Rights Reserved.

#include "Zakazane/Lifetime/LifetimeClient.h"

namespace Zkz::Lifetime
{

bool FLifetimeClient::IsValid() const
{
	return ClientName != NAME_None && !SubscribedLifetimes.IsEmpty() && Delegate.IsBound();
}

}  // namespace Zkz::Lifetime

FZkzLifetimeClientHandle::FZkzLifetimeClientHandle(FName InClientId) : ClientId{MoveTemp(InClientId)}
{
}

void FZkzLifetimeClientHandle::Reset()
{
	ClientId = NAME_None;
}

bool FZkzLifetimeClientHandle::IsValid() const
{
	return ClientId != NAME_None;
}