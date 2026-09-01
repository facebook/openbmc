#include <redfish_client/core/environmental_mapper.hpp>

#include <phosphor-logging/commit.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/State/Fan/event.hpp>
#include <xyz/openbmc_project/State/Filter/event.hpp>
#include <xyz/openbmc_project/State/Leak/Detector/event.hpp>
#include <xyz/openbmc_project/State/Pump/event.hpp>
#include <xyz/openbmc_project/State/Valve/event.hpp>

#include <string>
#include <string_view>
#include <vector>

PHOSPHOR_LOG2_USING;

namespace redfish_client::core
{

namespace
{

using MsgArgs = std::vector<std::string>;

namespace FanError = sdbusplus::error::xyz::openbmc_project::state::Fan;
namespace FanEvent = sdbusplus::event::xyz::openbmc_project::state::Fan;
namespace FilterError = sdbusplus::error::xyz::openbmc_project::state::Filter;
namespace FilterEvent = sdbusplus::event::xyz::openbmc_project::state::Filter;
namespace LeakDetectorError =
    sdbusplus::error::xyz::openbmc_project::state::leak::Detector;
namespace LeakDetectorEvent =
    sdbusplus::event::xyz::openbmc_project::state::leak::Detector;
namespace PumpError = sdbusplus::error::xyz::openbmc_project::state::Pump;
namespace PumpEvent = sdbusplus::event::xyz::openbmc_project::state::Pump;
namespace ValveError = sdbusplus::error::xyz::openbmc_project::state::Valve;

std::string argStr(const MsgArgs& args, size_t i)
{
    return i < args.size() ? args[i] : std::string{};
}

sdbusplus::object_path argPath(const MsgArgs& args, size_t i)
{
    return sdbusplus::object_path(argStr(args, i));
}

std::string_view getPrefix(const std::string& messageId)
{
    std::string_view id = messageId;
    auto pos = id.find('.');
    return pos == std::string_view::npos ? id : id.substr(0, pos);
}

std::string_view getSuffix(const std::string& messageId)
{
    std::string_view id = messageId;
    auto pos = id.find_last_of('.');
    return pos == std::string_view::npos ? id : id.substr(pos + 1);
}

template <typename T>
T makeFan(const MsgArgs& args)
{
    return T("FAN_NAME", argPath(args, 0));
}

template <typename T>
T makeFilter(const MsgArgs& args)
{
    return T("FILTER_NAME", argPath(args, 0));
}

template <typename T>
T makeLeakDetector(const MsgArgs& args)
{
    return T("DETECTOR_NAME", argPath(args, 0));
}

template <typename T>
T makePump(const MsgArgs& args)
{
    return T("PUMP_NAME", argPath(args, 0));
}

template <typename T>
T makeValve(const MsgArgs& args)
{
    return T("VALVE_NAME", argPath(args, 0));
}

} // anonymous namespace

bool EnvironmentalMapper::canHandle(
    redfish_binding::LogEntry::LogEntry& entry) const
{
    auto& maybeMessageId = entry.getMessageId();
    if (!maybeMessageId.hasValue())
    {
        return false;
    }
    const auto& msgId = maybeMessageId.value();
    if (getPrefix(msgId) != "Environmental")
    {
        return false;
    }
    auto suffix = getSuffix(msgId);
    return suffix == "FanFailed"                  ||
           suffix == "FanRestored"                ||
           suffix == "FilterRequiresService"      ||
           suffix == "FilterRestored"             ||
           suffix == "LeakDetectedCritical"       ||
           suffix == "LeakDetectedNormal"         ||
           suffix == "LeakDetectedWarning"        ||
           suffix == "PumpFailed"                 ||
           suffix == "PumpRestored"               ||
           suffix == "ValveUnableToReachSetPoint";
}

void EnvironmentalMapper::map(redfish_binding::LogEntry::LogEntry& entry)
{
    auto suffix = getSuffix(entry.getMessageId().value());

    static const MsgArgs kNoArgs;
    auto& maybeArgs = entry.getMessageArgs();
    const MsgArgs& args = maybeArgs.hasValue() ? maybeArgs.value() : kNoArgs;

    if (suffix == "FanFailed")
    {
        lg2::commit(makeFan<FanError::FanFailed>(args));
    }
    else if (suffix == "FanRestored")
    {
        lg2::commit(makeFan<FanEvent::FanRestored>(args));
    }
    else if (suffix == "FilterRequiresService")
    {
        lg2::commit(makeFilter<FilterError::FilterRequiresService>(args));
    }
    else if (suffix == "FilterRestored")
    {
        lg2::commit(makeFilter<FilterEvent::FilterRestored>(args));
    }
    else if (suffix == "LeakDetectedCritical")
    {
        lg2::commit(
            makeLeakDetector<LeakDetectorError::LeakDetectedCritical>(args));
    }
    else if (suffix == "LeakDetectedWarning")
    {
        lg2::commit(
            makeLeakDetector<LeakDetectorError::LeakDetectedWarning>(args));
    }
    else if (suffix == "LeakDetectedNormal")
    {
        lg2::commit(
            makeLeakDetector<LeakDetectorEvent::LeakDetectedNormal>(args));
    }
    else if (suffix == "PumpFailed")
    {
        lg2::commit(makePump<PumpError::PumpFailed>(args));
    }
    else if (suffix == "PumpRestored")
    {
        lg2::commit(makePump<PumpEvent::PumpRestored>(args));
    }
    else if (suffix == "ValveUnableToReachSetPoint")
    {
        lg2::commit(makeValve<ValveError::ValveUnableToReachSetPoint>(args));
    }
    else
    {
        warning("EnvironmentalMapper::map: unhandled suffix {SUFFIX}", "SUFFIX",
                std::string(suffix));
    }
}

} // namespace redfish_client::core
