#ifndef __IOCD_H__
#define __IOCD_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>

#define MAX_PID_PATH_SIZE          64
#define MAX_REQUESTS               2
#define MAX_MCTP_RETRY_CNT         3
#define I2C_RETRIES_MAX            15

#define MCTP_MAX_WRITE_SIZE        90
#define MCTP_MAX_READ_SIZE         1024
#define MCTP_MIN_READ_SIZE         15
#define PKT_PAYLOAD_HDR_EXTRA_SIZE 9
#define IOC_SLAVE_ADDRESS          0x68

#define SMBUS_COMMAND_CODE         0x0F
#define HEADER_VERSION             0x01
#define BRCM_MSG_TAG               0xC8
#define MESSAGE_TYPE               0x7E
#define MESSAGE_TYPE_PEC           0xFE

#define RES_PAYLOAD_OFFSET         11
#define RES_NUM_SENSOR_OFFSET      80
#define RES_IOC_TEMP_OFFSET        (RES_NUM_SENSOR_OFFSET + 4)
#define RES_IOC_TEMP_VALID_OFFSET  (RES_IOC_TEMP_OFFSET + 4)
#define RES_IOC_FW_VER_OFFSET      76

#define RES_PL_PEC_FAILURE         0xA1
#define RES_PL_CMD_RESP_NOT_READY  0xA6
#define RES_PL_RESET_IN_PROGRESS   0xA9

#define READ_IOC_TEMP_FAILED       -1
#define WRITE_I2C_RAW_FAILED       -2

#define I2C_MSLAVE_POLL_TIME       100 // 100 milliseconds

#define IOC_SLAVE_QUEUE            "/sys/bus/i2c/devices/%d-%04x/slave-mqueue"
#define DEVICE_NAME                "slave-mqueue"
#define DEVICE_ADDRESS             (0x1010)

typedef enum {
  VDM_RESET,
  GET_IOC_TEMP,
  GET_IOC_FW,
  COMMAND_RESUME,
  COMMAND_DONE,
} ioc_command;

struct mctp_smbus_header_tx {
  uint8_t command_code;  
  uint8_t byte_count;  
  uint8_t source_slave_address;  
};

struct mctp_hdr {
  uint8_t ver;  
  uint8_t dest;  
  uint8_t src;  
  uint8_t flags_seq_tag;  
};

typedef struct {
  uint8_t message_type;
  uint8_t vendor_id[2];
  uint8_t payload_id;
  uint8_t msg_seq_count[2];
  uint8_t app_msg_tag;
} pkt_payload_hdr;

typedef struct {
  uint8_t payload_id;
  uint8_t res_payload_id;
  bool is_need_request;
  uint8_t opcode;
  uint8_t payload_len[4];
  uint8_t response_len[4];
} pkt_payload_hdr_extra;

typedef struct {
  char* command_name;
  pkt_payload_hdr_extra pkt_pl_hdr; 
} ioc_command_info;

typedef struct {
  uint8_t build_num_2;
  uint8_t build_num_1;
  uint8_t cus_id_2;
  uint8_t cus_id_1;
  uint8_t phase_minor;
  uint8_t phase_major;
  uint8_t gen_minor;
  uint8_t gen_major;
} ioc_fw_ver;

//{command name, {payload id, is need request, opcode, payload len[4], response len[4]}}
ioc_command_info ioc_command_map[] = {
  [VDM_RESET] =
  {"vdm reset",                {0x20, 0x44, true, 0xFF, {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}}},
  [GET_IOC_TEMP] =
  {"get IOC temperature",      {0x20, 0x44, true, 0x86, {0x40, 0x00, 0x00, 0x00}, {0x9C, 0x00, 0x00, 0x00}}},
  [GET_IOC_FW] =
  {"get IOC firmware version", {0x20, 0x44, true, 0x86, {0x30, 0x00, 0x00, 0x00}, {0x88, 0x00, 0x00, 0x00}}},
  [COMMAND_RESUME] =
  {"command resume",           {0x40, 0x20, false, 0x00, {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}}},
  [COMMAND_DONE] =
  {"command done",             {0x43, 0x44, false, 0x00, {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}}},
};

uint8_t get_ioc_temp_payload[] = {
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t get_ioc_fw_version_payload[] = {
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#endif
