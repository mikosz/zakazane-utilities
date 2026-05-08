// Copyright ZAKAZANE Studio. All Rights Reserved.

#include "Zakazane/Budgeting/ZkzTickableBudget.h"

DEFINE_LOG_CATEGORY_CLASS(UZkzTickableBudget, LogCategory);

void UZkzTickableBudget::PostInitProperties()
{
	UObject::PostInitProperties();

	Budget.SetPerFrameBudget(PerFrameBudget);
}

void UZkzTickableBudget::EnqueueTask(TaskType InTask)
{
	Budget.EnqueueTask(MoveTemp(InTask));
}

void UZkzTickableBudget::Tick(float DeltaTime)
{
	Budget.Tick();
}

bool UZkzTickableBudget::IsTickable() const
{
	return !IsTemplate();
}

TStatId UZkzTickableBudget::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UZkzTickableBudget, STATGROUP_Tickables);
}

UWorld* UZkzTickableBudget::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

bool UZkzTickableBudget::IsTickableWhenPaused() const
{
	return true;
}
