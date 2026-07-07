# Event Emulator

A command-line tool for generating and resolving emulated OpenBMC events for
various device types. Useful for testing event pipelines, Redfish log entries,
and resolution workflows without real hardware faults.

All emulated events use D-Bus paths suffixed with `_EMULATED` to distinguish
them from real device events.

## Usage

```bash
# Generate an event
event-emulator <device-type> <event-type> [EventName]

# Resolve a previously generated event (-r / --resolve)
event-emulator <device-type> <event-type> [EventName] --resolve

# Show supported events for a device
event-emulator <device-type> --help
```

`EventName` is optional. When provided, it overrides the leaf (last segment)
of the event object path/name; the interface prefix and the `_EMULATED`
suffix are still applied. When omitted, the device's default name is used.
`EventName` cannot be combined with `all`.

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

Not all devices support all event types. Use
`event-emulator <device-type> --help` to see supported events.

## Examples

```bash
# Generate a PSU power fault event
event-emulator psu power-fault
# output: power-fault: /xyz/openbmc_project/logging/entry/42

# Generate a PSU power fault event with a custom name
event-emulator psu power-fault PSU_3_2_CUSTOM_ALARM
# fires on /xyz/openbmc_project/state/power_rail/PSU_3_2_CUSTOM_ALARM_EMULATED

# Resolve that custom event (pass the same name)
event-emulator psu power-fault PSU_3_2_CUSTOM_ALARM --resolve

# Generate all BBU events
event-emulator bbu all

# Resolve the PSU power fault (-r is shorthand for --resolve)
event-emulator psu power-fault -r

# Resolve all BBU events
event-emulator bbu all --resolve
```

## State

Pending event paths are stored in `/tmp/event-emulator/events.json`, keyed by
`device:event-type:event-name`. This allows `--resolve` to find previously
generated events without requiring the user to track event paths manually.
Because the key includes the name, events of the same device and type but
different `EventName` can be pending at the same time; only an identical event
is rejected as already pending. `--resolve` must be given the same `EventName`
that was used to generate.

## Adding a New Device

Create a new file in `devices/` with a `DeviceRegistration`:

```cpp
#include "utils/device_registry.hpp"

namespace event_emulator
{

static DeviceRegistration myDeviceRegistration("mydevice", [] {
    return DeviceEventData{
        .sensorName = "MY_SENSOR",
        // ... other leaf names ...
        .supportedEvents = {"reading-critical", "fan-failure"},
    };
});

} // namespace event_emulator
```

Add the file to `meson.build` and rebuild. No other changes needed.

## Adding a New Event Type

1. If the new event uses a name not already in `DeviceEventData`, add a new
   leaf-name field to the struct in `utils/device_events.hpp` and populate it
   in the device files under `devices/` that support it. The interface
   object-path prefix is owned by the generate/resolve function (see the
   `*Prefix` constants in `utils/device_events.cpp`), not the struct.

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
