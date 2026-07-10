// Copyright ZAKAZANE Studio. All Rights Reserved.

#pragma once

#if defined(__JETBRAINS_IDE__)

/// Type should or can be passed by value without performance issues. Mark types with this attribute to avoid
/// inspection suggesting to pass argument by const reference.
#define ZKZ_PASS_BY_VALUE [[jetbrains::pass_by_value]]

/// Type is a scope guard. Mark types with this attribute to avoid inspections suggesting that a variable is unused.
#define ZKZ_GUARD [[jetbrains::guard]]

#else

#define ZKZ_PASS_BY_VALUE
#define ZKZ_GUARD

#endif
