require common/images/fb-openbmc-image.inc

IMAGE_INSTALL:append = " cpld-fw-handler"

IMAGE_CLASSES:append = " image_types_phosphor_aspeed_g7"
