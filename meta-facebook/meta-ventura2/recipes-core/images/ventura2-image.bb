require common/images/fb-openbmc-image.inc

# FTDI-based MDIO tool and scripts to initialize the Marvell switch during the EVT phase
IMAGE_INSTALL:append = " ftdi-mdio"

# Install temporary firmware update utilities.
IMAGE_INSTALL:append = " cpld-fw-handler"
