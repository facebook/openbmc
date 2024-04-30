require common/images/fb-openbmc-image.inc

# Install temporary firmware update utilities for POC phase.
IMAGE_INSTALL:append = " bios-fw-update"
IMAGE_INSTALL:append = " cpld-fw-handler"
IMAGE_INSTALL:append = " pldm-fw-update"
IMAGE_INSTALL:append = " pldm-package-re-wrapper"
IMAGE_INSTALL:append = " cxl-fw-update"
IMAGE_INSTALL:append = " addc"
IMAGE_INSTALL:append = " yaap"
