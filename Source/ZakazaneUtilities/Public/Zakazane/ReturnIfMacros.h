// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#include "Zakazane/Pointer.h"

/**
 * Macro simplifying adding verification code to functions. Automatically returns from function if expression is true.
 * Optional second parameter is the return value.
 * E.g.:
 *
 * - ZKZ_RETURN_IF(!SomeSharedPtr.IsValid());
 *
 * expands to:
 * 
 * if (!SomeSharedPtr.IsValid())
 * {
 *     return;
 * }
 * 
 * - ZKZ_RETURN_IF(!SomeSharedPtr.IsValid(), false);
 *
 * expands to:
 * 
 * if (!SomeSharedPtr.IsValid())
 * {
 *     return false;
 * }
 */
#define ZKZ_RETURN_IF(Expression, ...) \
	do                                 \
	{                                  \
		if (Expression)                \
		{                              \
			return __VA_ARGS__;        \
		}                              \
	} while (false)

/**
 * Same as ZKZ_RETURN_IF but also ensure-s the expression is false.
 */
#define ZKZ_RETURN_IF_ENSURE(Expression, ...) \
	do                                        \
	{                                         \
		if (!ensure(!(Expression)))           \
		{                                     \
			return __VA_ARGS__;               \
		}                                     \
	} while (false)

/**
 * Same as ZKZ_RETURN_IF but also ensureMsgf-s the expression is false.
 * Message format arguments are currently not supported.
 */
#define ZKZ_RETURN_IF_ENSUREMSGF(Expression, Msg, ...) \
	do                                                 \
	{                                                  \
		if (!ensureMsgf(!(Expression), TEXT(#Msg)))    \
		{                                              \
			return __VA_ARGS__;                        \
		}                                              \
	} while (false)

/**
 * Same as ZKZ_RETURN_IF but also ensureAlways-s the expression is false.
 */
#define ZKZ_RETURN_IF_ENSUREALWAYS(Expression, ...) \
	do                                              \
	{                                               \
		if (!ensureAlways(!(Expression)))           \
		{                                           \
			return __VA_ARGS__;                     \
		}                                           \
	} while (false)

/**
 * Same as ZKZ_RETURN_IF but also ensureAlwaysMsgf-s the expression is false.
 * Message format arguments are currently not supported.
 */
#define ZKZ_RETURN_IF_ENSUREALWAYSMSGF(Expression, Msg, ...) \
	do                                                       \
	{                                                        \
		if (!ensureAlwaysMsgf(!(Expression), TEXT(#Msg)))    \
		{                                                    \
			return __VA_ARGS__;                              \
		}                                                    \
	} while (false)

/**
 * Macro simplifying adding IsValid checks to functions. Automatically returns from function if pointer is not valid.
 * Optional second parameter is the return value.
 * E.g.:
 *
* - RETURN_IF_INVALID(Pointer);
 *
 * expands to:
 * 
 * if (!Zkz::Pointer::IsValid(Pointer))
 * {
 *     return;
 * }

* - RETURN_IF_INVALID(Pointer, false);
 *
 * expands to:
 * 
 * if (!Zkz::Pointer::IsValid(Pointer))
 * {
 *     return false;
 * }
 */
#define ZKZ_RETURN_IF_INVALID(Ptr, ...)  \
	do                                   \
	{                                    \
		if (!Zkz::Pointer::IsValid(Ptr)) \
		{                                \
			return __VA_ARGS__;          \
		}                                \
	} while (false)

/**
 * Same as ZKZ_RETURN_IF_INVALID but also ensure-s pointer is valid.
 */
#define ZKZ_RETURN_IF_INVALID_ENSURE(Ptr, ...)   \
	do                                           \
	{                                            \
		if (!ensure(Zkz::Pointer::IsValid(Ptr))) \
		{                                        \
			return __VA_ARGS__;                  \
		}                                        \
	} while (false)

/**
 * Same as ZKZ_RETURN_IF_INVALID but also ensureMsgf-s pointer is valid.
 * Message format arguments are currently not supported.
 */
#define ZKZ_RETURN_IF_INVALID_ENSUREMSGF(Ptr, Msg, ...)          \
	do                                                           \
	{                                                            \
		if (!ensureMsgf(Zkz::Pointer::IsValid(Ptr), TEXT(#Msg))) \
		{                                                        \
			return __VA_ARGS__;                                  \
		}                                                        \
	} while (false)

/**
 * Same as ZKZ_RETURN_IF_INVALID but also ensureAlways-s pointer is valid.
 */
#define ZKZ_RETURN_IF_INVALID_ENSUREALWAYS(Ptr, ...)   \
	do                                                 \
	{                                                  \
		if (!ensureAlways(Zkz::Pointer::IsValid(Ptr))) \
		{                                              \
			return __VA_ARGS__;                        \
		}                                              \
	} while (false)

/**
 * Same as ZKZ_RETURN_IF_INVALID but also ensureAlwaysMsgf-s pointer is valid.
 * Message format arguments are currently not supported.
 */
#define ZKZ_RETURN_IF_INVALID_ENSUREALWAYSMSGF(Ptr, Msg, ...)          \
	do                                                                 \
	{                                                                  \
		if (!ensureAlwaysMsgf(Zkz::Pointer::IsValid(Ptr), TEXT(#Msg))) \
		{                                                              \
			return __VA_ARGS__;                                        \
		}                                                              \
	} while (false)