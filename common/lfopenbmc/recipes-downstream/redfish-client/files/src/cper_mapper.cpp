#include <redfish_client/core/cper_mapper.hpp>
#include <phosphor-logging/lg2.hpp>
#include <phosphor-logging/commit.hpp>
#include <xyz/openbmc_project/State/CPER/event.hpp>
#include "redfish-binding/LogEntry_EventSeverity.hpp"

namespace redfish_client::core {

namespace
{

using EventSeverity = redfish_binding::LogEntry::EventSeverity;

template <typename T>
T makeCPER(const auto& source, const auto& cper)
{
    return T("SOURCE", sdbusplus::message::object_path(source), "CPER",
             cper.toJson().dump());
}

std::optional<std::string> cperOemNvidiaHandler(const nlohmann::json& entryJson)
{
    try
    {
        const auto& cperOemNvidia = entryJson.at("CPER").at("Oem").at("Nvidia");

        if (cperOemNvidia.contains("Pcie"))
        {
            const auto& pcie = cperOemNvidia["Pcie"];
            if (pcie.contains("DeviceID"))
            {
                const auto& deviceId = pcie["DeviceID"];
                if (deviceId.contains("PrimaryOrDeviceBusNumber") &&
                    deviceId.contains("DeviceNumber") &&
                    deviceId.contains("FunctionNumber"))
                {
                    return std::format(
                        "pcie-b{}-d{}-f{}",
                        deviceId["PrimaryOrDeviceBusNumber"].get<int>(),
                        deviceId["DeviceNumber"].get<int>(),
                        deviceId["FunctionNumber"].get<int>());
                }
            }
        }
    }
    catch (...)
    {
        // silent catch, just return null at end of function.
    }

    return std::nullopt;
}

} // anonymous namespace

bool CperMapper::canHandle(
      redfish_binding::LogEntry::LogEntry& entry) const {
  return entry.getCPER().hasValue();
}

void CperMapper::map(
    redfish_binding::LogEntry::LogEntry& entry) {
  namespace CPER = sdbusplus::error::xyz::openbmc_project::state::CPER;

  auto& maybeCPER = entry.getCPER();
  auto& maybeLinks = entry.getLinks();

  std::string source = [&]() -> std::string {
      if (maybeLinks.hasValue()) {
          auto& origin = maybeLinks.value().getOriginOfCondition();
          if (origin.hasValue() &&
              origin.value().getOdataId().hasValue()) {
              return origin.value().getOdataId().value();
          }
      }

      auto findSource = cperOemNvidiaHandler(entry.toJson());
      if (findSource) {
          return *findSource;
      }

      return "Unknown Source";
  }();

  auto eventSeverity = entry.getSeverity().hasValue()
                           ? entry.getSeverity().value()
                           : EventSeverity::Critical;

  if (eventSeverity == EventSeverity::Critical) {
      lg2::commit(makeCPER<CPER::GenericCPERFault>(
          source, maybeCPER.value()));
  } else {
      lg2::commit(makeCPER<CPER::GenericCPERWarning>(
          source, maybeCPER.value()));
  }
}

} // redfish_client::core
