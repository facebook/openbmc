require common/images/fb-openbmc-image.inc

# Install temporary firmware update utilities for EVT phase.
IMAGE_INSTALL:append = " cpld-fw-handler"
IMAGE_INSTALL:append = " fw-util"
IMAGE_INSTALL:append = " fw-versions"
IMAGE_INSTALL:append = " addc"
IMAGE_INSTALL:append = " apml"
IMAGE_INSTALL:append = " dimm-util"
IMAGE_INSTALL:append = " pldm-update"
IMAGE_INSTALL:append = " pldm-package-wrapper"
