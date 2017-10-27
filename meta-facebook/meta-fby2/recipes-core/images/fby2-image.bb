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
  python3-core \
  python3-io \
  python3-json \
  python3-shell \
  python3-subprocess \
  python3-argparse \
  python3-ctypes \
  python3-datetime \
  python3-email \
  python3-threading \
  python3-mime \
  python3-pickle \
  python3-misc \
  python3-netserver \
  python3-syslog \
  python3-ply \
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
  plat-utils \
  fan-util \
  fscd \
  watchdog-ctrl \
  i2c-tools \
  sensor-setup \
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
  ncsi-util \
  fby2-sensors \
  sensor-util \
  sensor-mon \
  gpiod \
  gpiointrd \
  front-paneld \
  power-util \
  mTerm\
  cfg-util \
  fw-util \
  hsvc-util \
  fpc-util \
  me-util \
  log-util \
  cherryPy \
  lldp-util \
  fan-util \
  spatula \
  passwd-util \
  openbmc-utils \
  ipmi-util \
  guid-util \
  asd \
  asd-test \
  logrotate \
  bios-util \
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
