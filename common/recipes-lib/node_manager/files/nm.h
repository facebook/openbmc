#ifndef __NODE_MANAGER_H__
#define __NODE_NANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <openbmc/ipmi.h>


//NM CMD INFO
#define SMBUS_MSG_READ_WORD        (0x3 << 1)
#define SMBUS_MSG_WRITE_WORD       (0x4 << 1)
#define SMBUS_STANDARD_READ_WORD   (0x80 | SMBUS_MSG_READ_WORD)
#define SMBUS_STANDARD_WRITE_WORD  (0x80 | SMBUS_MSG_WRITE_WORD)
#define SMBUS_EXTENDED_READ_WORD   (0x90 | SMBUS_MSG_READ_WORD)
#define SMBUS_EXTENDED_WRITE_WORD  (0x90 | SMBUS_MSG_WRITE_WORD)
#define EXTENDED_DEVICE_SMBUS      (0x00)
#define EXTENDED_DEVICE_SMLINK0    (0x01)
#define EXTENDED_DEVICE_SMLINK1    (0x02)
#define EXTENDED_MUX_ADDR          (0x70)
#define EXTENDED_MUX_DISABLE       (0x00)
#define EXTENDED_MUX_ENABLE        (0x01)
#define TRANS_PROTOCOL_PMBUS       (0x00)

enum {
  EXTENDED_MUX_CH0,
  EXTENDED_MUX_CH1,
  EXTENDED_MUX_CH2,
  EXTENDED_MUX_CH3,
};


typedef struct {
  uint8_t bus;
  uint8_t nm_addr;
  uint8_t nm_cmd;
  uint16_t bmc_addr;
} NM_RW_INFO;

int cmd_NM_pmbus_read_word(NM_RW_INFO info, uint8_t dev_addr, uint8_t *rbuf);
int cmd_NM_pmbus_write_word(NM_RW_INFO info, uint8_t dev_addr, uint8_t *data);
int cmd_NM_sensor_reading(NM_RW_INFO info, uint8_t snr_num, uint8_t* rbuf, uint8_t* rlen);
int cmd_NM_cpu_err_num_get(NM_RW_INFO info, bool is_caterr);
int cmd_NM_get_dev_id(NM_RW_INFO* info, ipmi_dev_id_t *dev_id);

#ifdef __cplusplus
} // extern "C"
#endif


#endif




