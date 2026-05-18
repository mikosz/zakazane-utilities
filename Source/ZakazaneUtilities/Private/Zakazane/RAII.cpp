#include "Zakazane/RAII.h"

namespace Zkz
{

FScopedExecution::FScopedExecution(TFunction<void()> InFunc) : Func{MoveTemp(InFunc)}
{
}

FScopedExecution::~FScopedExecution()
{
	Trigger();
}

FScopedExecution::FScopedExecution(FScopedExecution&& Other) : FScopedExecution{}
{
	using ::Swap;
	Swap(*this, Other);
}

FScopedExecution& FScopedExecution::operator=(FScopedExecution Other)
{
	Swap(*this, Other);
	return *this;
}

void FScopedExecution::Reset()
{
	Func.Reset();
}

void FScopedExecution::Trigger()
{
	if (Func.IsSet())
	{
		Func();
		Reset();
	}
}

[[nodiscard]] TFunction<void()> FScopedExecution::Release()
{
	return MoveTemp(Func);
}

void Swap(FScopedExecution& Lhs, FScopedExecution& Rhs)
{
	using ::Swap;
	Swap(Lhs.Func, Rhs.Func);
}

}  // namespace Zkz
