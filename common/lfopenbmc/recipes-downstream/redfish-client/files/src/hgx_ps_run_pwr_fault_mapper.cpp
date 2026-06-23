#include <redfish_client/core/hgx_ps_run_pwr_fault_mapper.hpp>
#include <redfish_client/core/log_entry_mapper_utils.hpp>
#include <phosphor-logging/lg2.hpp>
#include <phosphor-logging/commit.hpp>
#include <xyz/openbmc_project/State/Power/event.hpp>

#include <string>
#include <vector>

PHOSPHOR_LOG2_USING;

namespace redfish_client::core {

namespace
{

namespace PowerError = sdbusplus::error::xyz::openbmc_project::state::Power;

constexpr std::string_view kPsRunPwrFaultPattern = "PS_RUN_PWR_FAULT";

// BMC dbus inventory path for the NVIDIA HMC board.
constexpr std::string_view kBmcHmcBasePath =
    "/xyz/openbmc_project/inventory/system/board/NVIDIA_HMC";

bool matchesPowerRailFaultPattern(const std::string& value)
{
    return value.find(kPsRunPwrFaultPattern) != std::string::npos;
}

// Parses a semicolon-separated string of power states and extracts clean state
// names by stripping the hex value suffixes in curly braces.
//
// Input example: "PWR_FAIL_GPU{0x1};PWRSEQ_FAIL_STATE{0xf};PWRSEQ_GPU_FAIL{0x3}"
// Output: ["PWR_FAIL_GPU", "PWRSEQ_FAIL_STATE", "PWRSEQ_GPU_FAIL"]
//
// Each segment is delimited by ';' and may contain a hex value in curly braces
// (e.g., "{0x1}") which is stripped to get the clean power state name.
std::vector<std::string> parsePowerStates(const std::string& messageArg)
{
    std::vector<std::string> powerStates;

    // Helper to strip curly brace suffix and add to result if non-empty
    auto addState = [&powerStates](std::string state) {
        size_t bracePos = state.find('{');
        if (bracePos != std::string::npos)
        {
            state.resize(bracePos);
        }
        if (!state.empty())
        {
            powerStates.push_back(state);
        }
    };

    size_t start = 0;
    size_t end = 0;

    // Iterate through semicolon-delimited segments
    while ((end = messageArg.find(';', start)) != std::string::npos)
    {
        addState(messageArg.substr(start, end - start));
        start = end + 1;
    }

    // Handle the last (or only) segment after the final semicolon
    if (start < messageArg.size())
    {
        addState(messageArg.substr(start));
    }

    return powerStates;
}

} // anonymous namespace

bool HgxPsRunPwrFaultMapper::canHandle(
    redfish_binding::LogEntry::LogEntry& entry) const
{
    auto& maybeMessageId = entry.getMessageId();
    bool isResourceErrorsDetected =
        maybeMessageId.hasValue() &&
        maybeMessageId.value().find("ResourceErrorsDetected") !=
            std::string::npos;

    if (!isResourceErrorsDetected)
    {
        return false;
    }

    auto& maybeMessageArgs = entry.getMessageArgs();
    bool hasValidMessageArgs =
        maybeMessageArgs.hasValue() && maybeMessageArgs.value().size() >= 2;

    if (!hasValidMessageArgs)
    {
        return false;
    }

    const auto& messageArgs = maybeMessageArgs.value();
    bool hasPowerRailFaultInFirstArg =
        matchesPowerRailFaultPattern(messageArgs[0]);
    bool hasPowerStatesInSecondArg = !messageArgs[1].empty();

    return hasPowerRailFaultInFirstArg && hasPowerStatesInSecondArg;
}

void HgxPsRunPwrFaultMapper::map(
    redfish_binding::LogEntry::LogEntry& entry)
{
    std::string baseOrigin = std::string(kBmcHmcBasePath);
    std::string failureData = formatFailureData(entry);

    // MessageArgs[1] contains power state identifiers (e.g.,
    // "PWR_FAIL_GPU{0x1};PWRSEQ_FAIL_STATE{0xf};PWRSEQ_GPU_FAIL_STATE{0x3}").
    // Parse all semicolon-separated values and create individual PowerRailFault
    // events for each power failure status.
    auto& maybeMessageArgs = entry.getMessageArgs();
    if (maybeMessageArgs.hasValue() && maybeMessageArgs.value().size() > 1)
    {
        std::vector<std::string> powerStates =
            parsePowerStates(maybeMessageArgs.value()[1]);
        for (const auto& powerState : powerStates)
        {
            std::string origin = baseOrigin + "/" + powerState;
            lg2::commit(PowerError::PowerRailFault(
                PowerError::PowerRailFault::metadata_t<"POWER_RAIL">{
                    "POWER_RAIL"},
                sdbusplus::object_path(origin),
                PowerError::PowerRailFault::metadata_t<"FAILURE_DATA">{
                    "FAILURE_DATA"},
                failureData));
        }
    }
}

} // redfish_client::core
