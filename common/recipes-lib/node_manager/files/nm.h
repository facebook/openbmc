#ifndef __NODE_MANAGER_H__
#define __NODE_NANAGER_H__

//NM CMD INFO
#define NM_IPMB_BUS_ID           (5)
#define NM_SLAVE_ADDR            (0x2C)
#define SMBUS_MSG_READ_WORD      (0x3 << 1)
#define SMBUS_MSG_WRITE_WORD     (0x4 << 1)
#define SMBUS_EXTEND_READ_WORD   (0x80 | SMBUS_MSG_READ_WORD);
#define SMBUS_EXTEND_WRITE_WORD  (0x80 | SMBUS_MSG_WRITE_WORD);

typedef struct {
  uint8_t bus;
  uint8_t nm_addr;
  uint8_t nm_cmd;
  uint16_t bmc_addr;
} NM_RW_INFO;

int cmd_NM_pmbus_read_word(NM_RW_INFO info, uint8_t dev_addr, uint8_t *rbuf);
int cmd_NM_pmbus_write_word(NM_RW_INFO info, uint8_t dev_addr, uint8_t *data);
int cmd_NM_sensor_reading(NM_RW_INFO info, uint8_t snr_num, uint8_t* rbuf, uint8_t* rlen);
#endif




