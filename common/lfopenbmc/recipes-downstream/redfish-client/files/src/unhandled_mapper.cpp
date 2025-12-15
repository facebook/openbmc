#include <redfish_client/core/unhandled_mapper.hpp>
#include "redfish-binding/LogEntry_EventSeverity.hpp"
#include <phosphor-logging/commit.hpp>
#include <phosphor-logging/lg2.hpp>

#include <exception>

namespace redfish_client::core {


namespace {

using EventSeverity = redfish_binding::LogEntry::EventSeverity;

class UnhandledException : public sdbusplus::exception::generated_event_base
{
  public:
    UnhandledException(EventSeverity eventSeverity,
                       const nlohmann::json& eventData) :
        eventSeverity(eventSeverity), eventData(eventData)
    {}

    const char* name() const noexcept override
    {
        return kExceptionName;
    }

    const char* description() const noexcept override
    {
        return "Unhandled Exception from Satellite MC";
    }

    int get_errno() const noexcept override
    {
        return EIO;
    }

    nlohmann::json to_json() const override
    {
        nlohmann::json j = {};
        j["REDFISH_EVENT"] = eventData.dump();

        return nlohmann::json{{kExceptionName, std::move(j)}};
    }

    int severity() const noexcept override
    {
        switch (eventSeverity)
        {
            case EventSeverity::OK:
                return LOG_INFO;
            case EventSeverity::Warning:
                return LOG_WARNING;
            case EventSeverity::Critical:
                return LOG_CRIT;
            default:
                return LOG_ERR;
        }
    }

  private:
    EventSeverity eventSeverity;
    nlohmann::json eventData;
    static constexpr const char* kExceptionName =
        "com.meta.RedfishClient.UnexpectedException";
};

} // anonymous namespace

bool UnhandledMapper::canHandle(
      redfish_binding::LogEntry::LogEntry& entry) const {
  return true;
}

void UnhandledMapper::map(
    redfish_binding::LogEntry::LogEntry& entry) {
  auto eventSeverity = entry.getSeverity().hasValue()
                           ? entry.getSeverity().value()
                           : EventSeverity::Critical;

  lg2::commit(UnhandledException(eventSeverity, entry.toJson()));
}

} // redfish_client::core
