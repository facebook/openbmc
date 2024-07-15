The BMC must provide a way for a user to interact with each server/slot's serial
console via CLI obmc-console-client or an equivalent Redfish method.
```
obmc-console-client -i <host-identifier>
```
host-identifier is host1-hostN for multi-host server systems and host0 for single host server systems.
