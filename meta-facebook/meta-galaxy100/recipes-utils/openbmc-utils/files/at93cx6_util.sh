#!/bin/sh

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

#At this way, it uses to GPIO mode
devmem_clear_bit 0x1e6e2070 12
devmem_clear_bit 0x1e6e2070 13

if [ ! -L "${SHADOW_GPIO}/BMC_EEPROM1_SPI_SS" ]; then
    gpio_export_by_name "$ASPEED_GPIO" GPIOI4 BMC_EEPROM1_SPI_SS
    gpio_export_by_name "$ASPEED_GPIO" GPIOI5 BMC_EEPROM1_SPI_SCK
    gpio_export_by_name "$ASPEED_GPIO" GPIOI6 BMC_EEPROM1_SPI_MOSI
    gpio_export_by_name "$ASPEED_GPIO" GPIOI7 BMC_EEPROM1_SPI_MISO
fi

gpio_set_value SWITCH_EEPROM1_WRT 1

/usr/local/bin/at93cx6_util_py3.py --cs BMC_EEPROM1_SPI_SS \
                                   --clk BMC_EEPROM1_SPI_SCK \
                                   --mosi BMC_EEPROM1_SPI_MOSI \
                                   --miso BMC_EEPROM1_SPI_MISO $@

#resolve to SPI mode for another flash using(backup flash)
devmem_set_bit 0x1e6e2070 12
