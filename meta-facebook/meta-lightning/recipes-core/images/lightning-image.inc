inherit aspeed_uboot_image

# /dev
require recipes-core/images/aspeed-dev.inc

# Base this image on core-image-minimal
include recipes-core/images/core-image-minimal.bb

# Changing the image compression from gz to lzma achieves 30% saving (~3M).
# However, the current u-boot does not have lzma enabled. Stick to gz
# until we generate a new u-boot image.
IMAGE_FSTYPES += "cpio.lzma.u-boot"
UBOOT_IMAGE_ENTRYPOINT = "0x40800000"

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
  fbutils \
  ipmid \
  healthd \
  watchdog-ctrl \
  i2c-tools \
  usb-console \
  sensor-util \
  sensor-mon \
  ${PYTHON_PKGS} \
  ${NTP_PKGS} \
  iproute2 \
  dhcp-client \
  fruid \
  log-util \
  fan-util \
  front-paneld \
  fpc-util \
  fscd \
  rest-api \
  bottle \
  lldp-util \
  spatula \
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
