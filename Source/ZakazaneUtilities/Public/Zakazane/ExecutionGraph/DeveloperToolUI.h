// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Scheduler.h"

namespace Zkz::ExecutionGraph
{

/// Generic implementation of a developer tool monitoring TScheduler instances.
/// You need to implement a developer toolkit tool using this template for your scheduler.
/// This tool does not show the developer tool window to allow including it in other tools.
template <CSchedulerType SchedulerType>
class TDeveloperToolUI
{
public:
	void Tick(SchedulerType& Scheduler, float DeltaTime, bool bShow);
};

}  // namespace Zkz::ExecutionGraph

#include "Private/DeveloperToolUI.tpp"
