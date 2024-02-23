require common/images/fb-openbmc-image.inc

# Install temporary firmware update utilities and apml debug tools for POC phase.
IMAGE_INSTALL:append = " fw-util"
IMAGE_INSTALL:append = " apml"
