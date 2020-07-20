#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include "openbmc/log.h"
#include "facebook/wedge_eeprom.h"

int platform_get_mac(uint8_t mac[6])
{
  struct wedge_eeprom_st eeprom;
  int from_eeprom = 0;

  /* read EEPROM for the MAC */
  if (wedge_eeprom_parse(NULL, &eeprom) == 0) {
    uint16_t carry;
    int pos;
    int adj;
    /*
     * OOB MAC comes from this range. We pick the last MAC from the range to
     * use as OOB MAC.
     */
    if (eeprom.fbw_mac_size > 128) {
      OBMC_ERROR(EFAULT, "Extended MAC size (%d) is too large.",
              eeprom.fbw_mac_size);
      carry = 128;
    } else {
      carry = eeprom.fbw_mac_size;
    }

    /*
     * Due to various manufacture issues, some FC boards have MAC range overlap
     * between LEFT and RIGHT sides. A SW workaround is done below to use the
     * 8th (or 7th for right side FC) last MAC from the range for FC.
     */
    if (strncmp(eeprom.fbw_location, "LEFT", FBW_EEPROM_F_LOCATION) == 0) {
      adj = 8;
    } else if (strncmp(eeprom.fbw_location, "RIGHT", FBW_EEPROM_F_LOCATION)
               == 0) {
      adj = 7;
    } else {
      adj = 1;
    }

    if (carry < adj) {
      OBMC_ERROR(EFAULT, "Invalid extended MAC size: %d", eeprom.fbw_mac_size);
    } else {
      carry -= adj;
      memcpy(mac, eeprom.fbw_mac_base, sizeof(mac));
      for (pos = sizeof(mac) - 1; pos >= 0 && carry; pos--) {
        uint16_t tmp = mac[pos] + carry;
        mac[pos] = tmp & 0xFF;
        carry = tmp >> 8;
      }
      from_eeprom = 1;
    }
  }
  return from_eeprom ? 0 : -1;
}
