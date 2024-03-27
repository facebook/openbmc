#include "utils/register.hpp"

#include <CLI/CLI.hpp>

#include <algorithm>

int main(int argc, char** argv)
{
    CLI::App app{"mfg-tool: temporary utilities for manufacturing support"};
    app.require_subcommand(1);
    mfgtool::init_commands(app);

    CLI11_PARSE(app, argc, argv);
    return 0;
}
