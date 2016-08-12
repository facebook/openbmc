inherit kernel_fitimage

# Base this image on core-image-minimal
include recipes-core/images/core-image-minimal.bb

# Changing the image compression from gz to lzma achieves 30% saving (~3M).
# However, the current u-boot does not have lzma enabled. Stick to gz
# until we generate a new u-boot image.
# UBOOT_IMAGE_LOADADDRESS = "0x80008000"
# UBOOT_IMAGE_ENTRYPOINT = "0x80008000"
UBOOT_RAMDISK_LOADADDRESS = "0x80800000"

# The offset must match with the offsets defined in
# dev-spi-cmm.c. Rootfs starts from 4.5M
FLASH_ROOTFS_OFFSET = "4608"

PYTHON_PKGS = " \
  python-core \
  python-io \
  python-json \
  python-shell \
  python-subprocess \
  python-argparse \
  python-ctypes \
  python-datetime \
  python-email \
  python-threading \
  python-mime \
  python-pickle \
  python-misc \
  python-netserver \
  "

NTP_PKGS = " \
  ntp \
  ntp-utils \
  sntp \
  ntpdate \
  "

# Include modules in rootfs
IMAGE_INSTALL += " \
  kernel-modules \
  u-boot \
  u-boot-fw-utils \
  openbmc-utils \
  at93cx6-util \
  ast-mdio \
  watchdog-ctrl \
  i2c-tools \
  lldp-util \
  bitbang \
  ${PYTHON_PKGS} \
  ${NTP_PKGS} \
  iproute2 \
  dhcp-client \
  flashrom \
  cherryPy \
  plat-utils \
  fscd \
  fan-util \
  power-util \
  consoled \
  front-paneld \
  ipmid \
  fruid \
  sensor-util \
  sensor-mon \
  ipmbd \
  me-util \
  log-util \
  kcsd \
  healthd \
  fpc-util \
  fw-util \
  cfg-util \
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
