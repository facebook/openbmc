IMAGE_INSTALL:append = " \
    fw-util \
    fw-versions \
    redfish-client \
    cpld-fw-handler \
"

IMAGE_INSTALL:append:clemente = " \
    pldm-update \
"
