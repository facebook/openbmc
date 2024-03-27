#include "utils/register.hpp"

#include <algorithm>
#include <functional>
#include <vector>

namespace mfgtool
{
namespace details
{
using init_callback_t = std::function<void(CLI::App&)>;

// Use a static member to ensure .init order doesn't matter.
static auto& commands()
{
    static auto commands = std::vector<init_callback_t>{};
    return commands;
}

void register_command(init_callback_t cb)
{
    commands().push_back(cb);
}
} // namespace details

void init_commands(CLI::App& app)
{
    std::ranges::for_each(details::commands(), [&app](auto& cb) { cb(app); });
}
} // namespace mfgtool
