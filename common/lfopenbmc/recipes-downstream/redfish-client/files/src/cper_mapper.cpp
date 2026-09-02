#include <redfish_client/core/cper_mapper.hpp>
#include <phosphor-logging/lg2.hpp>
#include <phosphor-logging/commit.hpp>
#include <xyz/openbmc_project/Logging/Extension/CPER/Processed/common.hpp>
#include <xyz/openbmc_project/State/CPER/event.hpp>
#include "redfish-binding/LogEntry_EventSeverity.hpp"
#include "redfish-binding/LogEntry_LogDiagnosticDataTypes.hpp"

namespace redfish_client::core {

namespace
{

using EventSeverity = redfish_binding::LogEntry::EventSeverity;
using DiagnosticDataTypes = redfish_binding::LogEntry::LogDiagnosticDataTypes;
using Processed =
    sdbusplus::common::xyz::openbmc_project::logging::extension::cper::Processed;
using ContentType =
    sdbusplus::common::xyz::openbmc_project::logging::cper::Types::ContentType;

template <typename T>
T makeCPER(const auto& source, const auto& cper)
{
    return T("SOURCE", sdbusplus::object_path(source), "CPER",
             cper.toJson().dump());
}

// Build the `Extension.CPER.Processed` properties from the vendor's Redfish
// LogEntry. The vendor hands us CPER that is already decoded, so this maps
// json onto the extension directly rather than going through
// `Logging.CPER.Processor.Process`, which is the ingress for producers holding
// CPER binary.
Processed::properties_t makeProcessed(redfish_binding::LogEntry::LogEntry& entry,
                                      redfish_binding::LogEntry::CPER& cper)
{
    Processed::properties_t props;

    // Only CPER and CPERSection of the Redfish enum describe CPER content; the
    // rest (Manager, PreOS, OS, OEM) are unrelated diagnostic payloads and
    // never reach here, because `canHandle` gates on the CPER object. Absent
    // or unrecognised means a whole record, which is what an entry carrying a
    // NotificationType is.
    props.diagnostic_data_type = ContentType::CPER;
    if (auto& diagnosticDataType = entry.getDiagnosticDataType();
        diagnosticDataType.hasValue() &&
        diagnosticDataType.value() == DiagnosticDataTypes::CPERSection)
    {
        props.diagnostic_data_type = ContentType::CPERSection;
    }

    // A GUID only applies to one of the two content types, so the vendor omits
    // the inapplicable one. The interface documents an empty string -- also the
    // generated default -- as "not applicable or unavailable".
    if (auto& notificationType = cper.getNotificationType();
        notificationType.hasValue())
    {
        props.notification_type = notificationType.value();
    }
    if (auto& sectionType = cper.getSectionType(); sectionType.hasValue())
    {
        props.section_type = sectionType.value();
    }

    // `Oem` is a{ss}: vendor namespace -> a Redfish json object serialized as a
    // string. `Resource::Oem` declares no typed properties, so every vendor key
    // lands in the binding's leftover map and `toJson` returns it intact.
    if (auto& oem = cper.getOem(); oem.hasValue())
    {
        const auto oemJson = oem.value().toJson();
        if (oemJson.is_object())
        {
            for (const auto& [vendor, value] : oemJson.items())
            {
                props.oem[vendor] = value.dump();
            }
        }
    }

    return props;
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

  // The CPER metadata rides an extension interface so that bmcweb can render
  // it as a Redfish `CPER` object. The `CPER` AdditionalData string stays for
  // now: consumers that parse it out of `messageArgs` have not migrated to the
  // extension yet.
  auto processed = makeProcessed(entry, maybeCPER.value());

  if (eventSeverity == EventSeverity::Critical) {
      lg2::commit(makeCPER<CPER::GenericCPERFault>(
          source, maybeCPER.value()).extend(processed));
  } else {
      lg2::commit(makeCPER<CPER::GenericCPERWarning>(
          source, maybeCPER.value()).extend(processed));
  }
}

} // redfish_client::core
