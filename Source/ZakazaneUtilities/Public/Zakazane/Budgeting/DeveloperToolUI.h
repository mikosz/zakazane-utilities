// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Budget.h"

namespace Zkz::Budgeting
{

/// Generic implementation of a developer tool monitoring TBudget instances.
/// You need to implement a developer toolkit tool using this template for your scheduler.
/// This tool does not show the developer tool window to allow including it in other tools.
template <CBudgetType BudgetType>
class TDeveloperToolUI
{
public:
	void Tick(const BudgetType& Budget, float DeltaTime, bool bShow);
};

}  // namespace Zkz::Budgeting

#include "Private/DeveloperToolUI.tpp"
