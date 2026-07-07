# Event Emulator

A command-line tool for generating and resolving emulated OpenBMC events for
various device types. Useful for testing event pipelines, Redfish log entries,
and resolution workflows without real hardware faults.

All emulated events use D-Bus paths suffixed with `_EMULATED` to distinguish
them from real device events.

## Usage

```bash
# Generate an event
event-emulator <device-type> <event-type>

# Resolve a previously generated event
event-emulator resolve <device-type> <event-type>

# Show supported events for a device
event-emulator <device-type>
```

## Device Types

- `psu` — Power Supply Unit
- `bbu` — Battery Backup Unit
- `cbu` — Capacitor Bank Unit

## Event Types

| Event Type           | Description                        |
|----------------------|------------------------------------|
| reading-critical     | Sensor reading critical threshold  |
| power-fault          | Power rail fault                   |
| fan-failure          | Fan failure                        |
| controller-failure   | SMC/controller failure             |
| sensor-failure       | Sensor failure                     |
| all                  | Generate/resolve all device events |

Not all devices support all event types. Use `event-emulator <device-type>` to
see supported events.

## Examples

```bash
# Generate a PSU power fault event
event-emulator psu power-fault
# output: power-fault: /xyz/openbmc_project/logging/entry/42

# Generate all BBU events
event-emulator bbu all

# Resolve the PSU power fault
event-emulator resolve psu power-fault

# Resolve all BBU events
event-emulator resolve bbu all
```

## State

Pending event paths are stored in `/tmp/event-emulator/events.json`. This
allows the resolve command to find previously generated events without
requiring the user to track event paths manually.

## Adding a New Device

Create a new file in `devices/` with a `DeviceRegistration`:

```cpp
#include "utils/device_registry.hpp"

namespace event_emulator
{

static DeviceRegistration myDeviceRegistration("mydevice", [] {
    return DeviceEventData{
        .sensorPath = "/xyz/openbmc_project/sensor/MY_SENSOR",
        // ... other paths ...
        .supportedEvents = {"reading-critical", "fan-failure"},
    };
});

} // namespace event_emulator
```

Add the file to `meson.build` and rebuild. No other changes needed.

## Adding a New Event Type

1. If the new event uses a D-Bus path not already in `DeviceEventData`, add a
   new path field to the struct in `utils/device_events.hpp` and populate it
   in the device files under `devices/` that support it.

2. Add generate and resolve functions in `utils/device_events.hpp` and
   `utils/device_events.cpp` following the existing pattern.

3. Add dispatch entries in `utils/event_actions.cpp` in both
   `dispatchGenerate()` and `dispatchResolve()`:

   ```cpp
   if (eventType == "my-event")
   {
       co_return co_await generateMyEvent(ctx, data);
   }
   ```

4. Add the event type string to `supportedEvents` in each device file under
   `devices/` that supports it.
