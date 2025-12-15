#pragma once

#include "redfish-binding/LogEntry_LogEntry.hpp"

namespace redfish_client::core {

class LogEntryMapper {
public:
  virtual ~LogEntryMapper() = default;

  virtual bool canHandle(
      redfish_binding::LogEntry::LogEntry& entry) const = 0;

  virtual void map(
      redfish_binding::LogEntry::LogEntry& entry) = 0;
};

} // redfish_client::core
