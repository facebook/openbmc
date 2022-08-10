The OpenBMC image must be able to be updated fully while the BMC is running,
without reboot.  The preferred method for supporting this is to use a static
single `initramfs+rootfs` file-system layout.
