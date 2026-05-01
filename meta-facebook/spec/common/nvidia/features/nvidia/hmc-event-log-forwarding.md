BMC shall poll the HMC Redfish LogServices for new log entries and forward
them as BMC system event log entries.

The redfish-client daemon periodically queries the HMC's
`/redfish/v1/Systems/HGX_Baseboard_0/LogServices/EventLog/Entries` endpoint,
maps each Redfish log entry to the appropriate BMC dbus event using a
pluggable mapper registry, and commits the event via phosphor-logging.

Log entries that cannot be mapped by a specific handler are processed by
the unhandled mapper, which preserves the original Redfish event data
(MessageId, Message, Severity, Resolution) as a generic BMC event.
