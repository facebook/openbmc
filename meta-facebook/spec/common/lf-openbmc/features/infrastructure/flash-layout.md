The flash should layout shall contain the following in a static layout on
redundant flash devices (unless specified otherwise):

- u-boot SPL
- u-boot environment
- image metadata
- u-boot
- kernel and initramfs FIT image
- persistent data store

The persistent data store should only be present on the non-Recovery flash
device.  The persistent data store should not be mounted if the BMC has booted
into the Recovery image.

The initramfs must contain the full rootfs image so that the BMC image can be
updated while running.
