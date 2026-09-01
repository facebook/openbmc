#include <redfish_client/core/openbmc_mapper.hpp>

#include <phosphor-logging/commit.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/Chassis/Buttons/Button/event.hpp>
#include <xyz/openbmc_project/Common/FactoryReset/event.hpp>
#include <xyz/openbmc_project/Common/event.hpp>
#include <xyz/openbmc_project/Logging/event.hpp>
#include <xyz/openbmc_project/State/BMC/event.hpp>
#include <xyz/openbmc_project/State/CPER/event.hpp>
#include <xyz/openbmc_project/State/Cable/event.hpp>
#include <xyz/openbmc_project/State/Leak/DetectorGroup/event.hpp>
#include <xyz/openbmc_project/State/LockOut/event.hpp>
#include <xyz/openbmc_project/State/Power/event.hpp>
#include <xyz/openbmc_project/State/SMC/event.hpp>
#include <xyz/openbmc_project/State/Thermal/event.hpp>
#include <xyz/openbmc_project/State/Valve/event.hpp>

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

PHOSPHOR_LOG2_USING;

namespace redfish_client::core
{

namespace
{

using MsgArgs = std::vector<std::string>;

namespace ButtonEvent =
    sdbusplus::event::xyz::openbmc_project::chassis::buttons::Button;
namespace CommonError = sdbusplus::error::xyz::openbmc_project::Common;
namespace FactoryResetError =
    sdbusplus::error::xyz::openbmc_project::common::FactoryReset;
namespace FactoryResetEvent =
    sdbusplus::event::xyz::openbmc_project::common::FactoryReset;
namespace LoggingEvent = sdbusplus::event::xyz::openbmc_project::Logging;
namespace BmcEvent = sdbusplus::event::xyz::openbmc_project::state::BMC;
namespace CperError = sdbusplus::error::xyz::openbmc_project::state::CPER;
namespace CableError = sdbusplus::error::xyz::openbmc_project::state::Cable;
namespace CableEvent = sdbusplus::event::xyz::openbmc_project::state::Cable;
namespace LeakGroupError =
    sdbusplus::error::xyz::openbmc_project::state::leak::DetectorGroup;
namespace LeakGroupEvent =
    sdbusplus::event::xyz::openbmc_project::state::leak::DetectorGroup;
namespace LockOutEvent = sdbusplus::event::xyz::openbmc_project::state::LockOut;
namespace PowerError = sdbusplus::error::xyz::openbmc_project::state::Power;
namespace PowerEvent = sdbusplus::event::xyz::openbmc_project::state::Power;
namespace SmcError = sdbusplus::error::xyz::openbmc_project::state::SMC;
namespace SmcEvent = sdbusplus::event::xyz::openbmc_project::state::SMC;
namespace ThermalError = sdbusplus::error::xyz::openbmc_project::state::Thermal;
namespace ThermalEvent = sdbusplus::event::xyz::openbmc_project::state::Thermal;
namespace ValveEvent = sdbusplus::event::xyz::openbmc_project::state::Valve;
using CommonBmc = sdbusplus::common::xyz::openbmc_project::state::BMC;

std::string argStr(const MsgArgs& args, size_t i)
{
    return i < args.size() ? args[i] : std::string{};
}

sdbusplus::object_path argPath(const MsgArgs& args, size_t i)
{
    return sdbusplus::object_path(argStr(args, i));
}

uint64_t argUint(const MsgArgs& args, size_t i)
{
    uint64_t value = 0;
    std::string text = argStr(args, i);
    std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}

std::string_view getSuffix(const std::string& messageId)
{
    std::string_view id = messageId;
    auto pos = id.find_last_of('.');
    return pos == std::string_view::npos ? id : id.substr(pos + 1);
}

template <typename T>
T makeReset(const MsgArgs& args)
{
    return T("RESET_CAUSE", argStr(args, 0), "SOURCE", argPath(args, 1));
}

template <typename T>
T makeCper(const MsgArgs& args)
{
    return T("SOURCE", argPath(args, 0), "CPER", argStr(args, 1));
}

template <typename T>
T makePortId(const MsgArgs& args)
{
    return T("PORT_ID", argStr(args, 0));
}

template <typename T>
T makeDetectorGroup(const MsgArgs& args)
{
    return T("DETECTOR_GROUP_NAME", argPath(args, 0));
}

template <typename T>
T makeIdentifier(const MsgArgs& args)
{
    return T("IDENTIFIER", argPath(args, 0));
}

template <typename T>
T makeValveName(const MsgArgs& args)
{
    return T("VALVE_NAME", argPath(args, 0));
}

template <typename T>
T makeDeviceFailure(const MsgArgs& args)
{
    return T("DEVICE", argPath(args, 0), "FAILURE_DATA", argStr(args, 1));
}

CommonError::ObjectAlreadyExists makeObjectAlreadyExists(const MsgArgs& args)
{
    return CommonError::ObjectAlreadyExists("OBJECT_PATH", argPath(args, 0));
}

PowerError::PowerRailFault makePowerRailFault(const MsgArgs& args)
{
    return PowerError::PowerRailFault("POWER_RAIL", argPath(args, 0),
                                      "FAILURE_DATA", argStr(args, 1));
}

PowerEvent::PowerRailFaultRecovered makePowerRailFaultRecovered(
    const MsgArgs& args)
{
    return PowerEvent::PowerRailFaultRecovered("POWER_RAIL", argPath(args, 0));
}

PowerError::VoltageRegulatorFault makeVoltageRegulatorFault(const MsgArgs& args)
{
    return PowerError::VoltageRegulatorFault(
        "VOLTAGE_REGULATOR", argPath(args, 0), "FAILURE_DATA", argStr(args, 1));
}

PowerEvent::VoltageRegulatorFaultRecovered makeVoltageRegulatorFaultRecovered(
    const MsgArgs& args)
{
    return PowerEvent::VoltageRegulatorFaultRecovered("VOLTAGE_REGULATOR",
                                                      argPath(args, 0));
}

SmcError::SMCFailed makeSmcFailed(const MsgArgs& args)
{
    return SmcError::SMCFailed("IDENTIFIER", argPath(args, 0), "FAILURE_TYPE",
                              argStr(args, 1));
}

ThermalEvent::DeviceOperatingNormalTemperature makeDeviceOperatingNormal(
    const MsgArgs& args)
{
    return ThermalEvent::DeviceOperatingNormalTemperature("DEVICE",
                                                          argPath(args, 0));
}

ButtonEvent::ButtonPressed makeButtonPressed(const MsgArgs& args)
{
    return ButtonEvent::ButtonPressed("BUTTON_NAME", argPath(args, 0),
                                      "ENGAGE_DURATION", argUint(args, 1));
}

LoggingEvent::Cleared makeCleared(const MsgArgs& args)
{
    return LoggingEvent::Cleared("NUMBER_OF_LOGS", argUint(args, 0));
}

std::optional<BmcEvent::RebootCause> makeRebootCause(const MsgArgs& args)
{
    auto cause = sdbusplus::message::convert_from_string<CommonBmc::RebootCause>(
        argStr(args, 0));
    if (!cause)
    {
        return std::nullopt;
    }
    return BmcEvent::RebootCause("CAUSE", *cause, "BOOT_DEVICE",
                                 argStr(args, 1));
}

std::optional<BmcEvent::StateChanged> makeStateChanged(const MsgArgs& args)
{
    auto state = sdbusplus::message::convert_from_string<CommonBmc::BMCState>(
        argStr(args, 0));
    if (!state)
    {
        return std::nullopt;
    }
    return BmcEvent::StateChanged("STATE", *state);
}

} // anonymous namespace

bool OpenBmcMapper::canHandle(redfish_binding::LogEntry::LogEntry& entry) const
{
    auto& maybeMessageId = entry.getMessageId();
    if (!maybeMessageId.hasValue())
    {
        return false;
    }
    const auto& msgId = maybeMessageId.value();
    if (!msgId.starts_with("OpenBMC_"))
    {
        return false;
    }
    auto suffix = getSuffix(msgId);
    return suffix == "ButtonPressed"                       ||
           suffix == "ResetFailure"                        ||
           suffix == "ResetSuccess"                        ||
           suffix == "ObjectAlreadyExists"                 ||
           suffix == "Cleared"                             ||
           suffix == "RebootCause"                         ||
           suffix == "StateChanged"                        ||
           suffix == "GenericCPERFault"                    ||
           suffix == "GenericCPERWarning"                  ||
           suffix == "CableConnected"                      ||
           suffix == "CableDisconnected"                   ||
           suffix == "DetectorGroupCritical"               ||
           suffix == "DetectorGroupNormal"                 ||
           suffix == "DetectorGroupWarning"                ||
           suffix == "LockOutDisabled"                     ||
           suffix == "LockOutEnabled"                      ||
           suffix == "PowerRailFault"                      ||
           suffix == "PowerRailFaultRecovered"             ||
           suffix == "VoltageRegulatorFault"               ||
           suffix == "VoltageRegulatorFaultRecovered"      ||
           suffix == "SMCFailed"                           ||
           suffix == "SMCRestored"                         ||
           suffix == "DeviceOperatingNormalTemperature"    ||
           suffix == "DeviceOverOperatingTemperature"      ||
           suffix == "DeviceOverOperatingTemperatureFault" ||
           suffix == "ValveClose"                          ||
           suffix == "ValveOpen";
}

void OpenBmcMapper::map(redfish_binding::LogEntry::LogEntry& entry)
{
    auto suffix = getSuffix(entry.getMessageId().value());

    static const MsgArgs kNoArgs;
    auto& maybeArgs = entry.getMessageArgs();
    const MsgArgs& args = maybeArgs.hasValue() ? maybeArgs.value() : kNoArgs;

    if (suffix == "ResetFailure")
    {
        lg2::commit(makeReset<FactoryResetError::ResetFailure>(args));
    }
    else if (suffix == "ResetSuccess")
    {
        lg2::commit(makeReset<FactoryResetEvent::ResetSuccess>(args));
    }
    else if (suffix == "GenericCPERFault")
    {
        lg2::commit(makeCper<CperError::GenericCPERFault>(args));
    }
    else if (suffix == "GenericCPERWarning")
    {
        lg2::commit(makeCper<CperError::GenericCPERWarning>(args));
    }
    else if (suffix == "CableConnected")
    {
        lg2::commit(makePortId<CableEvent::CableConnected>(args));
    }
    else if (suffix == "CableDisconnected")
    {
        lg2::commit(makePortId<CableError::CableDisconnected>(args));
    }
    else if (suffix == "DetectorGroupCritical")
    {
        lg2::commit(
            makeDetectorGroup<LeakGroupError::DetectorGroupCritical>(args));
    }
    else if (suffix == "DetectorGroupWarning")
    {
        lg2::commit(
            makeDetectorGroup<LeakGroupError::DetectorGroupWarning>(args));
    }
    else if (suffix == "DetectorGroupNormal")
    {
        lg2::commit(
            makeDetectorGroup<LeakGroupEvent::DetectorGroupNormal>(args));
    }
    else if (suffix == "LockOutEnabled")
    {
        lg2::commit(makeIdentifier<LockOutEvent::LockOutEnabled>(args));
    }
    else if (suffix == "LockOutDisabled")
    {
        lg2::commit(makeIdentifier<LockOutEvent::LockOutDisabled>(args));
    }
    else if (suffix == "SMCRestored")
    {
        lg2::commit(makeIdentifier<SmcEvent::SMCRestored>(args));
    }
    else if (suffix == "ValveOpen")
    {
        lg2::commit(makeValveName<ValveEvent::ValveOpen>(args));
    }
    else if (suffix == "ValveClose")
    {
        lg2::commit(makeValveName<ValveEvent::ValveClose>(args));
    }
    else if (suffix == "DeviceOverOperatingTemperature")
    {
        lg2::commit(
            makeDeviceFailure<ThermalError::DeviceOverOperatingTemperature>(
                args));
    }
    else if (suffix == "DeviceOverOperatingTemperatureFault")
    {
        lg2::commit(
            makeDeviceFailure<
                ThermalError::DeviceOverOperatingTemperatureFault>(args));
    }
    else if (suffix == "ObjectAlreadyExists")
    {
        lg2::commit(makeObjectAlreadyExists(args));
    }
    else if (suffix == "PowerRailFault")
    {
        lg2::commit(makePowerRailFault(args));
    }
    else if (suffix == "PowerRailFaultRecovered")
    {
        lg2::commit(makePowerRailFaultRecovered(args));
    }
    else if (suffix == "VoltageRegulatorFault")
    {
        lg2::commit(makeVoltageRegulatorFault(args));
    }
    else if (suffix == "VoltageRegulatorFaultRecovered")
    {
        lg2::commit(makeVoltageRegulatorFaultRecovered(args));
    }
    else if (suffix == "SMCFailed")
    {
        lg2::commit(makeSmcFailed(args));
    }
    else if (suffix == "DeviceOperatingNormalTemperature")
    {
        lg2::commit(makeDeviceOperatingNormal(args));
    }
    else if (suffix == "ButtonPressed")
    {
        lg2::commit(makeButtonPressed(args));
    }
    else if (suffix == "Cleared")
    {
        lg2::commit(makeCleared(args));
    }
    else if (suffix == "RebootCause")
    {
        if (auto event = makeRebootCause(args))
        {
            lg2::commit(std::move(*event));
        }
        else
        {
            warning("OpenBmcMapper::map: invalid CAUSE {VAL}", "VAL",
                    argStr(args, 0));
        }
    }
    else if (suffix == "StateChanged")
    {
        if (auto event = makeStateChanged(args))
        {
            lg2::commit(std::move(*event));
        }
        else
        {
            warning("OpenBmcMapper::map: invalid STATE {VAL}", "VAL",
                    argStr(args, 0));
        }
    }
    else
    {
        warning("OpenBmcMapper::map: unhandled suffix {SUFFIX}", "SUFFIX",
                std::string(suffix));
    }
}

} // namespace redfish_client::core
