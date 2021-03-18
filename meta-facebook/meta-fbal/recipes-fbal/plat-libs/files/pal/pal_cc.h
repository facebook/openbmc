#ifndef __PAL_CC_H__
#define __PAL_CC_H__

#include <openbmc/ipmi.h>

#define CC_I2C_BUS_NUMBER                (6)
#define CC_I2C_SLAVE_ADDR                (0x2E)
#define CC_I2C_RESET_IOEXP_ADDR          (0xEC)

#define CC_PDB_SNR_HSC    (0x51)

//PCA9555
#define CC_IOEXP_IN0_REG   (0x00)
#define CC_IOEXP_IN1_REG   (0x01)
#define CC_IOEXP_OUT0_REG  (0x02)
#define CC_IOEXP_OUT1_REG  (0x03)
#define CC_IOEXP_CFG0_REG  (0x06)
#define CC_IOEXP_CFG1_REG  (0x07)

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
