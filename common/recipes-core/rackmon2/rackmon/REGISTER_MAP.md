# Rack Monitor Register Map

## Introduction

Rackmon consumes "register maps" which describes in detail on how
to interpret each register of the target device and how to identify
the target device.

Rackmon uses the device address to identify the type of device. This
is very similar to I2C devices which have assigned address ranges.

Most of the software for Register/Register Maps are in Register.[h.cpp].

## JSON Configuration

There is one JSON file per device family.
Example configuration covering all the cases:
```
{
  "name": "orv2_psu",
  "address_range": [[160, 191]],
  "probe_register": 104,
  "default_baudrate": 19200,
  "preferred_baudrate": 19200,
  "registers": []
}
```

| Key | Description |
| --- | ----------- |
| name | The human readable unique name for the device family |
| address_range | The range of addresses associated with the device family (inclusive range). Example, [[1,3],[6,7]] would imply that devices 1,2,3,6,7 would be associated with this map |
| probe_register | Address of a register of length 1 which can always be read irrespective of device status and will be used to determine the presence of device (discovery) |
| default_baudrate | Default baud-rate used for the device during discovery |
| preferred_baudrate | (Optional) Operating baud-rate which rackmon should negotiate to (See [Baud-rate Negotiation](#baud-rate-negotiation)) |
| baud_config | (Optional) See [Baud-rate Negotiation](#baud-rate-negotiation) |
| special_handlers | (Optional) See [Special Handling](#special-handling) |
| registers | List of register descriptors - See [Register Descriptor](#register-descriptor-json) |

## Register Format Types

Each register can have one of the following formats:

| Format Name | Format Description |
| ----------- | ------------------ |
| RAW | Unformatted bytes. Not recommended (debug-only) |
| STRING | ASCII formatted string |
| INTEGER | Numeric value which can be represented fairly as int32_t |
| LONG | Long numeric value which can be represented fairly as int64_t |
| FLOAT | 2s complement fixed point number with an associated precision. |
| FLAGS | Bit-mask where each bit position in the register have a significance. Bits are counted from the lowest bit when bytes are ordered as Big-endian |

## Register Endian Types

By default modbus specifies all registers to be big-endian. But some times, the devices returns the value
in little-endian. This can be specified in the "endian" field.
The field is optional for formats: LONG, INTEGER, FLOAT and defaults to "B".

| Endian Value | Endian Description |
| ------------ | ------------------ |
| B | Big endian (Default) |
| L | Little endian |

## Register Descriptor (CSV)

Registers can be described as a CSV for easy human consumption and management.

| Column Name | Column Description |
| ----------- | ------------------ |
| begin | Offset of the register |
| length | Number of modbus words part of the register (modbus word = 2 bytes). |
| keep | [Optional for all, default=1] Number of values to keep in the register history |
| changes_only | [Optional for all] If keep is specified and greater than 1, store new value only if different from the previously read value. Useful for flags |
| flags_bit | offset of the flag bit when viewed the combined registers in Big-endian format |
| flags_name | Name of the Flag bit. Preferred format is First_Name_Upper_Case_With_Underscore |
| format | See [Register Format Types](#register-format-types) |
| endian | See [Register Endian Types](#register-endian-types) |
| interval | [Optional for all] At what interval in seconds should the register be read (If unspecified, use rackmon internal default) |
| precision | [Mandatory for FLOAT] Number of bits reserved for fractional value |
| sign | [Optional for LONG, INTEGER, FLOAT, default=False] True/False (Default). If True, value can be negative |
| scale | [Optional for FLOAT, default=1.0] constant number to multiply the register float value before shifted: `final = READ * scale + shift` |
| shift | [Optional for FLOAT, default=0.0] constant number to be added to the register float value: `final = READ * scale + shift` |
| name | Name of the register. Preferred format is First_Name_Upper_Case_With_Underscore |

NOTE: For flags, after the row describing the register, the subsequent rows provide information for each bit utilizing the `flags_bit` and `flags_name` only.
Should be easier to see from an example:

| begin | length | keep | changes_only | flags_bit | flags_name | format | endian | interval | precision | sign | scale | shift | name |
| ----- | ------ | ---- | ------------ | --------- | ---------- | ------ | ------ | -------- | --------- | ---- | ----- | ----- | ---- |
| 0 | 4 | | | | | STRING | | | | | | | Manufacturer_Name |
| 10 | 1 | 4 | | | | INTEGER | L | 60 | | True | | | Drive_Voltage |
| 20 | 1 | | | | | FLOAT | | | 4 | | 0.1 | 10.4 | Drive_Current |
| 30 | 1 | 4 | True | | | FLAGS | | | | | | | Error_Flags |
| | | | | 0 | Thing_1_Failed | | | | | | | | |
| | | | | 1 | Thing_2_Failed | | | | | | | | |

## Register Descriptor (JSON)

See the previous section for a more detailed account for each of the keys. The only difference is how flags are specified.
They are specified as array of bit-number, bit-name pairs while in CSV they get their own row (Since humans don't fare well
consuming tables within tables).

```
{
  "registers": [
    {"begin": 0, "length": 4, "name": "Manufacturer_Name", "format": "STRING"},
    {"begin": 10, "length": 1, "name": "Drive_Voltage", "keep": 4, "format": "INTEGER", "endian": "L", "interval": 60, "sign": true},
    {"begin": 20, "length": 1, "name": "Drive_Current", "precision": 4, "scale": 0.1, "shift": 10.4},
    {"begin": 30, "length": 1, "name": "Error_Flags" "keep": 4, "changes_only": true, "format": "FLAGS",
      "flags": [
        [0, "Thing_1_Failed"],
        [1, "Thing_2_Failed"] 
      ]
    }
  ]
}
```

## Special Handling
Sometimes we want to perform certain "special" operations on registers
of all discovered modbus devices. For example, PSUs/BBUs do not have a
synchronized clock. Thus, it is beneficial if the controller "feeds" the
correct system time to the PSU, which would make blackbox/debug data
retrieved from the PSU dependable (useless if the time-stamp is inaccurate).

This is accomplished using the `special_handlers` key in the register map.
(This is not shown in the earlier section to maintain simplicity).

```
{
  "special_handlers": [
    {
      "reg": 298,
      "len": 2,
      "period": 3600,
      "action": "write",
      "info": {
        "interpret": "INTEGER",
        "shell": "date +%s"
      }
    }
  ]
}
```
| Field | Field Description |
| ----- | ----------------- |
| reg | Register address on which the action is performed |
| len | The length of data we are performing the operation with |
| period | If provided, the action is periodic. If not provided, the action is performed once at start up |
| action | action to take. Currently only "write" is supported. This allows a particular value to be written to the register |
| info | This determines what value is written. We need one of the two keys to be present: "shell", "value" |
| interpret | How to interpret the output (See [Register Format Types](#register-format-types)) |
| shell | Run a shell command and interpret its output (Example, date +%s) |
| value | Write the value provided |


## Baud-rate Negotiation

Some devices may support variable baud-rate. As in, they operate with a default
baud-rate, but allow users to change the operating baud-rate by writing to a
special register with a value indicative of the requested baud-rate.
`ModbusDevice` uses this information to switch to the `preferred_baudrate`
if it is not the same as `default_baudrate` as specified in the device's register map.

The register map can be extended to provide the required information by adding to it
a new key, `baud_config`:

```
    "baud_config": {
      "reg": 163,
      "baud_value_map": [
        [19200, 1],
        [38400, 2],
        [57600, 3],
        [115200, 4]
      ]
    },
```

| Config Key | Config Description |
| ---------- | ------------------ |
| reg | The register to write the configuration |
| baud_value_map | List of [baud-rate, value] pairs which maps the required baud-rate to the value needed to be written in the specified register |

Note: Future if needed, we may enhance this to specify the number of registers to be written.
But for now assume there is only one configuration register and we are writing a fixed integer
(not a flags register where we would require a read/modify/write).

## Converting From CSV to JSON

./scripts/regmap_format.py /path/to/file1.csv [/path/to/file2.csv] -o /path./to/file.json [--update]

In order to allow better sharing of register descriptions between multiple device types, we allow reading
from multiple inputs and generate a single output. Its assumed that the maps are non-overlapping.

The --update allows us to just update the "registers" field of an existing register map instead of overwriting
it with optional default garbage (Since the CSV does not specify other non-register things like name, address, etc)

## Converting from JSON to CSV

./scripts/regmap_format.py /path/to/file1.json -o /path/to/file1.csv

This is mostly for debug.
