# Introduction
## What is Rackmon

"rackmon" is a set of services and tools to monitor PSUs (Power Supply
Units) and BBUs (Battery Backup Units) and other special power related components
on Racks, and the services are usually deployed on RSWs such as Wedge40, Wedge100 and Wedge400
or a dedicated rack monitoring controller.

"rackmon" talks to discovered PSUs/BBUs using Modbus protocol over RS485. "rackmon"
sends commands to devices over one or more UARTs connected to the host controller.

## Why Rackmon V2?
Open Rack V3 brings with it a few changes to the design:
1. There can be more than 1 UART/RS485 connection each having 1 or more modbus devices.
2. There can be multiple types of modbus devices (Each with its own unique
   register map).
3. Possible number of devices were extended from 20 to 128. This means any simple
   scanning is going to be terrible for runtime performance.

V2 addresses all these changes:
* Rewrite in C++. This allows us to use more appropriate data structures like maps/vectors.
* Written in a way to allow for unit-testing each layer/component independently.
* Use JSON configuration files for interfaces (Allows for us to define the 1 or more UARTs)
* Use JSON configuration for register maps for each device type.
* register map also includes information on how to interpret the contents of a register
  (Better user visible output - support for rackmoninfo).
* Use JSON libraries instead of hand-weaving the JSON output.
* Use JSON as the primary means of encoding between CLI and daemon.
* Properly abstract the service layer to allow for anyone to replace it with something
  else if the UNIX socket layer+JSON is not appropriate (For example, Thrift, proto-buf etc).
  This allows for easy porting to other non-openbmc hosts.
* Split out probing and baudrate negotiation algorithms to configuration.
* Split monitoring, scan and service to their own threads. This allow for us to better
  manage responsibilities (Example, service and monitor have read only access to the devices
  structure while scan has write access -- use appropriate locking, shared locks for monitor
  and service while write locks for the rare probe event).

## Design choices
* Optimize for automation. Everything is JSON now. No one should need to parse CLI or binary output.
  It should be transparent to users of REST.
* Optimize for testability. Everything is abstracted to allow each component to be mocked from the other.
* Modularity. Everything is split into its own device. So, if I am porting to another platform, I probably
  don't have to rewrite everything! Just replace that module's implementation.
* Optimize for the more frequent event:
  - Raw requests on the service layer can go in parallel with the monitor loop. The only lock contended for
    is to access the UART layer. (Magic is with the use of shared mutexes).
  - Scan of all 128 possible devices is done once at BMC boot-up or after a pause+resume or a force-scan
    request (In contrast with V1 which scanned all possible devices every 2-3min which was fine when
    "all possible devices" == 20).
  - Compensate for lack of scanning by doing so one address at at time. Example, probe for 0xa4, then  2 min
    later probe for 0xa5. Rather than doing it all at one shot. This means we are paying a smaller penalty.
    Thus, adding a new device to the rack after rackmond has started would mean it would be discovered
    worst case, 128*2min (~4hrs). We could always ask for a force-scan after a repair.
  - Maintain devices which used to be active but in error in a different list. This list will be scanned
    every 2-3min. Thus, a service of a PSU would not incur the same 4hr penalty as adding a new device.


# Device/Interface Abstraction

The `Device` class provided by `Device.h` abstracts the OS from the rest of
the code. Since modbus requires exact read, the `read()` method reads
exact number of bytes rather than faithfully abstract the OS `read` call
which returns the number of bytes read. The methods may throw a `TimeoutException`
exception whenever something times-out. Otherwise, on errors it throws
either `std::system_error` or `std::runtime_error`. The caller is expected
to catch these.

Inheriting from `Device` we have `UARTDevice` in `UARTDevice.h` which implements all the
UART specific methods. In particular it has the concept of baudrate,
enabling/disabling read. It also overrides `wait_write` method to actually
poll the device on write completion.

Also in `UARTDevice.h` we have `AspeedRS485Device` which inherits from `UARTDevice`.
This is the abstraction for the native RS485 Device on the ASPEED Chip. With
special IOCTL needed at device open (Used by wedge100. Wedge400 canuses the FDTI
USB device and hence needs the default UARTDevice).

*Note, the write specialization is because we need to send commands with READ
disabled. Disabling READ during write has a special problem where if the
caller takes too long to re-enable READ we may miss the incoming response.
Hence, we need to muck around the thread priorities to get to enabling READ ASAP.
NOTE: If we dont do this, `wait_write` ends up busy-looping.
TODO: Check if we can get away with not doing a `wait_write` and disabling `READ`.*

All these inheritances are here to allow us to unit-test individual interfaces
and allow us to mock any layer.

# Modbus interface

`Modbus.h` implements the `Modbus` interface providing a request/response
type pattern over the UART path.
Since we have potentially multiple inheritance sources (`UARTDevice` or
`AspeedRS485Device` or more in the future), we are using composition instead
of inheritance and the device is created based on the configuration fed
by `Modbus::initialize(const json&)`

Unit tests can take advantage of this by fully testing everything above
by using a mock `UARTDevice`.

## Interface Configuration
The interfaces to be used is provided by `rackmond.json` of the format:
```
{
  "interfaces": [
    {
      "baudrate": 19200,
      "device_path": "/dev/ttyUSB0"
    },
    {
      "baudrate": 19200,
      "device_path": "/dev/ttyS4",
      "device_type": "AspeedRS485",
      "default_timeout": 300,
      "ignored_addrs": [32,33],
      "min_delay": 1,
    },
  ]
}
```
Mandatory:
  "baudrate": This is the default baudrate on this bus.
  "device_path": The UART Device path.
Optional:
  "device_type": {"default","AspeedRS485","LocalEcho"}, to pick the type of UART device.
      default: UART Device using an IOCTL to flush TX.
      AspeedRS485: The Aspeed RS485 subsystem.
      LocalEcho: UART Device with the TX tied to the RX for local echo (Transmitted bytes
      are available as RX to verify transmitted bytes)
  "default_timeout": default to use when use does not specify a timeout.
  "min_delay": Minimum delay to add after each command. 0 by default.
  "ignored_addrs": Do not scan for these addresses (Useful in debugging).

Once created, users primarily interact with the interface using the
`command(req,resp,baud,timeout)` method. The message structure
is discussed next.

# Messaging

`Msg.h` provides a native `Msg` structure which allows us to create packs/unpacks
very easily. It provides `<<` operator to allow one to encode fields and
`>>` operator to decode fields.
Currently only byte and nibble is supported since most of the work revolves
around those. We can add more in the future if future messages need it.
It correctly uses big-endian for members larger than a byte.

It exposes the reference to the first byte as `addr` for easy detection
of target address.

Creates virtual methods `encode` and `decode` which just does `finalize()`
(Adds a CRC16 checksum at the end of the message as per the Modbus spec)
and `validate()` which checks the CRC16 checksum at the end of the message
(after popping it.).

The `Modbus` class is expected to call the `encode()` method just before
writing a request and the `decode()` method right after receiving the response.

`Msg` is further specialized in `ModbusCmds.h` to create `ReadHoldingRegistersReq`
and `ReadHoldingRegistersResp`. Users can use this to read registers on
the modbus devices.

TODO (Future addition):
WriteHoldingRegister(req, resp) - Needed by baudrate negotiation.
WriteHoldingRegisters(req, resp) - Needed to update PSU timestamp.

# Register Maps
Register.h defines a register map which can be used to define the
registers supported by a modbus device of a given address range.
NOTE: This is roughly equivalent to what was previously done by
`rackmon-configure.py`.

There is one JSON file per device family.
Example configuration covering all the cases:
```
{
  "name": "orv2_psu",
  "address_range": [160, 191],
  "probe_register": 104,
  "default_baudrate": 19200,
  "preferred_baudrate": 19200,
  "registers": [
    {"begin": 0, "length": 8, "name": "MFG_MODEL", "format": "string"},
    {"begin": 152, "length": 1, "name": "RPM fan0", "keep": 10, "format": "integer"},
    {"begin": 153, "length": 1, "name": "RPM fan0", "keep": 10, "format": "integer", "endian": "L"},
    {"begin": 127, "length": 1, "name": "BBU Absolute State of Charge",
      "keep": 10, "format": "float", "precision": 6},
    {"begin": 104, "length": 1, "name": "PSU Status register", "keep": 10,
      "changes_only": true, "format": "flags",
      "flags": [
        [0, "Main Converter Fail"],
        [1, "Aux 54V Converter Fail"],
        [2, "Battery Charger Fail"],
        [3, "Current Feed (Boost Converter) Fail"],
        [4, "Temp Alarm   Alarm set at shutdown temp"],
        [5, "Fan Alarm"],
        [6, "Battery Alarm Set for BBU Fail or BBU voltage =< 26 VDC"],
        [7, "SoH Requested"],
        [8, "SoH Discharge"]
      ]
    }
  ]
}
```

"name": Name of configuration, unused by code mostly for humans.
"address_range": (Inclusive) range of address which use the defined register map.
"probe_register": Devices in this address range will respond successfully to this register read and can be used as a "probe" or discovery mechanism.
"default_baudrate": This is the default or starting baudrate of these devices.
"preferred_baudrate": We would prefer to negotiate this baudrate if possible.
"registers": List of register descriptors. Each descriptor contains:
  "begin": The starting address
  "length": Length in modbus-words (16bit words).
  "name": Name of the register.
  "format": How the register contents can be formatted. Possible values:
    "raw": Unformatted bytes.
    "string": ASCII string.
    "integer": Number
    "float": 2s complement fixed point number. Has an associated precision
    "flags": flags/bitmask where, each bit has a name provided by "flags"
  "flags": map of name to bit position (Valid when format is "flags").
  "precision": number of integer places (binary bbb.bbb). (Valid when format is "float")
  "endian": {"L" or "B" (Default)} specify how to interpret the binary value read from the register.
            (Modbus defines endian as Big-endian, but sometimes the devices return the value in
            little-endian especially if it is acting as a bridge to SMBus entities like the BBU).

There can be multiple register maps since we could potentially have
multiple types of devices. Currently planned types:
```
rackmon.d/orv2_psu.json
rackmon.d/orv3_psu.json
rackmon.d/orv3_bbu.json
rackmon.d/orv3_rpu.json
```

# ORv3 Register Address types
This more describes either up-coming or existing Register map JSONs.
We can formally interpret the address of a MODBUS device as:
```
|--7--|--6--|--5--|--4--|--3--|--2--|--1--|--0--|
| T1  | T0  | R2  | R1  | R0  | D2  | D1  | D0  |
|-----|-----|-----|-----|-----|-----|-----|-----|
```
| T1 | T0 | Description |
| -- | -- | ----------- |
| 1 | 1 | PSU (Power Supply Unit) |
| 0 | 1 | BBU (Battery Backup Unit) |
| 0 | 0 | Special/Miscellaneous Device |
| 1 | 0 | Combined PSU+BBU Device (ORv2) |


R2:R0 - Rack address
D2:D0 - Device address (PSU/BBU/PSU+BBU/Special).

ORv3 gets rid of the concept of a shelf address and moves
it into the device address to allow for greater flexibility.
Legacy ORv2 devices can be considered as special extensions:
| Bits | Interpretation |
| ---- | -------------- |
| R2   | Always 1       |
| R1:R0 | Rack Address  |
| D2   | Shelf Address  |
| D1:D0 | PSU Address   |

This information is used in constructing the "address_range" field
of the register map. That allows us to use different maps for
each of the device type.

# Modbus device interface
`ModbusDevice.h` defines a Modbus device. You construct it with
a Modbus interface it was discovered on, its address and finally its
register map described above.

It defines a `command()` method very similar to the `Modbus` class

To help with monitoring it defines a `monitor()` method which will
read all registers defined in the register map and store it in the 
local member of type `ModbusDeviceRawData`.

It exposes `get_raw_data` to help users retrieve a copy of the
monitored data and `is_flaky` and `last_active` to the monitor agent
to determine/remediate flaky devices.

# Monitoring
Rackmon.h defines the monitor. The meat of the daemon. This pretty
much ties everything defined till now together.
It has a `load()` method which takes in the path to the interface
configuration file and the directory path where the register map
configuration files are stored (Purposefully abstract to allow for
mocking in unit tests).
Has `start()` which startes the monitoring, and `stop()` which stops it.
`force_scan_all()` forces a scan of all the devices. It has `getMonitorData*`
methods to get the monitored data in a structured format.


# Service Interface
Currently there is only one service interface: The UNIX socket interface
to communicate with rackmond. This is mildly departing from V1 to use
JSON for all the messaging between the client and service while still
sticking to the older UNIX socket.
(A schema is also in the works to allow for better validation).

Since one of the users is rest-api, the V1 interface is kept behind
as a temporary measure while it migrates to the JSON API.

`rackmon_sock.cpp` defines all the common socket abstractions declared
in `RackmonSvcUnix.h` defines all the common interfaces between
the client and the service. `rackmon_svc_unix.cpp` implements the
service `main()`.

# CLI
Another departure from V1 is we have a single CLI for rackmon: `rackmoncli`.
Other than that, we are trying to maintain the same command line options
to the executable (changing the executable name). The output format is
not guaranteed to change (All automation has migrated to rest-api).
We can for the time being, provide shell abstractions to maintain the
`rackmon*` and `modbuscmd` commands.

# Not a fan of UNIX Sockets+JSON?
Everything related to the UNIX sockets are well contained in 4 sources:
```
RackmonSvcUnix.cpp
RackmonSvcUnix.h
RackmonCliUnix.cpp
RackmonSock.cpp
```
Just replace these with your own fancy one (thrift, protobuf, etc.) since
all the data should be coming from Rackmon class in `Rackmon.h`.
The additional work should solely be only for the interface to the external
world and the CLI.

The service layer is not abstracted in the hopes of reusing `main.cpp` so
feel free to do whatever you want.

# Profiling
rackmond can be recompiled with latency profiling enabled. This is done by:
```
RACKMON_PROFILING=1 BB_ENV_EXTRAWHITE="$BB_ENV_EXTRAWHITE RACKMON_PROFILING" bitbake rackmon
```
and copying over the generated IPK `tmp/deploy/ipk/armv6/rackmon_0.2-r1_armv6.ipk` to
the BMC and installing it:
```
opkg install --force_reinstall ./rackmon_0.2-r1_armv6.ipk
```
Then restarting it using the appropriate service manager (`sv restart rackmond` or `systemctl restart rackmond`).
You can retrieve the profile data from rackmond using:
```
rackmoncli profile
PROFILE modbus::182 : 46 ms
PROFILE rawcmd::182 : 46 ms
PROFILE modbus::165 : 46 ms
PROFILE rawcmd::165 : 46 ms
PROFILE modbus::165 : 42 ms
PROFILE rawcmd::165 : 42 ms
PROFILE modbus::164 : 39 ms
<snip>
```
this automatically clears the store. Note, if we do not retrieve the profile store, there
is nothing clearing up the memory, there is no upper-limit. So, don't use on production :-)

# Upcoming

* Baudrate negotiation
* Writing timestamp to the device.

# References
Protocol Specification: https://modbus.org/docs/Modbus_Application_Protocol_V1_1b.pdf
