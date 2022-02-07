#ifndef __PAL_POWER_H__
#define __PAL_POWER_H__

#define DELAY_12V_CYCLE 2

enum {
  DEVICE_POWER_OFF = 0x0,
  DEVICE_POWER_ON = 0x1,
};

int pal_server_set_nic_power(const uint8_t expected_pwr);
int pal_set_server_power(uint8_t fru, uint8_t cmd);
int pal_get_server_power(uint8_t fru, uint8_t *status);
int pal_get_server_12v_power(uint8_t slot_id, uint8_t *status);
int pal_get_device_power(uint8_t slot_id, uint8_t dev_id, uint8_t *status, uint8_t *type);
int pal_set_device_power(uint8_t slot_id, uint8_t dev_id, uint8_t cmd);

#endif
