=========================
OpenRack Power Monitoring
=========================

"rackmon" is a set of services and tools to monitor PSUs (Power Supply
Units) and BBUs (Battery Backup Units) on Racks, and the services are
usually deployed on RSWs such as Wedge40, Wedge100 and Wedge400.

PSU/BBU Address Scheme
======================

"rackmon" talks to PSUs/BBUs using Modbus protocol over RS485. "rackmon"
sends commands to BMC UART4: all the connected PSUs/BBUs can receive the
commands but the one with matched address responds.

For details about PSU/BBU Addressing, please refer to:
https://fb.quip.com/SeTVAraq7iFa

More References
===============

- https://our.internmc.facebook.com/intern/wiki/OpenBMC/Runbook/Rackmon/
- https://opencompute.dozuki.com/c/OCP_Open_Rack_V2
