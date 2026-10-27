require common/images/fb-openbmc-image.inc

IMAGE_INSTALL:append = " apml"
IMAGE_INSTALL:append = " bmc-ras"
IMAGE_INSTALL:append = " redfish-client"
IMAGE_INSTALL:append = " amc-util"
IMAGE_INSTALL:append = " bmc-ppr"
IMAGE_INSTALL:append = " fru-am4-init-update"
IMAGE_INSTALL:append = " adhoc-sensor"
IMAGE_INSTALL:append = " obmc-console"
IMAGE_INSTALL:append = " libamdcper"
