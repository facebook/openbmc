The system should use the same kernel version as the corresponding BMC vendor
support contained [upstream][lf-openbmc].  The device tree definition for the
machine should be upstreamed.

It is expected that all devices are specified in the device tree and/or
device tree overlays and not dynamically instantiated (such as through `bind`
sysfs calls), unless otherwise approved.

No kernel modules should be used or loaded unless approved.
