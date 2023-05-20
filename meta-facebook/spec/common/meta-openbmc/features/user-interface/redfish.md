The primary system management interface shall be Redfish. Schemas shall conform
to the latest published Redfish standards and OEM schemas should only be used in
approved cases.

The Redfish implementation shall also conform to the latest published OCP schema
requirements.

The Redfish Validator must be executed and meet Redfish requirements for all
system features.

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
