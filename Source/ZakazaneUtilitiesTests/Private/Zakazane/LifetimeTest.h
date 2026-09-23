// Copyright Mikolaj Radwan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "UObject/Object.h"
#include "Zakazane/Lifetime/ZkzLifetimeProvider.h"

#include "LifetimeTest.generated.h"

UCLASS()
class ZAKAZANEUTILITIESTESTS_API UTestLifetimeProvider : public UObject, public IZkzLifetimeProvider
{
	GENERATED_BODY()

public:
	void StartLifetime(FName LifetimeId);
	void EndLifetime(FName LifetimeId);

protected:
	virtual Zkz::Lifetime::FProviderData& GetLifetimeProviderData() override;

private:
	Zkz::Lifetime::FProviderData LifetimeProviderData{TEXT("Test Lifetime Provider"), {"lifetime", "a", "b"}};
};
