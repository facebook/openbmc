#pragma once

#include <redfish_client/core/log_entry_mapper.hpp>
#include <redfish_client/core/config.hpp>
#include <vector>

namespace redfish_client::core
{

class SensorThresholdMapper : public LogEntryMapper
{
  public:
    SensorThresholdMapper(const std::vector<SensorMapper>& mappers) :
        mappers(mappers)
    {}

    bool canHandle(
        redfish_binding::LogEntry::LogEntry& entry) const override;

    void map(redfish_binding::LogEntry::LogEntry& entry) override;

  private:
    std::vector<SensorMapper> mappers;
};

} // namespace redfish_client::core
