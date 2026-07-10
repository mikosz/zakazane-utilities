// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Budget.h"

#include "ZkzTickableBudget.generated.h"

/// UObject wrapper for TBudget. Automatically calls "Tick" every frame, but doesn't allow
/// customization of the log category or changing the used inspections type.
UCLASS()
class ZAKAZANEUTILITIES_API UZkzTickableBudget final : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	ZKZ_DECLARE_LOG_CATEGORY_CLASS(LogCategory, Display, All);

	using BudgetType = decltype(Zkz::Budgeting::MakeBudget(FTimespan{}, LogCategory));
	using TaskType = BudgetType::TaskType;

	UPROPERTY(EditAnywhere)
	FTimespan PerFrameBudget;

	// ~ UObject
	virtual void PostInitProperties() override;
	// ~ UObject

	void EnqueueTask(TaskType InTask);

private:
	BudgetType Budget = Zkz::Budgeting::MakeBudget(FTimespan{}, LogCategory);

	// ~ FTickableGameObject
	// ReSharper disable CppOverrideWithDifferentVisibility
	virtual void Tick(float DeltaTime) override;

	virtual bool IsTickable() const override;

	virtual TStatId GetStatId() const override;

	virtual UWorld* GetTickableGameObjectWorld() const override;

	virtual bool IsTickableWhenPaused() const override;
	// ReSharper restore CppOverrideWithDifferentVisibility
	// ~ FTickableGameObject
};
