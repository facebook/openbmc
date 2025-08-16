require common/images/fb-openbmc-image.inc

IMAGE_INSTALL:append = " serfmon-cache \
                         serfmon-cache-dbus \
                         prefdl-eeprom \
                        "
