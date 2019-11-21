#ifndef __PAL_CMC_H__
#define __PAL_CMC_H__

#include <openbmc/ipmi.h>

#define CMD_CMC_GET_CONFIG_MODE           (0x63)
#define CMD_CMC_GET_MB_POSITION           (0x0F)
#define CMD_CMC_OEM_GET_SENSOR_READING    (0xF1)
#define CM_I2C_BUS_NUMBER                 (8)
#define CM_I2C_SLAVE_ADDR                 (0x68)

//CMC SENSOR TABLE
enum {
  CM_SNR_FAN0_VOLT = 0x54,
  CM_SNR_FAN1_VOLT = 0x55,
  CM_SNR_FAN2_VOLT = 0x56,
  CM_SNR_FAN3_VOLT = 0x57,
  CM_SNR_FAN0_INTLET_SPEED = 0x66,
  CM_SNR_FAN0_OUTLET_SPEED = 0x67,
  CM_SNR_FAN1_INTLET_SPEED = 0x68,
  CM_SNR_FAN1_OUTLET_SPEED = 0x69,
  CM_SNR_FAN2_INTLET_SPEED = 0x6A,
  CM_SNR_FAN2_OUTLET_SPEED = 0x6B,
  CM_SNR_FAN3_INTLET_SPEED = 0x6C,
  CM_SNR_FAN3_OUTLET_SPEED = 0x6D,
  CM_SNR_P12V = 0x7C,
  CM_SNR_P3V = 0x80,
  CM_SNR_FAN0_CURR = 0xA0,
  CM_SNR_FAN1_CURR = 0xA1,
  CM_SNR_FAN2_CURR = 0xA2,
  CM_SNR_FAN3_CURR = 0xA3,
  CM_SNR_HSC_VIN = 0xC0,
  CM_SNR_HSC_IOUT = 0xC1,
  CM_SNR_HSC_TEMP = 0xC2,
  CM_SNR_HSC_PIN = 0xC3,
  CM_SNR_HSC_PEAK_IOUT = 0xC4,
  CM_SNR_HSC_PEAK_PIN = 0xC5,
};

typedef struct {
  uint8_t bus;
  uint8_t cm_addr;
  uint16_t bmc_addr;
} CMC_RW_INFO;


int cmd_cmc_get_dev_id(ipmi_dev_id_t *dev_id);
int cmd_cmc_get_config_mode(uint8_t *mode);
int cmd_cmc_get_mb_position(uint8_t *partion);
int cmd_cmc_get_sensor_value(uint8_t snr_num, uint8_t *value, uint8_t* rlen);
#endif

