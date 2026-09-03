require common/images/fb-openbmc-image.inc

IMAGE_INSTALL:append = " apml"
IMAGE_INSTALL:append = " addc"
IMAGE_INSTALL:append = " dimm-util"
IMAGE_INSTALL:append = " stress-ng"
IMAGE_INSTALL:append = " libcper"
IMAGE_INSTALL:append = " cpld-fw-handler"
IMAGE_INSTALL:append = " drive-fault-monitor"
IMAGE_INSTALL:append = " phosphor-state-manager-drive"

IMAGE_CLASSES:append = " image_types_phosphor_aspeed_g7"
