require common/images/fb-openbmc-image.inc

IMAGE_INSTALL:append = " serfmon-cache \
                         serfmon-cache-dbus \
                         prefdl-eeprom \
                         eth0-mac-fixup \
                         sol \
                         wedge-power \
                         boot-info \
                         mfg-tool \
                         bmc-eeprom-checker \
                         fboss-serfmon-emitter \
                         lldp-util \
                         dhcp-id \
                         fboss-board-revision \
                         packagegroup-openbmc-emmc \
                        "
