# Ad-hoc Sensor Service

A streamlined OpenBMC service that provides numeric sensors from file contents using sdbusplus.

## Overview

This service creates D-Bus sensors that track numeric values by reading file contents.
Files are monitored via inotify for instant updates when values change.

The service **automatically monitors** two directories:
- `/run/openbmc/sensors/utilization/` - Numeric values (file contents), as sensors
  of any supported unit (the directory name predates units)
- `/run/openbmc/metrics/` - Numeric values, as OEM metrics (see below)

## Behavior

- Each file is a number on line 1 and an optional unit on line 2
- The unit picks the sensor type, its D-Bus namespace and its name suffix:

  | Line 2 | Namespace | Unit | Sensor name |
  |---|---|---|---|
  | *(none)* or `percent` | `utilization` | Percent | `<file>_UTIL_PCT` |
  | `celsius` | `temperature` | DegreesC | `<file>_TEMP_C` |
  | `watts` | `power` | Watts | `<file>_PWR_W` |
  | `amperes` | `current` | Amperes | `<file>_CURR_A` |
  | `volts` | `voltage` | Volts | `<file>_VOLT_V` |
  | `rpm` | `fan_tach` | RPMS | `<file>_SPEED_RPM` |
  | `cfm` | `airflow` | CFM | `<file>_AIRFLOW_CFM` |
  | `joules` | `energy` | Joules | `<file>_ENERGY_J` |
  | `pascals` | `pressure` | Pascals | `<file>_PRESSURE_PA` |

- The suffixes follow Meta's sensor naming convention; Central Proxy drops
  sensors whose names don't end in one.
- The suffix isn't added twice if the file name already ends with it.
- A real sensor always wins its path: if another service already owns the
  sensor path, the file is ignored, and if one publishes it later, the adhoc
  sensor is withdrawn. Both are logged. With two owners of one path, bmcweb
  fails requests for both. Delete the file and pick another name.
- Only percent values are clamped to 0-100 (with MinValue/MaxValue 0/100)
- An unknown unit is logged and the file ignored; changing the unit
  republishes the sensor under its new path
- File names must be `[A-Za-z0-9_]+`; use upper case to match the naming
  convention
- Invalid or unparseable values result in NaN (Not a Number)
- When the file is removed, the sensor is removed from D-Bus
- File changes are detected instantly via inotify (no polling delay)

## Example

```bash
# Percent (default)
echo "87" > /run/openbmc/sensors/utilization/CPU
# Creates /xyz/openbmc_project/sensors/utilization/CPU_UTIL_PCT = 87.0

# Other units
printf '45.5\ncelsius\n' > /run/openbmc/sensors/utilization/NIC0
# Creates /xyz/openbmc_project/sensors/temperature/NIC0_TEMP_C = 45.5
printf '12.1\nvolts\n' > /run/openbmc/sensors/utilization/P12V
# Creates /xyz/openbmc_project/sensors/voltage/P12V_VOLT_V = 12.1

# Percent values above 100 are clamped
echo "250" > /run/openbmc/sensors/utilization/CPU
# CPU_UTIL_PCT = 100.0

# Invalid values become NaN
echo "invalid" > /run/openbmc/sensors/utilization/TEST
# TEST_UTIL_PCT = NaN

# Remove sensor
rm /run/openbmc/sensors/utilization/CPU
```

## OEM metrics (not sensors)

The service also watches a second directory:

- `/run/openbmc/metrics/<name>` - a number on line 1, optional unit on line 2

Each file becomes an `xyz.openbmc_project.Metric.Value` object (the interface
phosphor-health-monitor uses) at `/xyz/openbmc_project/metric/bmc/oem/<name>`.
Unlike sensors there is no unit suffix, no 0-100 clamp and no chassis
association. The value is a double (the only type Metric.Value carries), so
flags and states are numbers. The unit (`bytes`, `count`, `frequency`,
`percent`, `seconds`; default `count`) becomes the Metric.Value `Unit`
property; it is const on D-Bus, so changing it republishes the object. `<name>` must be `[A-Za-z0-9_]+`, since it becomes a D-Bus path
element; other names are ignored and logged.

bmcweb reports every metric under that namespace in
`/redfish/v1/Managers/bmc/ManagerDiagnosticData`:

```json
"Oem": {
  "Meta": {
    "@odata.type": "#MetaManagerDiagnosticData.v1_0_0.ManagerDiagnosticData",
    "Metrics": {
      "persist_rofs": { "Value": 1.0, "Unit": "Count" },
      "free_kb": { "Value": 20080.0, "Unit": "Bytes" }
    }
  }
}
```

Use the `bmc-oem-metric` helper rather than writing the files directly; it
validates the name and value and renames the file into place atomically:

```bash
bmc-oem-metric set persist_rofs 1
bmc-oem-metric set free_kb 20080 bytes
bmc-oem-metric get persist_rofs
bmc-oem-metric list
bmc-oem-metric rm persist_rofs
```

## Chassis Association

Each sensor is associated with one inventory chassis via
`xyz.openbmc_project.Association.Definitions`, so bmcweb lists it under
`/redfish/v1/Chassis/<id>/Sensors`. The chassis is discovered at runtime:

- the `default-chassis` meson option (`CHASSIS_PATH` in a bbappend), if set
  and present as an `Item.Chassis` or `Item.Board`
- otherwise the `Item.Chassis` objects, or `Item.Board` if there are none,
  minus any with a `contained_by` association, first by path

Discovery reruns 2 seconds after inventory stops changing. Sensors created
before entity-manager publishes inventory get the association then.

## Files

- `adhoc-sensor.cpp` - Main C++ implementation
- `meson.build` - Meson build configuration
- `adhoc-sensor.service` - Systemd service definition
- `adhoc-sensor_0.1.bb` - Yocto/BitBake recipe

## D-Bus Interface

**Service Name:** `xyz.openbmc_project.AdhocSensor`

**Object Paths:** `/xyz/openbmc_project/sensors/<namespace>/<file><suffix>` (see Behavior)

**Interfaces:** `xyz.openbmc_project.Sensor.Value`, `xyz.openbmc_project.Association.Definitions`

**Properties:**
- `Value` (double) - Sensor value, or NaN
- `Unit` - from line 2 (see Behavior)
- `MaxValue`/`MinValue` (double) - 100.0/0.0 for percent, unbounded otherwise

## Usage Examples

### Basic Usage

```bash
# Create adhoc sensors for utilization metrics
echo "87" > /run/openbmc/sensors/utilization/cpu_utilization
echo "42" > /run/openbmc/sensors/utilization/memory_utilization

# Check via D-Bus
busctl get-property xyz.openbmc_project.AdhocSensor \
    /xyz/openbmc_project/sensors/utilization/cpu_utilization_UTIL_PCT \
    xyz.openbmc_project.Sensor.Value Value
# Output: d 87

# Update value (instantly detected via inotify)
echo "95" > /run/openbmc/sensors/utilization/cpu_utilization

# Check again
busctl get-property xyz.openbmc_project.AdhocSensor \
    /xyz/openbmc_project/sensors/utilization/cpu_utilization_UTIL_PCT \
    xyz.openbmc_project.Sensor.Value Value
# Output: d 95
```

## Redfish API Access

Sensors automatically appear in Redfish:

```bash
# List all sensors
curl -sk -u root:0penBmc \
  https://<BMC_IP>/redfish/v1/Chassis/<CHASSIS_NAME>/Sensors

# Get specific sensor
curl -sk -u root:0penBmc \
  https://<BMC_IP>/redfish/v1/Chassis/<CHASSIS_NAME>/Sensors/cpu_utilization_UTIL_PCT \
  | jq '{Name, Reading, ReadingType}'
```

## Customization

### Changing Directory

Edit `adhoc-sensor.cpp` and modify:

```cpp
constexpr const char* ADHOC_DIR = "/run/openbmc/sensors/utilization";
```

### Changing Chassis Association

Only needed if discovery picks the wrong chassis. In your platform's bbappend:

```bitbake
CHASSIS_PATH = "/xyz/openbmc_project/inventory/system/chassis/YourChassis"
```

## Building

```bash
# In your OpenBMC build environment
bitbake adhoc-sensor

# Or rebuild entire image
bitbake <your-platform>-image
```

## Integration Patterns

### Shell Script Integration

```bash
#!/bin/bash
# Update sensor from shell script

# Read temperature from hardware (millidegrees)
TEMP=$(cat /sys/class/hwmon/hwmon0/temp1_input)

# Update sensor: DEVICE_TEMP_C
printf '%s\ncelsius\n' "$((TEMP / 1000))" > /run/openbmc/sensors/utilization/DEVICE
```

### C/C++ Application Integration

```cpp
#include <fstream>
#include <string>

void updateSensor(const std::string& name, int value) {
    std::string path = "/run/openbmc/sensors/utilization/" + name;
    std::ofstream file(path);
    if (file.is_open()) {
        file << value;
    }
}

// Usage
updateSensor("cpu_utilization", 75);
```

### Systemd Service Integration

Create a systemd service that manages sensor files:

```ini
[Unit]
Description=Device Monitoring
After=adhoc-sensor.service

[Service]
Type=oneshot
ExecStart=/usr/bin/update-device-sensors.sh

[Install]
WantedBy=multi-user.target
```

### Systemd Timer for Periodic Updates

```ini
# /etc/systemd/system/device-monitor.timer
[Unit]
Description=Device Monitor Timer

[Timer]
OnBootSec=30s
OnUnitActiveSec=60s

[Install]
WantedBy=timers.target
```

## D-Bus Usage Examples

### List All Sensors

```bash
busctl tree xyz.openbmc_project.AdhocSensor
```

### Monitor Sensor Changes

```bash
busctl monitor xyz.openbmc_project.AdhocSensor
```

### Get Sensor Associations

```bash
busctl get-property xyz.openbmc_project.AdhocSensor \
    /xyz/openbmc_project/sensors/utilization/cpu_utilization_UTIL_PCT \
    xyz.openbmc_project.Association.Definitions Associations
```

## Dependencies

- `boost` - For async I/O
- `sdbusplus` - D-Bus C++ bindings
- `phosphor-dbus-interfaces` - OpenBMC D-Bus interfaces
- `phosphor-logging` - OpenBMC lg2 structured logging
- `systemd` - Service management

## Performance

- **Monitoring:** inotify-based (instant updates, no polling overhead)
- **File Operations:** Read on file change only
- **D-Bus Updates:** Only when value changes
- **Memory:** Minimal per sensor (~1KB)

## Troubleshooting

### Sensor Not Appearing

```bash
# Check service status
systemctl status adhoc-sensor

# Check if directory exists
ls -la /run/openbmc/sensors/utilization/

# Check D-Bus service
busctl list | grep AdhocSensor
```

### Sensor Shows NaN Value

```bash
# Verify file contents are numeric
cat /run/openbmc/sensors/utilization/my_sensor

# Check service logs for parse errors
journalctl -u adhoc-sensor -f
```

### Sensor Not Updating

```bash
# Check if inotify is working
# Create/modify a test file and check logs
echo "50" > /run/openbmc/sensors/utilization/test
journalctl -u adhoc-sensor -n 20

# Restart service if needed
systemctl restart adhoc-sensor
```

## License

Apache-2.0
