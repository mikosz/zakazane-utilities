// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "Zakazane/Pointer.h"

/// Macro simplifying adding verification code to functions. Automatically continues the current loop if expression is true.
/// E.g.:
///
/// - ZKZ_CONTINUE_IF(!SomeSharedPtr.IsValid());
///
/// expands to:
///
/// @code
/// if (!SomeSharedPtr.IsValid())
/// {
///     continue;
/// }
/// @endcode
#define ZKZ_CONTINUE_IF(Expression) \
	if (Expression)                 \
	{                               \
		continue;                   \
	}

/// Same as ZKZ_CONTINUE_IF but also ensure-s the expression is false.
#define ZKZ_CONTINUE_IF_ENSURE(Expression) \
	if (!ensure(!(Expression)))            \
	{                                      \
		continue;                          \
	}

/// Same as ZKZ_CONTINUE_IF but also ensureMsgf-s the expression is false.
/// Message format arguments are currently not supported.
#define ZKZ_CONTINUE_IF_ENSUREMSGF(Expression, Msg) \
	if (!ensureMsgf(!(Expression), TEXT(#Msg)))     \
	{                                               \
		continue;                                   \
	}

/// Same as ZKZ_CONTINUE_IF but also ensureAlways-s the expression is false.
#define ZKZ_CONTINUE_IF_ENSUREALWAYS(Expression) \
	if (!ensureAlways(!(Expression)))            \
	{                                            \
		continue;                                \
	}

/// Same as ZKZ_CONTINUE_IF but also ensureAlwaysMsgf-s the expression is false.
/// Message format arguments are currently not supported.
#define ZKZ_CONTINUE_IF_ENSUREALWAYSMSGF(Expression, Msg) \
	if (!ensureAlwaysMsgf(!(Expression), TEXT(#Msg)))     \
	{                                                     \
		continue;                                         \
	}

/// Macro simplifying adding IsValid checks to functions. Automatically continues the current loop if pointer is not valid.
/// E.g.:
///
/// - CONTINUE_IF_INVALID(Pointer);
///
/// expands to:
///
/// @code
/// if (!IsValid(Pointer))
/// {
///     continue;
/// }
/// @endcode
#define ZKZ_CONTINUE_IF_INVALID(UObjectPtr) \
	if (!Zkz::Pointer::IsValid(UObjectPtr))             \
	{                                       \
		continue;                           \
	}

/// Same as ZKZ_CONTINUE_IF_INVALID but also ensure-s pointer is valid.
#define ZKZ_CONTINUE_IF_INVALID_ENSURE(UObjectPtr) \
	if (!ensure(Zkz::Pointer::IsValid(UObjectPtr)))            \
	{                                              \
		continue;                                  \
	}

/// Same as ZKZ_CONTINUE_IF_INVALID but also ensureMsgf-s pointer is valid.
/// Message format arguments are currently not supported.
#define ZKZ_CONTINUE_IF_INVALID_ENSUREMSGF(UObjectPtr, Msg) \
	if (!ensureMsgf(Zkz::Pointer::IsValid(UObjectPtr), TEXT(#Msg)))     \
	{                                                       \
		continue;                                           \
	}

/// Same as ZKZ_CONTINUE_IF_INVALID but also ensureAlways-s pointer is valid.
#define ZKZ_CONTINUE_IF_INVALID_ENSUREALWAYS(UObjectPtr) \
	if (!ensureAlways(Zkz::Pointer::IsValid(UObjectPtr)))            \
	{                                                    \
		continue;                                        \
	}

/// Same as ZKZ_CONTINUE_IF_INVALID but also ensureAlwaysMsgf-s pointer is valid.
/// Message format arguments are currently not supported.
#define ZKZ_CONTINUE_IF_INVALID_ENSUREALWAYSMSGF(UObjectPtr, Msg) \
	if (!ensureAlwaysMsgf(Zkz::Pointer::IsValid(UObjectPtr), TEXT(#Msg)))     \
	{                                                             \
		continue;                                                 \
	}
