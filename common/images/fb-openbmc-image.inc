OVERRIDES:append = ":pn-obmc-phosphor-image"
require recipes-phosphor/images/fb-phosphor-image.inc
require recipes-phosphor/images/obmc-phosphor-image.bb
require common/images/fb-openbmc-common.inc

# Include the package group that enables tests2 support.
IMAGE_INSTALL += "packagegroup-openbmc-tests2"
