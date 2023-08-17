The OpenBMC image must be able to be updated fully while the BMC is running,
without reboot.  The preferred method for supporting this is to use a static
single `initramfs+rootfs` file-system layout ("static norootfs").

The update should handle updating both flash banks independently and show the
versions for:

- Currently running image.
- Flash bank 0 (potentially pending).
- Flash bank 1 (potentially pending).
