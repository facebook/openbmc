#include "utils/clowntown.hpp"
#include "utils/json.hpp"
#include "utils/register.hpp"
#include "utils/supervise.hpp"

#include <CLI/CLI.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char** argv)
{
    CLI::App app{"mfg-tool: temporary utilities for manufacturing support"};
    app.require_subcommand(1);

    // Global expert-mode flag.  When set, mfg-tool skips its safety guardrails
    // (e.g. refusing to run when multiple services publish conflicting data for
    // the same object).  Bound to the global accessor so any command can honor
    // it.  Use it before the subcommand, e.g. `mfg-tool --clowntown inventory`.
    app.add_flag(
        "--clowntown", mfgtool::clowntown::flag(),
        "Expert mode: skip mfg-tool's safety guardrails. You are telling the "
        "tool you know what you are doing and do not want it being careful.");

    mfgtool::init_commands(app);

    int timeout = 0;
    int retries = 0;
    std::string configFN{};

    if (getenv("HOME"))
    {
        configFN = std::format("{}/.mfgtool.json", getenv("HOME"));
    }
    if (configFN.length() && std::filesystem::exists(configFN))
    {
        std::ifstream ifs(configFN);
        auto config = mfgtool::js::parse(ifs);

        if (auto iter = config.find("timeout"); iter != config.end())
        {
            timeout = config["timeout"];
        }
        if (auto iter = config.find("retries"); iter != config.end())
        {
            retries = config["retries"];
        }
    }
    auto _ = [&]() {
        CLI11_PARSE(app, argc, argv);
        return 0;
    };
    return mfg_tool::supervise(_, timeout, retries);
}
