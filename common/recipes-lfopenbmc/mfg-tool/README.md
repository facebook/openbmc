# mfg-tool

mfg-tool is a command-line tool specifically designed for manufacturing and testing purposes on OpenBMC platforms.<br><br>
 It provides various subcommands to perform different tasks, such as displaying sensor values, setting fan speeds, and manipulating fan modes, among others.<br><br>
This tool leverages the sdbusplus library and system APIs and libraries to interact with the D-Bus interface and modify behavior as needed.

## Usage

To run mfg-tool, log in to the BMC system and execute the following command:

```bash
$ mfg-tool <subcommand> [options]
```

To see the available subcommands and options, run the following command:

```bash
$ mfg-tool --help
```

## Subcommands

mfg-tool supports the following subcommands:

- `sensor-display`: Displays the sensor values of the BMC system.
- `power-state`: Shows the current power state of the host system.
- `power-control`: Controls the power state of the host system.
- `log-resolve`: Resolves the log entries of the BMC system.
- `log-display`: Displays the log entries of the BMC system.
- `log-clear`: Clears the log entries of the BMC system.
- `inventory`: Shows the inventory information of the BMC system.
- `fan-speed`: Displays and adjust the fan speeds of the BMC system.
- `fan-mode`: Displays and manipulating the fan mode of the BMC system.
- `bmc-arch`: Shows the architecture of the BMC system.
- `bmc-state`: Displays the BMC's readiness state.

## 1. sensor-display

The `sensor-display` subcommand provides an overview of data from all sensors on the BMC system, including temperature, voltage, power, and more.  
For instance, when executed on YV4, it will present details about sensors linked to the Spider Board, Management Board, Medusa Board, Fan Board, Sentinel Dome, and Waluia Falls.  
It retrieves a wide range of sensor properties and presents them in a structured format.

### Usage

To use the `sensor-display` subcommand, execute the following command:

```bash
$ mfg-tool sensor-display
```

### Output

The output will resemble the following:

```bash
{
    "CPU": {
        "critical": {
            "high": 90.0
        },
        "max": 100.0,
        "min": 0.0,
        "status": "ok",
        "unit": "Percent",
        "value": 15.872707100551539,
        "warning": {
            "high": 80.0
        }
    },
    "FANBOARD0_ADC_3V3_STBY_VOLT_V": {
        "critical": {
            "high": 3.432,
            "low": 3.168
        },
        "hard-shutdown": {
            "high": 3.83,
            "low": 2.64
        },
        "max": 255.0,
        "min": 0.0,
        "status": "ok",
        "unit": "Volts",
        "value": 3.31,
        "warning": {
            "high": 3.399,
            "low": 3.201
        }
    },
    "MB_ADC_P12V_DIMM_0_VOLT_V_38_40": {
        "critical": {
            "high": 14.214,
            "low": 9.894
        },
        "max": 13.8,
        "min": 10.200000000000001,
        "status": "ok",
        "unit": "Volts",
        "value": 11.939,
        "warning": {
            "high": 14.076,
            "low": 9.996
        }
    },
    "MEDUSA_12VDELTA3_CURR_A": {
        "critical": {
            "high": 116.919
        },
        "hard-shutdown": {
            "high": 130.0
        },
        "max": 255.0,
        "min": 0.0,
        "status": "ok",
        "unit": "Amperes",
        "value": -1.0,
        "warning": {
            "high": 111.6045
        }
    },
    "MGNT_P5V_VOLT_V": {
        "critical": {
            "high": 5.55,
            "low": 4.45
        },
        "max": 6.624954120108078,
        "min": 0.0,
        "status": "ok",
        "unit": "Volts",
        "value": 5.0939,
        "warning": {
            "high": 5.5,
            "low": 4.5
        }
    },
    "Memory": {
        "critical": {
            "high": 85.0
        },
        "max": 100.0,
        "min": 0.0,
        "status": "ok",
        "unit": "Percent",
        "value": 50.70193584984709,
        "warning": {
            "high": 70.0
        }
    },
    "SPIDER_INA233_12V_NIC0_PWR_W": {
        "critical": {
            "high": 56.375
        },
        "hard-shutdown": {
            "high": 67.5
        },
        "max": 3000.0,
        "min": 0.0,
        "status": "ok",
        "unit": "Watts",
        "value": 0.0,
        "warning": {
            "high": 53.8125
        }
    },
    "WF_INA233_P12V_E1S_0_L_CURR_A_65_42": {
        "critical": {
            "high": 0.0,
            "low": 0.0
        },
        "max": 0.0,
        "min": 0.0,
        "status": "ok",
        "unit": "Amperes",
        "value": 0.0,
        "warning": {
            "high": 0.0,
            "low": 0.0
        }
    },
    ...
    }
}
```

It includes the following information for each sensor:

- The name of the sensor.  
- `hard-shutdown`, `critical`, and `warning`: These fields contain the high and low thresholds for the corresponding sensor states. If the sensor reading crosses a threshold, the sensor status is updated accordingly.
- `max` and `min`: The maximum and minimum values that the sensor can measure.
- `status`: The status of the sensor, which can be “ok”, “warning”, “critical”, or “unavailable”.
- `unit`: The unit of measurement for the sensor reading.
- `value`: The current reading of the sensor.



## 2. power-state

The `power-state` subcommand displays the current power state of both the host system and the chassis.   
It checks the system’s power status and reports the operational state of the host and the power state of the chassis.

### Usage

To use the `power-state` subcommand, execute the following command:

```bash
$ mfg-tool power-state
```

### Output

The output will resemable the following:

```bash
{
    "0": {
        "standby": "off"
    },
    "1": {
        "runtime": "transition-on",
        "standby": "off"
    },
    "2": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "3": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "4": {
        "runtime": "on",
        "standby": "on"
    },
    "5": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "6": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "7": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "8": {
        "runtime": "transition-on",
        "standby": "transition-on"
    }
}
```

It includes the following information:

- The number of the device: : This corresponds to the slot numbers 1-8.    
- `runtime`: The operational state of the host system. This indicates whether the host system is running (“on”), off (“off”), or transitioning to these states (“transition-on” or "transition-off").  
- `standby`: The power state of the chassis. This reflects whether the power of the physical server enclosure is on("on"), off("off'), or transitioning to these states (“transition-on” or transition-off").  
 
 
## 3. power-control

The `power-control` subcommand manipulates the power state of both the host system and the chassis.  
It enables the user to specify an action and a scope to perform the action on.

### Usage

To use the `power-control` subcommand, execute the following command:

```bash
$ mfg-tool power-control -p <position> -a <action> -s <scope>
```

### Options

- `-p, --position <position>`: The position of the device. This is a required parameter.  
- `-a, --action <action>`: The control action (e.g., on, off, or cycle). This is a required parameter.  
- `-s, --scope <scope>`: The scope of the action (e.g., runtime, standby, or acpi). This is a required parameter.   

### Example

**(1) Host powers on**

#### Check current power state

- Command

```bash
$ mfg-tool power-state
```

- Output

```bash
{
    "0": {
        "standby": "transition-off"
    },
    "1": {
        "runtime": "off",
        "standby": "transition-on"
    },
    "2": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "3": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "4": {
        "runtime": "on",
        "standby": "on"
    },
    "5": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "6": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "7": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "8": {
        "runtime": "transition-on",
        "standby": "transition-on"
    }
}
```

#### Host power on in slot 1

- Command

```bash
$ mfg-tool power-control -p 1 -a on -s runtime
```

- Output

```bash
"success"
```

#### Check current power state

- Command

```bash
$ mfg-tool power-state
```

- Output

```bash
{
    "0": {
        "standby": "transition-off"
    },
    "1": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "2": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "3": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "4": {
        "runtime": "on",
        "standby": "on"
    },
    "5": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "6": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "7": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "8": {
        "runtime": "transition-on",
        "standby": "transition-on"
    }
}
```


**(2) Host powers off**

#### Check current power state

- Command

```bash
$ mfg-tool power-state
```

- Output

```bash
{
    "0": {
        "standby": "transition-off"
    },
    "1": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "2": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "3": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "4": {
        "runtime": "on",
        "standby": "on"
    },
    "5": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "6": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "7": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "8": {
        "runtime": "transition-on",
        "standby": "transition-on"
    }
}
```

#### Host power off in slot 4

- Command

```bash
$ mfg-tool power-control -p 4 -a off -s runtime
```

- Output

```bash
"success"
```

#### Check current power state

- Command

```bash
$ mfg-tool power-state
```

- Output

```bash
{
    "0": {
        "standby": "transition-off"
    },
    "1": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "2": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "3": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "4": {
        "runtime": "off",
        "standby": "on"
    },
    "5": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "6": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "7": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "8": {
        "runtime": "transition-on",
        "standby": "transition-on"
    }
}
```


**(3) Host powers cycle**


#### Check current power state

- Command 

```bash
$ mfg-tool power-state
```

- Output

```bash
{
    "0": {
        "standby": "transition-off"
    },
    "1": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "2": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "3": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "4": {
        "runtime": "on",
        "standby": "on"
    },
    "5": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "6": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "7": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "8": {
        "runtime": "transition-on",
        "standby": "transition-on"
    }
}
```

#### Host power cycle in slot 4

- Command

```bash
$ mfg-tool -p 4 -a cycle -s runtime
```

- Output

```bash
"success"

```

#### Check current power state

- Command

```bash
$ mfg-tool power-state
```

- Output

```bash
{
    "0": {
        "standby": "transition-off"
    },
    "1": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "2": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "3": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "4": {
        "runtime": "on",
        "standby": "on"
    },
    "5": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "6": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "7": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "8": {
        "runtime": "transition-on",
        "standby": "transition-on"
    }
}
```


**(4) Host power reset**

#### Check current power state

- Command 

```bash
$ mfg-tool power-state
```

- Output

```bash
{
    "0": {
        "standby": "off"
    },
    "1": {
        "runtime": "on",
        "standby": "on"
    },
    "2": {
        "runtime": "on",
        "standby": "on"
    },
    "3": {
        "runtime": "on",
        "standby": "on"
    },
    "4": {
        "runtime": "on",
        "standby": "on"
    },
    "5": {
        "runtime": "on",
        "standby": "on"
    },
    "6": {
        "runtime": "on",
        "standby": "on"
    },
    "7": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "8": {
        "runtime": "on",
        "standby": "on"
    }
}
```

#### Host power reset in slot 1

- Command

```bash
$ mfg-tool power-control -p 1 -a cycle -s acpi

```

- Output

```bash
"success"
```

#### Check current power state

- Command

```bash
$ mfg-tool power-state
```

- Output

```bash
{
    "0": {
        "standby": "off"
    },
    "1": {
        "runtime": "off",
        "standby": "on"
    },
    "2": {
        "runtime": "on",
        "standby": "on"
    },
    "3": {
        "runtime": "on",
        "standby": "on"
    },
    "4": {
        "runtime": "on",
        "standby": "on"
    },
    "5": {
        "runtime": "on",
        "standby": "on"
    },
    "6": {
        "runtime": "on",
        "standby": "on"
    },
    "7": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "8": {
        "runtime": "on",
        "standby": "on"
    }
}
```

#### Check current power state

- Command

```bash
$ mfg-tool power-state
```

- Output

```bash
{
    "0": {
        "standby": "off"
    },
    "1": {
        "runtime": "on",
        "standby": "on"
    },
    "2": {
        "runtime": "on",
        "standby": "on"
    },
    "3": {
        "runtime": "on",
        "standby": "on"
    },
    "4": {
        "runtime": "on",
        "standby": "on"
    },
    "5": {
        "runtime": "on",
        "standby": "on"
    },
    "6": {
        "runtime": "on",
        "standby": "on"
    },
    "7": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "8": {
        "runtime": "on",
        "standby": "on"
    }
}
```


**(5) 12V powers on**

#### Check current power state

- Command

```bash
$ mfg-tool power-state
```

- Output

```bash
{
    "0": {
        "standby": "off"
    },
    "1": {
        "runtime": "off",
        "standby": "off"
    },
    "2": {
        "runtime": "transition-on",
        "standby": "off"
    },
    "3": {
        "runtime": "off",
        "standby": "off"
    },
    "4": {
        "runtime": "on",
        "standby": "on"
    },
    "5": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "6": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "7": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "8": {
        "runtime": "transition-on",
        "standby": "transition-on"
    }
}
```

#### Chassis power on in slot 2

- Command

```bash
$ mfg-tool power-control -p 2 -a on -s standby
```

- Output

```bash
"success"
```

#### Check current power state

- Command

```bash
$ mfg-tool power-state
```

- Output

```bash
{
    "0": {
        "standby": "off"
    },
    "1": {
        "runtime": "off",
        "standby": "off"
    },
    "2": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "3": {
        "runtime": "off",
        "standby": "off"
    },
    "4": {
        "runtime": "on",
        "standby": "on"
    },
    "5": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "6": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "7": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "8": {
        "runtime": "transition-on",
        "standby": "transition-on"
    }
}
```


**(6) 12V powers off**

#### Check current power state

- Command

```bash
$ mfg-tool power-state
```

- Output:

```bash
{
    "0": {
        "standby": "off"
    },
    "1": {
        "runtime": "off",
        "standby": "off"
    },
    "2": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "3": {
        "runtime": "off",
        "standby": "off"
    },
    "4": {
        "runtime": "on",
        "standby": "on"
    },
    "5": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "6": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "7": {
        "runtime": "transition-on",
        "standby": "transition-on"
    },
    "8": {
        "runtime": "transition-on",
        "standby": "transition-on"
    }
}
```

#### Chassis power off in slot 4

- Command

```bash
$ mfg-tool power-control -p 4 -a off -c standby
```

- Output

```bash
"success"
```

#### Check current power state

- Command

```bash
$ mfg-tool power-state
```
- Output

***TBD***


**(7) 12V powers cycle**

#### Check current power state

- Command

```bash
$ mfg-tool power-state
```

***TBD***


**(8) Sled cycle**


- Command

```bash
$ mfg-tool power-control -p 0 -a cycle -s standby
```



## 4. log-resolve

The `log-resolve` subcommand resolves a specified log entry.  
It enables the user to specify a log entry ID, and it will mark that log entry as resolved.

### Usage

To use the `log-resolve` subcommand, execute the following command:

```bash
$ mfg-tool log-resolve -i <id>
```

### Options

- `-i, --id <id>`: The ID of the log entry. This is a required parameter.

### Note 

After resolving the log, you can verify the resolution by using the following command:

```bash
$ mfg-tool log-display
```

In the output, the `resolved` field for the corresponding log entry should now be `true`.

### Example

Check current log entries

- Command

```bash
$ mfg-tool log-display
```

- Output

``` bash
{
    "1": {
        "additional_data": [
            "NUM_LOGS=1"
        ],
        "event_id": "",
        "message": "xyz.openbmc_project.Logging.Error.LogsCleared",
        "resolution": "",
        "resolved": true,
        "severity": "xyz.openbmc_project.Logging.Entry.Level.Informational",
        "timestamp": "2024-02-21T05:17:52.106000000Z",
        "updated_timestamp": "2024-02-21T05:23:13.772000000Z"
    },
    "125": {
        "additional_data": [
            "DIRECTION=low",
            "EVENT=critical low",
            "READING=0.000000",
            "SENSOR_PATH=/xyz/openbmc_project/sensors/voltage/SPIDER_INA233_12V_NIC3_VOLT_V",
            "THRESHOLD=11.500000"
        ],
        "event_id": "",
        "message": "SPIDER_INA233_12V_NIC3_VOLT_V critical low threshold assert. Reading=0.000000 Threshold=11.500000.",
        "resolution": "",
        "resolved": false,
        "severity": "xyz.openbmc_project.Logging.Entry.Level.Critical",
        "timestamp": "2024-02-21T05:57:05.637000000Z",
        "updated_timestamp": "2024-02-21T05:57:05.637000000Z"
    }
}
```

Resolve the log entry with ID 125

- Command

```bash
$ mfg-tool log-resolve -i 125
```

- Output

```bash
"success"
```

Check current log entries

- Command

```bash
$ mfg-tool log-display
```

- Output

```bash
{
    "1": {
        "additional_data": [
            "NUM_LOGS=1"
        ],
        "event_id": "",
        "message": "xyz.openbmc_project.Logging.Error.LogsCleared",
        "resolution": "",
        "resolved": true,
        "severity": "xyz.openbmc_project.Logging.Entry.Level.Informational",
        "timestamp": "2024-02-21T05:17:52.106000000Z",
        "updated_timestamp": "2024-02-21T05:23:13.772000000Z"
    },
    "125": {
        "additional_data": [
            "DIRECTION=low",
            "EVENT=critical low",
            "READING=0.000000",
            "SENSOR_PATH=/xyz/openbmc_project/sensors/voltage/SPIDER_INA233_12V_NIC3_VOLT_V",
            "THRESHOLD=11.500000"
        ],
        "event_id": "",
        "message": "SPIDER_INA233_12V_NIC3_VOLT_V critical low threshold assert. Reading=0.000000 Threshold=11.500000.",
        "resolution": "",
        "resolved": true,
        "severity": "xyz.openbmc_project.Logging.Entry.Level.Critical",
        "timestamp": "2024-02-21T05:57:05.637000000Z",
        "updated_timestamp": "2024-02-21T05:57:05.637000000Z"
    }
}
```



## 5. log-display

The `log-display` subcommand displays log entries.  
It enables the user to view all log entries or only those that are unresolved.

### Usage

To use the `log-display` subcommand, execute the following command:

```bash
$ mfg-tool log-display [-u]
```

### Options

- `-u, --unresolved-only`: This flag shows only unresolved log entries. This is an optional flag.

### Output

The output will resemble the following:

```bash
{
    "1": {
        "additional_data": [
            "NUM_LOGS=1"
        ],
        "event_id": "",
        "message": "xyz.openbmc_project.Logging.Error.LogsCleared",
        "resolution": "",
        "resolved": true,
        "severity": "xyz.openbmc_project.Logging.Entry.Level.Informational",
        "timestamp": "2024-02-21T05:17:52.106000000Z",
        "updated_timestamp": "2024-02-21T05:23:13.772000000Z"
    },
    "125": {
        "additional_data": [
            "DIRECTION=low",
            "EVENT=critical low",
            "READING=0.000000",
            "SENSOR_PATH=/xyz/openbmc_project/sensors/voltage/SPIDER_INA233_12V_NIC3_VOLT_V",
            "THRESHOLD=11.500000"
        ],
        "event_id": "",
        "message": "SPIDER_INA233_12V_NIC3_VOLT_V critical low threshold assert. Reading=0.000000 Threshold=11.500000.",
        "resolution": "",
        "resolved": false,
        "severity": "xyz.openbmc_project.Logging.Entry.Level.Critical",
        "timestamp": "2024-02-21T05:57:05.637000000Z",
        "updated_timestamp": "2024-02-21T05:57:05.637000000Z"
    }
}
```

It includes the following information for each log entry:

- The ID of the log entry.
- `additional data`: Any additional data associated with the log entry.
- `event id`: The event ID of the log entry.
- `message`: The message of the log entry.
- `resolution`: The resolution of the log entry.
- `resolved`: A boolean indicating whether the log entry has been resolved.
- `severity`: The severity level of the log entry.
- `timestamp`: The timestamp when the log entry was created.
- `updated timestamp`: The timestamp when the log entry was last updated.



## 6. log-clear

The `log-clear` subcommand clears log entries.  
It removes all log entries from the BMC system.

### Usage

To use the `log-clear` subcommand, execute the following command:

```bash
$ mfg-tool log-clear
```

### Output

The output will resemble the following:

```bash
"success"
```

### Example

Check current log entries

- Command 

```bash
$ mfg-tool log-display
```

- Output 

```bash
{
    "1": {
        "additional_data": [
            "NUM_LOGS=1"
        ],
        "event_id": "",
        "message": "xyz.openbmc_project.Logging.Error.LogsCleared",
        "resolution": "",
        "resolved": true,
        "severity": "xyz.openbmc_project.Logging.Entry.Level.Informational",
        "timestamp": "2024-02-21T05:17:52.106000000Z",
        "updated_timestamp": "2024-02-21T05:23:13.772000000Z"
    },
    "125": {
        "additional_data": [
            "DIRECTION=low",
            "EVENT=critical low",
            "READING=0.000000",
            "SENSOR_PATH=/xyz/openbmc_project/sensors/voltage/SPIDER_INA233_12V_NIC3_VOLT_V",
            "THRESHOLD=11.500000"
        ],
        "event_id": "",
        "message": "SPIDER_INA233_12V_NIC3_VOLT_V critical low threshold assert. Reading=0.000000 Threshold=11.500000.",
        "resolution": "",
        "resolved": false,
        "severity": "xyz.openbmc_project.Logging.Entry.Level.Critical",
        "timestamp": "2024-02-21T05:57:05.637000000Z",
        "updated_timestamp": "2024-02-21T05:57:05.637000000Z"
    }
}
```

Clear all log entries

- Command

```bash
$ mfg-tool log-clear
```

- Output: 

```bash
$ "success"
```

Check current log entries

- Command

```bash
$ mfg-tool log-dsiplay
```

- Output

```bash
{
    "1": {
        "additional_data": [
            "NUM_LOGS=1"
        ],
        "event_id": "",
        "message": "xyz.openbmc_project.Logging.Error.LogsCleared",
        "resolution": "",
        "resolved": false,
        "severity": "xyz.openbmc_project.Logging.Entry.Level.Informational",
        "timestamp": "2024-03-04T06:29:28.077000000Z",
        "updated_timestamp": "2024-03-04T06:29:28.077000000Z"
    }
}
```



## 7. inventory

The `inventory` subcommand retrieves inventory information.  
It enables the user to query the system’s inventory data and returns a variety of properties for each inventory item.

### Usage

To use the `inventory` subcommand, execute the following command:

```bash
$ mfg-tool inventory
```

### Output

The output will resemble the following:

```bash
{
    "system/board/BMC_Storage_Module": {
        "Decorator.Asset": {
            "Manufacturer": "Wiwynn",
            "Model": "Yosemite V4",
            "PartNumber": "N/A",
            "SerialNumber": "N/A"
        },
        "Item.Board": {
            "Name": "BMC Storage Module",
            "Type": "Board"
        }
    },
    "system/board/Yosemite_4_FAN_Board_0": {
        "Decorator.Asset": {
            "Manufacturer": "Wiwynn",
            "Model": "Yosemite V4 EVT",
            "PartNumber": "N/A",
            "SerialNumber": "N/A"
        },
        "Item.Board": {
            "Name": "Yosemite 4 FAN Board 0",
            "Type": "Board"
        }
    },
    "system/board/Yosemite_4_Floating_Falls_Slot_4": {
        "Decorator.Asset": {
            "Manufacturer": "Wiwynn",
            "Model": "Yosemite V4 EVT",
            "PartNumber": "N/A",
            "SerialNumber": "N/A"
        },
        "Decorator.Slot": {
            "SlotNumber": false
        },
        "Item.Board": {
            "Name": "Yosemite 4 Floating Falls Slot 4",
            "Type": "Board"
        }
    },
    "system/board/Yosemite_4_Management_Board": {
        "Decorator.Asset": {
            "Manufacturer": "Wiwynn",
            "Model": "Yosemite V4 EVT",
            "PartNumber": "N/A",
            "SerialNumber": "N/A"
        },
        "Item.Board": {
            "Name": "Yosemite 4 Management Board",
            "Type": "Board"
        }
    },
    "system/board/Yosemite_4_Medusa_Board": {
        "Decorator.Asset": {
            "Manufacturer": "Wiwynn",
            "Model": "Yosemite V4 EVT",
            "PartNumber": "N/A",
            "SerialNumber": "N/A"
        },
        "Item.Board": {
            "Name": "Yosemite 4 Medusa Board",
            "Type": "Board"
        }
    },
    "system/board/Yosemite_4_Sentinel_Dome_Slot_4": {
        "Decorator.Asset": {
            "Manufacturer": "Wiwynn",
            "Model": "Yosemite V4 EVT",
            "PartNumber": "N/A",
            "SerialNumber": "N/A"
        },
        "Decorator.Slot": {
            "SlotNumber": false
        },
        "Item.Board": {
            "Name": "Yosemite 4 Sentinel Dome Slot 4",
            "Type": "Board"
        }
    },
    "system/board/Yosemite_4_Spider_Board": {
        "Decorator.Asset": {
            "Manufacturer": "Wiwynn",
            "Model": "Yosemite V4 EVT",
            "PartNumber": "N/A",
            "SerialNumber": "N/A"
        },
        "Item.Board": {
            "Name": "Yosemite 4 Spider Board",
            "Type": "Board"
        }
    },
    "system/board/Yosemite_4_Wailua_Falls_Slot_4": {
        "Decorator.Asset": {
            "Manufacturer": "Wiwynn",
            "Model": "Yosemite V4 EVT",
            "PartNumber": "N/A",
            "SerialNumber": "N/A"
        },
        "Decorator.Slot": {
            "SlotNumber": false
        },
        "Item.Board": {
            "Name": "Yosemite 4 Wailua Falls Slot 4",
            "Type": "Board"
        }
    }
}
```

It includes the following information for each inventory item:

- The path of the inventory item, stripped of the common prefix.  
- The interface of the inventory item, stripped of the common prefix.  
- `Manufacturer`: The manufacturer of the item.  
- `Model`: The model of the item.  
- `PartNumber`: The part number (if available).  
- `SerialNumber`: The serial number (if available).  
- `Slot Number` (if applicable): The slot number associated with the inventory item.  
- `Name`: The name of the board.  
- `Type`: The type of the board (e.g., “Board”).  



## 8. fan-speed

The `fan-speed` subcommand displays and adjusts the fan speed on the BMC system.  
It enables the user to specify a target Pulse Width Modulation (PWM) percentage and a fan position to set the fan speed.

### Usage

To use the `fan-speed` subcommand, execute the following command:

```bash
$ mfg-tool fan-speed [-t <target>] [-p <position>]
```

### Options

- `-t, --target <target>`: The target PWM percentage(e.g., 50.0). This is an optional parameter.  
	- If a target PWM percentage is specified, the tool will attempt to set the fan speed to the target value.  
	- If no target is specified, the tool will simply display the current fan speed.  
- `-p, --position <position>`: The position of the fan(e.g., FANBOARD0_FAN0). This is an optional parameter.  
	- If the position parameter is specified, the tool will only affect or display information for the fan at that position.  
	- If the position parameter is not specified, the tool will affect or display information for all fans.

### Output

If no parameters are provided, the output will resemble the following:

```bash
{
    "FANBOARD1_FAN0": {
        "pwm": {
            "current": 100.0,
            "target": 100.0
        },
        "tach_il": 15603.0,
        "tach_ol": 13107.0
    },
    "FANBOARD1_FAN1": {
        "pwm": {
            "current": 100.0,
            "target": 100.0
        },
        "tach_il": 15603.0,
        "tach_ol": 13284.0
    },
    "FANBOARD1_FAN2": {
        "pwm": {
            "current": 100.0,
            "target": 100.0
        },
        "tach_il": 15603.0,
        "tach_ol": 13284.0
    },
    "FANBOARD1_FAN3": {
        "pwm": {
            "current": 100.0,
            "target": 100.0
        },
        "tach_il": 15603.0,
        "tach_ol": 13284.0
    },
    "FANBOARD1_FAN4": {
        "pwm": {
            "current": 100.0,
            "target": 100.0
        },
        "tach_il": 15603.0,
        "tach_ol": 13466.0
    },
    "FANBOARD1_FAN5": {
        "pwm": {
            "current": 100.0,
            "target": 100.0
        },
        "tach_il": 15603.0,
        "tach_ol": 13466.0
    }
}
```

It includes the following information for each fan:

- The position of the fan.
- `pwm`: The current and target PWM value of the fan, which indicates the fan speed.
- `tach_il`: The current tachometer reading from the inside rotor of the fan, which indicates the fan’s RPM (Revolutions Per Minute).
- `tach_ol`: The current tachometer reading from the outside rotor of the fan, which indicates the fan’s RPM (Revolutions Per Minute).

### Example

Check the current fan speed of FANBOARD1_FAN0

- Command

```bash
$ mfg-tool fan-speed -p FANBOARD1_FAN0
```

- Output

```bash
{
    "FANBOARD1_FAN0": {
        "pwm": {
            "current": 100.0,
            "target": 100.0
        },
        "tach_il": 15603.0,
        "tach_ol": 13107.0
    }
}
```

Set FANBOARD1_FAN0's PWM to 50%

- Command

```bash
$ mfg-tool fan-speed -t 50.0 -p FANBOARD1_FAN0
```

Check the current fan speed of FANBOARD1_FAN0

- Command

```bash
$ mfg-tool fan-speed -p FANBOARD1_FAN0
```

- Output

```bash
{
    "FANBOARD1_FAN0": {
        "pwm": {
            "current": 49.9999997,
            "target": 49.9999997
        },
        "tach_il": 7802.0,
        "tach_ol": 6554.0
    }
}
```



## 9. fan-mode

The `fan-mode` subcommand sets the fan mode to either manual or automatic.  
- In manual mode, the fan speed can be controlled directly by the user.  
- In automatic mode, the fan speed is controlled by the system based on the current temperature and other factors.

### Usage

To use the `fan-mode` subcommand, execute the following command:

```bash
$ mfg-tool fan-mode [-m] [-a]
```

### Options

- `-m, --manual`: This flag sets the mode to manual. This is an optional flag.
- `-a, --auto`:  This flag sets the mode to automatic. This is an optional flag.

### Output

The output will resemble the following:

If the mode is set to manual
```bash
{
    "zone1": "manual"
}

```

If the mode is set to automatic
```bash
{
    "zone1": "auto"
}
```

### Note

Before setting the fan speed with the following command:

```bash
$ mfg-tool fan-speed [-t <target>] [-p <position>]
```

Ensure the fan mode is initially set to manual.  

Even when the fan mode is set to automatic, you can still override the fan speed.   
However, to switch back to automatic mode, you need to transition through manual mode first.  

This can be done using the following commands:
```bash
$ mfg-tool fan-mode -m
$ mfg-tool fan-mode -a
```

### Example

Check the current fan speed of FANBOARD1_FAN0

- Command

```bash
$ mfg-tool fan-speed -p FANBOARD1_FAN0
```

- Output

```bash
{
    "FANBOARD1_FAN0": {
        "pwm": {
            "current": 100.0,
            "target": 100.0
        },
        "tach_il": 15603.0,
        "tach_ol": 13107.0
    }
}
```

Switch fan mode to manual

- Command

```bash
$ mfg-tool fan-mode -m
```

- Output

```bash
{
    "zone1": "manual"
}
```

Set FANBOARD1_FAN0's PWM to 50%

- Command

```bash
$ mfg-tool fan-speed -t 50.0 -p FANBOARD1_FAN0
```

Check the current fan speed of FANBOARD1_FAN0

- Command

```bash
$ mfg-tool fan-speed -p FANBOARD1_FAN0
```

- Output

```bash
{
    "FANBOARD1_FAN0": {
        "pwm": {
            "current": 49.9999997,
            "target": 49.9999997
        },
        "tach_il": 7802.0,
        "tach_ol": 6554.0
    }
}
```

Switch fan mode to auto

- Command

``` bash
$ mfg-tool fan-mode -a
```

- Output

```bash
{
    "zone1": "auto"
}
```

Check the current fan speed of FANBOARD1_FAN0

- Command 

```bash
$ mfg-tool fan-speed -p FANBOARD1_FAN0
```

- Output

```bash
{
    "FANBOARD1_FAN0": {
        "pwm": {
            "current": 100.0,
            "target": 100.0
        },
        "tach_il": 15603.0,
        "tach_ol": 13107.0
    }
}
```



## 10. bmc-arch

The `bmc-arch` subcommand retrieves the architecture of the BMC (Baseboard Management Controller) on OpenBMC platforms.  
It enables the user to query the system’s architecture and returns the result.

### Usage

To use the `bmc-arch` subcommand, execute the following command:

```bash
$ mfg-tool bmc-arch
```

### Output

The output will resemble the following:

```bash
"arm"
```



## 11. bmc-state

The `bmc-state` subcommand retrieves and displays the BMC's readiness state.  
It ensures that the BMC is fully booted and prepared to execute commands.

### Usage

To use the `bmc-state` subcommand, execute the following command:

```bash
$ mfg-tool bmc-state
```

### Output

The output will resemble the following: 

If the BMC is fully operational and ready for commands

```bash
{
    "state": "ready"
}
```

If the BMC is in the process of booting up

```bash
{
    "state": "starting"
}
```

If the BMC is in a quiescent state

```bash
{
    "state": "quiesced"
}
```