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
  - Maintain devices previously active but currently in error with a special "dormant" state.
    This allows us to scan for its recovery every 2-3min. Thus, a service of a PSU would not incur the same
    4hr penalty as adding a new device.


# Device/Interface Abstraction

The `Device` class provided by `Device.h` abstracts the OS from the rest of
the code. The device class reads opportunistically as much data as requested.
If no bytes are read within the requested time, a `TimeoutException` is raised.
Otherwise it returns the number of non-zero bytes read out of the device.
Any other error raises either a `std::system_error` or `std::runtime_error`.
The caller is expected to catch these.

Inheriting from `Device` we have `UARTDevice` in `UARTDevice.h` which implements all the
UART specific methods. In particular it has the concept of baud-rate,
enabling/disabling read.

Also in `UARTDevice.h` we have `AspeedRS485Device` which inherits from `UARTDevice`.
This is the abstraction for the native RS485 Device on the ASPEED Chip. With
special IOCTL needed at device open (Used by wedge100. Wedge400 uses the FDTI
USB device and hence needs the default `UARTDevice`).

Note, the write specialization is because we need to send commands with READ
disabled. Disabling READ during write has a special problem where if the
caller takes too long to re-enable READ we may miss the incoming response.
Hence, we need to muck around the thread priorities to get to enabling READ ASAP.
NOTE: If we don't do this, `wait_write` ends up busy-looping.

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
  "baudrate": This is the default baud-rate on this bus.
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
Currently only byte, nibble, 32bit-word (and vectors of these) are supported.
We can add more in the future if future messages need it.
It correctly uses big-endian for members larger than a byte.

It exposes the reference to the first byte as `addr` for easy detection
of target address.

Creates virtual methods `encode` and `decode` which just does `finalize()`
(Adds a CRC16 checksum at the end of the message as per the Modbus spec)
and `validate()` which checks the CRC16 checksum at the end of the message
(after popping it.).

The `Modbus` class is expected to call the `encode()` method just before
writing a request and the `decode()` method right after receiving the response.

`Msg` is further specialized in `ModbusCmds.h` to create `Request` and `Response`.
Then these are further specialized to implement Modbus messages:

| Operation | Request Message | Response Message |
|-----------|-----------------|------------------|
| Read Holding Registers | ReadHoldingRegistersReq | ReadHoldingRegistersResp |
| Write Single Register | WriteSingleRegisterReq | WriteSIngleRegisterResp |
| Write Multiple Registers | WriteMultipleRegistersReq | WriteMultipleRegistersResp |
| Read File Record(s) | ReadFileRecordReq | ReadFileRecordResp |

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

This information is used in constructing the `address_range` field
of the register map.

# Modbus device interface
`ModbusDevice.h` defines an addressable Modbus device. You construct it with
a Modbus interface it was discovered on, its address and finally its
register map described above.

It defines a `command()` method very similar to the `Modbus` class

To help with monitoring it defines a `reloadRegisters()` method which will
read all registers defined in the register map and store it in the 
local member of type `ModbusDeviceRawData`.

It exposes two APIs to retrieve these values:

1. `getValueData`: Returns the interpreted structured value (as defined in the register map)
2. `getRawData`: Returns the raw bytes (Backwards compatibility)

# Monitoring
Rackmon.h defines the monitor. The meat of the daemon. This pretty
much ties everything defined till now together.
It has a `load()` method which takes in the path to the interface
configuration file and the directory path where the register map
configuration files are stored (Purposefully abstract to allow for
mocking in unit tests).
Has `start()` which starts the monitoring, and `stop()` which stops it.
`forceScan()` forces a scan of all the devices. It has `getMonitorData*`
methods to get the monitored data in a structured format.


# Service Interface
Currently there is only one service interface: The UNIX socket interface
to communicate with rackmond. This is mildly departing from V1 to use
JSON for all the messaging between the client and service while still
sticking to the older UNIX socket.
(A schema is also in the works to allow for better validation).

Since one of the users is rest-api, the V1 interface is kept behind
as a temporary measure while it migrates to the JSON API.

`UnixSock.cpp` and `UnixSock.h` define all the common client/service
socket abstractions. `RackmonSvcUnix.cpp` implements the
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
UnixSock.cpp
```
Just replace these with your own fancy one (thrift, protobuf, etc.) since
all the data should be coming from Rackmon class in `Rackmon.h`.
The additional work should solely be only for the interface to the external
world and the CLI.

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
You can retrieve the profile data from syslog:
```
 2022 May 16 13:33:06 $HOST user.info $VERS: $SVC: PROFILE modbus::180 : 36 ms
 2022 May 16 13:33:06 $HOST user.info $VERS: $SVC: PROFILE rawcmd::180 : 36 ms
 2022 May 16 13:33:07 $HOST user.info $VERS: $SVC: PROFILE modbus::181 : 46 ms
 2022 May 16 13:33:07 $HOST user.info $VERS: $SVC: PROFILE rawcmd::181 : 46 ms
```
Note that syslog rotation might not be designed for this level of logging so there
is a high chance that we might take up a lot of RAM. So, don't use on production :-)

# References
Protocol Specification: https://modbus.org/docs/Modbus_Application_Protocol_V1_1b.pdf
