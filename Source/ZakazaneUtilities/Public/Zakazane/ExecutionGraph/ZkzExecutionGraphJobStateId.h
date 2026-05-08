// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "ZkzExecutionGraphJobStateId.generated.h"

UENUM()
enum class EZkzExecutionGraphJobStateId
{
	/// Transient state used prior to initialization
	Default,

	/// Stub state created for undefined jobs referenced by other jobs (as predecessors)
	Stub,

	/// Stub state created for undefined stages referenced by other jobs (as parents)
	StageStub,

	/// Represents a task with known predecessors
	DefinedTask,

	/// Represents a stage with known predecessors
	DefinedStage,

	/// Represents a stage that is executing (tasks without predecessors are immediately executed,
	/// others are executed as their predecessors complete)
	ExecutingStage,

	/// Represents a task that is executing
	ExecutingTask,

	/// State for completed jobs
	Completed,
};
