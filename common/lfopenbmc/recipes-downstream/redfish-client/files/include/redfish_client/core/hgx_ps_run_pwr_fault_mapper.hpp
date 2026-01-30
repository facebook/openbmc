#pragma once

#include <redfish_client/core/log_entry_mapper.hpp>

namespace redfish_client::core {

class HgxPsRunPwrFaultMapper : public LogEntryMapper {
public:
  bool canHandle(
    redfish_binding::LogEntry::LogEntry& entry) const override;

  void map(redfish_binding::LogEntry::LogEntry& entry) override;
};

} // redfish_client::core
