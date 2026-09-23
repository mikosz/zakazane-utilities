// Copyright Mikolaj Radwan. All Rights Reserved.

#include "LifetimeTest.h"

#include "Zakazane/Lifetime/LifetimeClient.h"
#include "Zakazane/ReturnIfMacros.h"
#include "Zakazane/Test/Test.h"

void UTestLifetimeProvider::StartLifetime(const FName LifetimeId)
{
	LifetimeProviderData.NotifyLifetimeStarted(LifetimeId);
}

void UTestLifetimeProvider::EndLifetime(const FName LifetimeId)
{
	LifetimeProviderData.NotifyLifetimeEnded(LifetimeId);
}

Zkz::Lifetime::FProviderData& UTestLifetimeProvider::GetLifetimeProviderData()
{
	return LifetimeProviderData;
}

namespace Zkz::Lifetime::Tests
{

ZKZ_BEGIN_AUTOMATION_TEST(
	FLifetimeProviderTest,
	"Zakazane.ZakazaneUtilities.Lifetime.LifetimeProvider",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(NotifiesAboutActiveLifetimes)
{
	auto* const LifetimeProvider = NewObject<UTestLifetimeProvider>();
	ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(LifetimeProvider);

	auto StartCalls = 0;
	auto EndCalls = 0;

	LifetimeProvider->StartLifetime("started_before_delegate");
	std::ignore = LifetimeProvider->RegisterClient(
		FLifetimeClient{
			.ClientName = TEXT("TEST"),
			.SubscribedLifetimes = {"started_before_delegate"},
			.Delegate = FLifetimeClient::FDelegate::CreateLambda(
				[&StartCalls, &EndCalls](const bool bActive)
				{
					if (bActive)
					{
						++StartCalls;
					}
					else
					{
						++EndCalls;
					}
				})});

	TestEqual("Start triggered for active lifetime", StartCalls, 1);
	TestEqual("End not triggered for active lifetime", EndCalls, 0);
}

ZKZ_ADD_TEST(NotifiesAboutLifetimeStartsAndEnds)
{
	auto* const LifetimeProvider = NewObject<UTestLifetimeProvider>();
	ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(LifetimeProvider);

	auto StartCalls = 0;
	auto EndCalls = 0;

	std::ignore = LifetimeProvider->RegisterClient(
		FLifetimeClient{
			.ClientName = TEXT("TEST"),
			.SubscribedLifetimes = {"lifetime"},
			.Delegate = FLifetimeClient::FDelegate::CreateLambda(
				[&StartCalls, &EndCalls](const bool bActive)
				{
					if (bActive)
					{
						++StartCalls;
					}
					else
					{
						++EndCalls;
					}
				})});

	TestEqual("Start not triggered for inactive lifetime", StartCalls, 0);
	TestEqual("End not triggered for inactive lifetime", EndCalls, 0);

	LifetimeProvider->StartLifetime("lifetime");

	TestEqual("Start triggered for active lifetime", StartCalls, 1);
	TestEqual("End not triggered for active lifetime", EndCalls, 0);

	LifetimeProvider->EndLifetime("lifetime");

	TestEqual("Start not triggered for deactivated lifetime", StartCalls, 1);
	TestEqual("End triggered for deactivated lifetime", EndCalls, 1);
}

ZKZ_ADD_TEST(NotifiesAboutCombinedLifetimeStartsAndEnds)
{
	auto* const LifetimeProvider = NewObject<UTestLifetimeProvider>();
	ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(LifetimeProvider);

	auto StartCalls = 0;
	auto EndCalls = 0;

	std::ignore = LifetimeProvider->RegisterClient(
		FLifetimeClient{
			.ClientName = TEXT("TEST"),
			.SubscribedLifetimes = {"a", "b"},
			.Delegate = FLifetimeClient::FDelegate::CreateLambda(
				[&StartCalls, &EndCalls](const bool bActive)
				{
					if (bActive)
					{
						++StartCalls;
					}
					else
					{
						++EndCalls;
					}
				})});

	TestEqual("Start not triggered for inactive lifetimes", StartCalls, 0);
	TestEqual("End not triggered for inactive lifetimes", EndCalls, 0);

	LifetimeProvider->StartLifetime("a");

	TestEqual("Start not triggered for single active lifetime", StartCalls, 0);
	TestEqual("End not triggered for single active lifetime", EndCalls, 0);

	LifetimeProvider->StartLifetime("b");

	TestEqual("Start triggered for both active lifetimes", StartCalls, 1);
	TestEqual("End not triggered for both active lifetimes", EndCalls, 0);

	LifetimeProvider->EndLifetime("b");

	TestEqual("Start not triggered for single deactivated lifetime", StartCalls, 1);
	TestEqual("End triggered for single deactivated lifetime", EndCalls, 1);

	LifetimeProvider->EndLifetime("a");

	TestEqual("Start not triggered for second deactivated lifetime", StartCalls, 1);
	TestEqual("End not triggered for second deactivated lifetime", EndCalls, 1);
}

ZKZ_END_AUTOMATION_TEST(FLifetimeProviderTest);

}  // namespace Zkz::Lifetime::Tests
