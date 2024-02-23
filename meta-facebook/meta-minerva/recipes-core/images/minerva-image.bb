require common/images/fb-openbmc-image.inc

# Install temporary firmware update utilities for POC phase.
IMAGE_INSTALL:append = " fw-util"
