#include <redfish_client/core/log_entry_mapper_registry.hpp>

namespace redfish_client::core {

void LogEntryMapperRegistry::registerMapper(
  std::unique_ptr<LogEntryMapper> mapper,
int priority) {
  mappers_.insert({std::move(mapper), priority});
}

LogEntryMapper& LogEntryMapperRegistry::resolve(redfish_binding::LogEntry::LogEntry& entry) {
	for (auto& info : mappers_) {
		if (info.instance->canHandle(entry)) {
			return *info.instance;
    }
  }

  throw std::runtime_error("No mapper found for log entry");
}

} // namespace redfish_client::core

