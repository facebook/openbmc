require common/images/fb-openbmc-image.inc

IMAGE_INSTALL:append = " fw-util"
IMAGE_INSTALL:append = " fw-versions"
IMAGE_INSTALL:append = " redfish-client"
