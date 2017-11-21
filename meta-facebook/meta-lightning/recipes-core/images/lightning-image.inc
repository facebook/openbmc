inherit kernel_fitimage

# Base this image on core-image-minimal
include recipes-core/images/core-image-minimal.bb

# Changing the image compression from gz to lzma achieves 30% saving (~3M).
# However, the current u-boot does not have lzma enabled. Stick to gz
# until we generate a new u-boot image.
IMAGE_FSTYPES += "cpio.lzma.u-boot"
UBOOT_IMAGE_ENTRYPOINT = "0x40008000"
UBOOT_RAMDISK_LOADADDRESS = "0x40800000"
UBOOT_IMAGE_LOADADDRESS = "0x40008000"
FLASH_ROOTFS_OFFSET = "4608"


NTP_PKGS = " \
  ntp \
  ntp-utils \
  sntp \
  ntpdate \
  "

# Include modules in rootfs
IMAGE_INSTALL += " \
  openbmc-utils \
  fbutils \
  ipmid \
  healthd \
  watchdog-ctrl \
  usb-console \
  sensor-util \
  sensor-mon \
  packagegroup-openbmc-base \
  packagegroup-openbmc-python3 \
  ${NTP_PKGS} \
  iproute2 \
  dhcp-client \
  fruid \
  log-util \
  fan-util \
  front-paneld \
  fpc-util \
  fscd \
  packagegroup-openbmc-rest3 \
  bottle \
  lldp-util \
  spatula \
  enclosure-util \
  fw-util \
  openbmc-gpio \
  switchtec-user \
  "

IMAGE_FEATURES += " \
  ssh-server-openssh \
  tools-debug \
  "

DISTRO_FEATURES += " \
  ext2 \
  ipv6 \
  nfs \
  usbgadget \
  usbhost \
  "
