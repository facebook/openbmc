#include "utils/json.hpp"
#include "utils/register.hpp"

#include <phosphor-logging/lg2.hpp>

namespace mfgtool::cmds::hello
{
PHOSPHOR_LOG2_USING;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("hello", "Hello World");
        init_callback(cmd, *this);
    }

    void run()
    {
        info("Hello World!");
        json::display(js{"Hello World!"});
    }
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::hello
