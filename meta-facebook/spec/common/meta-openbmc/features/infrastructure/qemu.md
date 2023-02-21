A machine model should be contributed to [QEMU][qemu] for the BMC.  This
model shall, at a minimum, contain all devices present in the device tree for
which an existing QEMU device model exists.  The BMC image must boot on this
model to a `BMC Ready` state without QEMU-specific code patches.

[qemu]: https://github.com/qemu/qemu
