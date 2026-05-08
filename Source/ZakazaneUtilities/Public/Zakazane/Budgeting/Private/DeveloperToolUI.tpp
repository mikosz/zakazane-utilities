// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "Zakazane/Budgeting/DeveloperToolUI.h"

namespace Zkz::Budgeting
{

template <CBudgetType BudgetType>
void TDeveloperToolUI<BudgetType>::Tick(const BudgetType& Budget, const float DeltaTime, const bool bShow)
{
	if constexpr (!BudgetType::WithInspections)
	{
		return;
	}

	ZKZ_RETURN_IF(!bShow);

	const FDebugData* const DebugData = Budget.GetDebugData();
	ZKZ_RETURN_IF(DebugData == nullptr);

	// #TODO #Budgeting: make nice plots for budget inspections when we add some budgets to the game
}

}  // namespace Zkz::Budgeting
