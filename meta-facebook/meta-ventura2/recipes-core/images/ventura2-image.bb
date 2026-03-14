require common/images/fb-openbmc-image.inc

# FTDI-based MDIO tool and scripts to initialize the Marvell switch during the EVT phase
IMAGE_INSTALL:append = " ftdi-mdio"

# Install temporary firmware update utilities.
IMAGE_INSTALL:append = " cpld-fw-handler"

# Modify the NCSI strenth
IMAGE_INSTALL:append = " ncsi-strength"

# Rackmon support for AALCv2
IMAGE_INSTALL:append = " rackmon"

# Scripts to control connected AALCv2
IMAGE_INSTALL:append = " modbus-device-util"

# Scripts to perform FW Upgrade of AALCv2.
IMAGE_INSTALL:append = " psu-update"

