require common/images/fb-openbmc-image.inc

IMAGE_INSTALL:append = " apml"
IMAGE_INSTALL:append = " addc"
IMAGE_INSTALL:append = " dimm-util"
IMAGE_INSTALL:append = " stress-ng"

IMAGE_CLASSES:append = " image_types_phosphor_aspeed_g7"
