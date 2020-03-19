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
#define TRANS_PROTOCOL_I2C         (0x01)


enum {
  EXTENDED_MUX_CH0,
  EXTENDED_MUX_CH1,
  EXTENDED_MUX_CH2,
  EXTENDED_MUX_CH3,
  EXTENDED_MUX_CH4,
};

enum {
  NM_SENSOR_BUS_SMBUS, 
  NM_SENSOR_BUS_SMLINK0,
  NM_SENSOR_BUS_SMLINK1,
  NM_SENSOR_BUS_SMLINK2,
  NM_SENSOR_BUS_SMLINK3,
  NM_SENSOR_BUS_SMLINK4,
};

typedef struct {
  uint8_t bus;
  uint8_t nm_addr;
  uint16_t bmc_addr;
  uint8_t nm_cmd;
} NM_RW_INFO;

typedef struct __attribute__((__packed__)) {
  uint8_t psu_cmd;
  uint8_t psu_addr; 
} NM_PMBUS_STANDAR_DEV;

typedef struct __attribute__((__packed__)) {
  uint8_t psu_cmd;
  uint8_t psu_addr;
  uint8_t mux_addr;
  uint8_t mux_ch;
  uint8_t sensor_bus;
} NM_PMBUS_EXTEND_DEV;

#if 0
typedef struct __attribute__((__packed__)) {
  uint8_t flags;
  uint8_t target_addr;
  uint8_t mux_cfg;
  uint8_t trans_protocal;
  uint8_t wr_len;
  uint8_t rd_len;
  uint8_t pmbus_cmd;
} NM_SEND_PMBUS_STANDARD;

typedef struct __attribute__((__packed__)) {
  uint8_t flags;           //[3:1]smbus type, [5:4] device format [7] enable PEC
  uint8_t sensor_bus;      //SMLINK or SMBUS number 
  uint8_t target_addr;     //PSU addr
  uint8_t mux_addr;        //Mux slave addr
  uint8_t mux_ch;          //Mux channel
  uint8_t mux_cfg;         //Mux support
  uint8_t trans_protocal;  //I2C or PMBus
  uint8_t wr_len;          //cmd + data length
  uint8_t rd_len;          //read data length
  uint8_t pmbus_cmd;       //pmbus cmd
} NM_SEND_PMBUS_EXTEND;
#endif

int cmd_NM_pmbus_standard_read_word(NM_RW_INFO info, uint8_t* buf, uint8_t *rdata);
int cmd_NM_pmbus_standard_write_word(NM_RW_INFO info, uint8_t* buf, uint8_t *wdata);
int cmd_NM_pmbus_extend_read_word(NM_RW_INFO info, uint8_t* buf, uint8_t *rdata);
int cmd_NM_pmbus_extend_write_word(NM_RW_INFO info, uint8_t* buf, uint8_t *wdata);
int cmd_NM_sensor_reading(NM_RW_INFO info, uint8_t snr_num, uint8_t* rbuf, uint8_t* rlen);
int cmd_NM_cpu_err_num_get(NM_RW_INFO info, bool is_caterr);
int cmd_NM_get_dev_id(NM_RW_INFO* info, ipmi_dev_id_t *dev_id);
int cmd_NM_get_self_test_result(NM_RW_INFO* info, uint8_t *rbuf, uint8_t *rlen);

static inline int cmd_NM_pmbus_read_word(NM_RW_INFO info, uint8_t dev_addr, uint8_t *rbuf)
{
  NM_PMBUS_STANDAR_DEV dev = {info.nm_cmd, dev_addr};
  return cmd_NM_pmbus_standard_read_word(info, (uint8_t *)&dev, rbuf);
}
static inline int cmd_NM_pmbus_write_word(NM_RW_INFO info, uint8_t dev_addr, uint8_t *wbuf)
{
  NM_PMBUS_STANDAR_DEV dev = {info.nm_cmd, dev_addr};
  return cmd_NM_pmbus_standard_write_word(info, (uint8_t *)&dev, wbuf);
}

#ifdef __cplusplus
} // extern "C"
#endif


#endif




