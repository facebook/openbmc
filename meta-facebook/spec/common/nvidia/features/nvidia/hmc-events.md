# Meta OpenBMC Events for NVIDIA HMC Compute Trays

This document lists the event log requirements common to Meta OpenBMC
compute trays managed by an NVIDIA HMC (Hardware Management Controller)
over Redfish — currently Catalina (GB200) and Clemente (GB300), and
extending to future HMC-managed platforms.

The BMC shall surface events originating from the NVIDIA HMC (GPU and
CPU domains) alongside its own platform events, preserving event
severity, source identification, and resolution data.

Components: grace_cpu, blackwell_gpu, hgx_power_supply

## HMC Event Forwarding
| **Event Category** | **Event** | **Source** | **Comments** |
|---|---|---|---|
| HMC Log Forwarding | All HMC log entries | HMC EventLog | BMC shall forward HMC events as BMC events without loss of severity, source, or resolution data |

## HMC Power Rail Fault Events
| **Event Category** | **Event** | **Source** | **Comments** |
|---|---|---|---|
| Power Rail Fault | PS_RUN_PWR_FAULT | HMC EventLog | BMC shall surface a discrete PowerRailFault event per failed power state (e.g., PWR_FAIL_GPU, PWRSEQ_FAIL_STATE, PWRSEQ_GPU_FAIL_STATE) |

## HMC CPER Events
| **Event Category** | **Event** | **Source** | **Comments** |
|---|---|---|---|
| CPU CPER | CPER error records from Grace CPU | HMC EventLog | BMC shall decode CPER records into structured BMC error events |

## HMC Unhandled Events
| **Event Category** | **Event** | **Source** | **Comments** |
|---|---|---|---|
| Unhandled HMC Event | Any HMC event not matched by a specific mapper | HMC EventLog | BMC shall preserve the original MessageId, Message, Severity, and Resolution data as a generic BMC event |
