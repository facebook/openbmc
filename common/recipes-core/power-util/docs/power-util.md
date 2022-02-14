# Power Control

power-util allows the user to get the status of and control the power of components connected to the BMC.

## Status

The status subcommand will return the current power status of the component given as an argument. Returning either ON or OFF.

## Power commands

The following subcommands are the basic power control features:
- on
- off
- cycle

**Note: When used in a multihost system (slot#) these commands will control the power of the host, not the server board itself.**

## 12v Subcommands

There are several subcommands involving the 12v rail power:
- 12V-on
- 12V-off
- 12V-cycle

These commands will effect the 12v power for a given slot and will remove power not only from the host, but all components present on the server board.

## Sled-Cycle

This command will power cycle the entire sled. This will disconnect your ssh session from the BMC.

## Graceful-Shutdown

Gracefully shuts down the server.

## Reset

This command will initiate a reset (different than cycle). It's function will be dependent on the platform initiating the command.
