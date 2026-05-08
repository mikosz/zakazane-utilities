#include "Zakazane/ExecutionGraph/SynchronizationTraits.h"

namespace Zkz::ExecutionGraph
{

FThreadSafe::LockType FThreadSafe::Lock(MutexType& Mutex)
{
	return TScopedLock<MutexType>{Mutex};
}

bool FThreadSafe::IsLocked(const LockType& Lock)
{
	return !!Lock;
}

}  // namespace Zkz::ExecutionGraph
