require common/images/fb-openbmc-image.inc

# Install temporary firmware update utilities for EVT phase.
IMAGE_INSTALL:append = " cpld-fw-handler"
