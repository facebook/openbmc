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
  python-syslog \
  python-ply \
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
  healthd \
  fbutils \
  fan-util \
  fscd \
  watchdog-ctrl \
  i2c-tools \
  sensor-setup \
  usb-console \
  lmsensors-sensors \
  rest-api \
  bottle \
  ipmid \
  ${PYTHON_PKGS} \
  ${NTP_PKGS} \
  iproute2 \
  dhcp-client \
  fruid \
  ipmbd \
  bic-cached \
  bic-util \
  fbttn-sensors \
  sensor-util \
  sensor-mon \
  gpiod \
  front-paneld \
  power-util \
  cfg-util \
  fw-util \
  fpc-util \
  me-util \
  log-util \
  cherryPy \
  lldp-util \
  fan-util \
  spatula \
  passwd-util \
  openbmc-utils \
  libexp \
  expander-util \
  exp-cached \
  enclosure-util \
  libmctp \
  mTerm \
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
