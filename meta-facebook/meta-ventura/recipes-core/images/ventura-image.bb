require common/images/fb-openbmc-image.inc

# Install temporary firmware update utilities for POC phase.
IMAGE_INSTALL:append = " fw-util"

# Install temporary firmware update utilities for EVT phase.
IMAGE_INSTALL:append = " cpld-fw-handler"

IMAGE_INSTALL:append = " fw-versions"

# Ventura will start monitoring AALCs instead of the wedge400
IMAGE_INSTALL:append = " rackmon"

# Scripts to control connected RPUs
IMAGE_INSTALL:append = " modbus-device-util"

# Scripts to perform FW Upgrade of PSU/BBU/RPU via rackmon
IMAGE_INSTALL:append = " psu-update"

# Service to detect erroring usb-serial
IMAGE_INSTALL:append = " usb-serial-monitor"

# XR Config
IMAGE_INSTALL:append = " xr21-gpio-mod"
