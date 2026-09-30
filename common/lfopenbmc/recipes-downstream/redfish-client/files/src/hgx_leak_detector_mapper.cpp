#include <phosphor-logging/commit.hpp>
#include <phosphor-logging/lg2.hpp>
#include <redfish_client/core/hgx_leak_detector_mapper.hpp>
#include <redfish_client/core/log_entry_mapper_utils.hpp>
#include <xyz/openbmc_project/State/Leak/Detector/event.hpp>

#include <string>
#include <vector>

namespace redfish_client::core
{

bool HgxLeakDetectorMapper::canHandle(
    redfish_binding::LogEntry::LogEntry& entry) const
{
    auto& maybeMessageId = entry.getMessageId();
    bool isResourceStatusChangedCritical =
        maybeMessageId.hasValue() &&
        maybeMessageId.value().find("ResourceStatusChangedCritical") !=
            std::string::npos;

    if (!isResourceStatusChangedCritical)
    {
        return false;
    }

    std::string origin = extractOrigin(entry);
    if (origin.find("LeakDetectors") == std::string::npos)
    {
        return false;
    }

    auto& maybeMessageArgs = entry.getMessageArgs();
    return maybeMessageArgs.hasValue() && !maybeMessageArgs.value().empty();
}

void HgxLeakDetectorMapper::map(redfish_binding::LogEntry::LogEntry& entry)
{
    namespace LeakError =
        sdbusplus::error::xyz::openbmc_project::state::leak::Detector;

    auto& messageArgs = entry.getMessageArgs().value();
    const std::string& detectorName = messageArgs[0];

    std::string detectorPath =
        "/xyz/openbmc_project/state/leak/detector/" + detectorName;

    lg2::info("Leak detected! Detector: {PATH}", "PATH", detectorPath);

    lg2::commit(LeakError::LeakDetectedCritical("DETECTOR_NAME", detectorPath));
}

} // namespace redfish_client::core
