#ifndef __PAL_EP_H__
#define __PAL_EP_H__

#include <openbmc/ipmi.h>

#define EP_I2C_BUS_NUMBER                (6)
#define EP_I2C_SLAVE_ADDR                (0x2C)

#define EP_PDB_SNR_HSC  (0x6F)
enum {
  EP_USB_BMC_MB0 = 0x0, 
  EP_USB_BMC_MB1,
  EP_USB_PCH_MB0,
  EP_USB_PCH_MB1,
  EP_USB_CH_CNT,
};


int pal_ep_set_system_mode(uint8_t);
int pal_ep_sled_cycle(void);
bool is_ep_present(void);
int pal_ep_get_lan_config(uint8_t sel, uint8_t *buf, uint8_t *rlen);
int pal_ep_set_usb_ch(uint8_t dev, uint8_t mb);


#endif

