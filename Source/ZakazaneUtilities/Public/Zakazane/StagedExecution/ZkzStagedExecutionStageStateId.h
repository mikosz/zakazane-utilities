// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "ZkzStagedExecutionStageStateId.generated.h"

UENUM()
enum class EZkzStagedExecutionStageStateId
{
	Unknown,
	Undefined,
	Defined,
	Executing,
	Completed,
};
