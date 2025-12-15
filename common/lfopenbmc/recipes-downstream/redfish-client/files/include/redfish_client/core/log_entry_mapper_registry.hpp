#pragma once

#include <redfish_client/core/log_service_handler.hpp>

#include <redfish_client/core/log_entry_mapper.hpp>

#include <set>
#include <memory>
#include <functional>

namespace redfish_client::core {

class LogEntryMapperRegistry {
public:
  static LogEntryMapperRegistry& instance() {
    static LogEntryMapperRegistry registry;
    return registry;
  }

  void registerMapper(
    std::unique_ptr<LogEntryMapper> factory,
    int priority = 50);

  LogEntryMapper& resolve(redfish_binding::LogEntry::LogEntry& entry);

private:
  LogEntryMapperRegistry() = default;

  void ensureInitialized();

  struct MapperInfo {
      std::unique_ptr<LogEntryMapper> instance;
      int priority;
      bool operator<(MapperInfo const& other) const {
        return priority > other.priority;
      }
  };

  std::set<MapperInfo> mappers_;
  bool initialized_ = false;
};

} // namespace redfish_client::core
