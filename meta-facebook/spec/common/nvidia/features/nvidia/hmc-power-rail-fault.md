BMC shall detect and log HMC power rail faults (PS_RUN_PWR_FAULT events)
reported via the HMC Redfish event log.

When a ResourceErrorsDetected event with PS_RUN_PWR_FAULT pattern is received,
the BMC parses the semicolon-separated power state identifiers from the message
arguments (e.g., "PWR_FAIL_GPU{0x1};PWRSEQ_FAIL_STATE{0xf}") and creates
individual PowerRailFault dbus events for each failed power state.

Each event includes:
- The power rail identifier (e.g., PWR_FAIL_GPU, PWRSEQ_FAIL_STATE)
- The originating HMC dbus path
- Failure context data (timestamp, message, severity, resolution)
