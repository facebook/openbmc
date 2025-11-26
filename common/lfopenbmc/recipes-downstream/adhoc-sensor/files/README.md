# Ad-hoc Sensor Service

A streamlined OpenBMC service that provides numeric sensors (0-100%) from file contents using sdbusplus.

## Overview

This service creates D-Bus sensors that track numeric values by reading file contents.
Files are monitored via inotify for instant updates when values change.

The service **automatically monitors** one directory:
- `/run/openbmc/sensors/utilization/` - Numeric values (file contents)

## Behavior

- When a file is created, a sensor is automatically created
- The sensor value is read from the file contents (expects numeric value 0-100)
- Values are clamped between 0 and 100
- Invalid or unparseable values result in NaN (Not a Number)
- When the file is removed, the sensor is removed from D-Bus
- File changes are detected instantly via inotify (no polling delay)

## Example

```bash
# Create adhoc sensor
echo "87" > /run/openbmc/sensors/utilization/cpu_utilization
# Creates sensor: cpu_utilization_PCT with value 87.0

# Update value
echo "42" > /run/openbmc/sensors/utilization/cpu_utilization
# Updates sensor: cpu_utilization_PCT to value 42.0 (instantly via inotify)

# Values above 100 are clamped
echo "250" > /run/openbmc/sensors/utilization/utilization
# Creates sensor: utilization_PCT with value 100.0 (clamped)

# Invalid values become NaN
echo "invalid" > /run/openbmc/sensors/utilization/test
# Creates sensor: test_PCT with value NaN

# Remove sensor
rm /run/openbmc/sensors/utilization/cpu_utilization
# Removes sensor from D-Bus entirely
```

## Use Cases

- Percentage-based metrics (utilization, capacity, throttling)
- Normalized counters (0-100 scale)
- Custom application metrics
- Hardware monitoring data

## Architecture

All sensors are implemented as **utilization/percentage sensors**:
- **Values:** 0.0 to 100.0 (clamped), or NaN for errors
- **Unit:** Percent
- **Interface:** `xyz.openbmc_project.Sensor.Value`

### Naming Convention

All sensors follow Meta standards with `_PCT` suffix:
- File: `cpu_util` → Sensor: `cpu_util_PCT`
- File: `fan_speed` → Sensor: `fan_speed_PCT`

### Chassis Association

Each sensor is automatically associated with the platform's configured chassis via the `xyz.openbmc_project.Association.Definitions` interface.

This enables:
- Automatic appearance in bmcweb/Redfish chassis sensor collections
- Proper sensor-to-chassis relationship tracking
- Standard OpenBMC sensor discovery

## Files

- `adhoc-sensor.cpp` - Main C++ implementation
- `meson.build` - Meson build configuration
- `adhoc-sensor.service` - Systemd service definition
- `adhoc-sensor_0.1.bb` - Yocto/BitBake recipe

## D-Bus Interface

**Service Name:** `xyz.openbmc_project.AdhocSensor`

**Object Paths:** `/xyz/openbmc_project/sensors/utilization/<sensor_name>_PCT`

**Interface:** `xyz.openbmc_project.Sensor.Value`

**Properties:**
- `Value` (double) - Sensor value (0.0 to 100.0, or NaN)
- `Unit` (string) - "xyz.openbmc_project.Sensor.Value.Unit.Percent"
- `MaxValue` (double) - 100.0
- `MinValue` (double) - 0.0

## Usage Examples

### Basic Usage

```bash
# Create adhoc sensors for utilization metrics
echo "87" > /run/openbmc/sensors/utilization/cpu_utilization
echo "42" > /run/openbmc/sensors/utilization/memory_utilization

# Check via D-Bus
busctl get-property xyz.openbmc_project.AdhocSensor \
    /xyz/openbmc_project/sensors/utilization/cpu_utilization_PCT \
    xyz.openbmc_project.Sensor.Value Value
# Output: d 87

# Update value (instantly detected via inotify)
echo "95" > /run/openbmc/sensors/utilization/cpu_utilization

# Check again
busctl get-property xyz.openbmc_project.AdhocSensor \
    /xyz/openbmc_project/sensors/utilization/cpu_utilization_PCT \
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
  https://<BMC_IP>/redfish/v1/Chassis/<CHASSIS_NAME>/Sensors/cpu_utilization_PCT \
  | jq '{Name, Reading, ReadingType}'
```

## Customization

### Changing Directory

Edit `adhoc-sensor.cpp` and modify:

```cpp
constexpr const char* ADHOC_DIR = "/run/openbmc/sensors/utilization";
```

### Changing Chassis Association

The chassis path is configured via meson option. Edit your platform's bbappend:

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

# Read temperature from hardware
TEMP=$(cat /sys/class/hwmon/hwmon0/temp1_input)

# Convert to percentage (0-100 scale)
# Assuming max temp is 100C
TEMP_PCT=$((TEMP / 1000))

# Update sensor
echo "$TEMP_PCT" > /run/openbmc/sensors/utilization/device_temp
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
    /xyz/openbmc_project/sensors/utilization/cpu_utilization_PCT \
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
