#include "utils/json.hpp"

#include <iostream>

namespace mfgtool::json
{
void display(const js& j)
{
    std::cout << j.dump(4) << std::endl;
}
} // namespace mfgtool::json
