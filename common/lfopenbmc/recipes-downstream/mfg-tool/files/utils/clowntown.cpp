#include "utils/clowntown.hpp"

namespace mfgtool::clowntown
{

bool& flag()
{
    static bool value = false;
    return value;
}

} // namespace mfgtool::clowntown
