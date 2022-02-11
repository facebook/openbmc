# Firmware Utility

Firmware utility supports several command line options

- version
- update (force)
- dump
- scheduling

## Version

fw-util version will print the version of a component of a FRU. The command takes a FRU as an argument as well as a component found on that FRU.

## Update

fw-util Any component connected to the BMC can be updated using the fw-util tool. For a list of components upgradable by fw-util, execute with the help option. Update takes the FRU and it's component as argument as well as the path to a firmware image.
**NOTE: There is little to no checks in place when flashing firmware to a component. Nothing is stopping you from flashing the wrong firmware and bricking the device.**


## Dump

fw-util dump will dump the firmware of a component to the file specified by IMAGE_PATH.
**Note: Not all firmware/components is supported**

## Scheduling

### Schedule a Firmware Update

fw-util provides several mechanisms to help with scheduling firmware updates for later.

fw-util bmc --update bmc $image --schedule TIME allows a user to specify when a firmware update will take place.

For example

- Update the BMC image after 30 min : ` fw-util bmc --update bmc $image --schedule "now + 30 min"`
- Update the BMC image at 2:30 pm 10/21/2020 : ` fw-util bmc --update bmc $image --schedule "2:30 pm 10/21/2020"`

### List Schedules

`fw-util all --show-schedule` will list all the firmware update jobs queued for later.

### Delete Schedules

`fw-util all --delete-schedule JOB_NUM` will allow deleting future scheduled firmware updates where JOB_NUM is the number shown when running `--show-schedule.`
