#pragma once

#include "redfish-binding/LogEntry_LogEntry.hpp"

#include <optional>
#include <string>

namespace redfish_client::core {

// Extracts the severity from a log entry and returns its string representation.
// Returns std::nullopt if severity is not present or not a recognized value.
std::optional<std::string> extractSeverity(
    redfish_binding::LogEntry::LogEntry& entry);

// Extracts the origin URL from a log entry's OriginOfCondition link.
// Returns "Unknown Source" if the origin is not available.
std::string extractOrigin(redfish_binding::LogEntry::LogEntry& entry);

// Formats key fields from a log entry as a JSON string for use as failure
// correlation data. The output is capped at maxSize bytes to ensure the
// resulting BMC event stays under 4k total.
std::string formatFailureData(
    redfish_binding::LogEntry::LogEntry& entry,
    size_t maxSize = 1536);

} // namespace redfish_client::core
