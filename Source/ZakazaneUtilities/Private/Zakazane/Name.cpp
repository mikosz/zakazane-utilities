#include "Zakazane/Name.h"

namespace Zkz
{

bool AlphabeticalLess(const FName Lhs, const FName Rhs)
{
	return Lhs.Compare(Rhs) < 0;
}

}  // namespace Zkz
