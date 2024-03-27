require common/images/fb-openbmc-image.inc

# Install temporary firmware update utilities, retimer-util and apml debug tools for POC phase.
IMAGE_INSTALL:append = " fw-util"
IMAGE_INSTALL:append = " apml"
IMAGE_INSTALL:append = " retimer-util"
IMAGE_INSTALL:append = " system-state-init"
