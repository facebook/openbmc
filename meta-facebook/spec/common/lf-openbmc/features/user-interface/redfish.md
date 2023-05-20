The primary system management interface shall be Redfish and leveraging the
upstream [Redfish server][bmcweb]. Schemas shall conform to the latest published
Redfish standards and OEM schemas should only be used in approved cases.

The Redfish implementation shall also conform to the latest published OCP schema
requirements.

The minimum required endpoints are:

- `AccountService`
- `SessionService`
- `Chassis`
- `Chassis/.../Sensors`
- `Systems`
- `Systems/.../LogServices`
- `Managers/1/LogServices`
- `Managers/1/EthernetInterfaces`
- `Managers/1/NetworkProtocol`
- `Managers/1/Actions/Manager.Reset`
- `UpdateService`
- `UpdateService/FirmwareInventory`

[bmcweb]: https://github.com/openbmc/bmcweb
