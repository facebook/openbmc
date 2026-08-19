require common/images/fb-openbmc-image.inc

IMAGE_INSTALL:append = " apml"
IMAGE_INSTALL:append = " addc"
IMAGE_INSTALL:append = " redfish-client"
IMAGE_INSTALL:append = " amc-util"
IMAGE_INSTALL:append = " bmc-ppr"
