#ifndef __PAL_CC_H__
#define __PAL_CC_H__

#include <openbmc/ipmi.h>

#define CC_I2C_BUS_NUMBER                (6)
#define CC_I2C_SLAVE_ADDR                (0x2E)
#define CC_I2C_RESET_IOEXP_ADDR          (0xEC)

#define CC_PDB_SNR_HSC    (0x51)

enum {
  CC_USB_BMC_MB0 = 0x0,
  CC_USB_BMC_MB1,
  CC_USB_PCH_MB0,
  CC_USB_PCH_MB1,
  CC_USB_CH_CNT,
};


int pal_cc_sled_cycle(void);
bool is_cc_present(void);
int pal_cc_get_lan_config(uint8_t sel, uint8_t *buf, uint8_t *rlen);
int pal_cc_set_usb_ch(uint8_t dev, uint8_t mb);

#endif
