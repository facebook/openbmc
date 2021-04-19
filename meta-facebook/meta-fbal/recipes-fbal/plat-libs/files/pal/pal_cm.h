#ifndef __PAL_CMC_H__
#define __PAL_CMC_H__

#include <openbmc/ipmi.h>

#define CMD_CMC_GET_FAN_DUTY              (0x07)
#define CMD_CMC_SET_FAN_DUTY              (0x08)
#define CMD_CMC_SET_FAN_CONTROL_STATUS    (0x0E)
#define CMD_CMC_GET_CONFIG_MODE           (0x63)
#define CMD_CMC_SET_SYSTEM_MODE           (0x64)
#define CMD_CMC_REQ_DC_ON                 (0x68)
#define CMD_CMC_SLED_CYCLE                (0x70)
#define CMD_CMC_POWER_CYCLE               (0x72)
#define CMD_CMC_GET_MB_POSITION           (0x0F)
#define CMD_CMC_OEM_GET_SENSOR_READING    (0xF1)
#define CMD_CMC_OEM_SET_BLOCK_COMMON_FLAG (0xF2)
#define CMD_CMC_OEM_1S_BMC_RESET          (0x16)
#define CM_IPMB_BUS_ID                    (8)
#define CM_SLAVE_ADDR                     (0x68)

#define CM_SET_FAN_MANUAL_MODE            (0x00)
#define CM_SET_FAN_AUTO_MODE              (0x01)

#define CM_SET_FLAG_STATUS                (0x00)
#define CM_GET_FLAG_STATUS                (0x01)

#define CM_COMMAND_UNBLOCK                (0x00)
#define CM_COMMAND_BLOCK                  (0x01)

#define UPDATE_BIOS_BLOCK       (1 << 1)
#define UPDATE_CM_BLOCK         (1 << 2)
#define UPDATE_VR_BLOCK         (1 << 3)
#define UPDATE_GLOB_PLD_BLOCK   (1 << 4)
#define UPDATE_MAIN_PLD_BLOCK   (1 << 8)
#define UPDATE_MOD_PLD_BLOCK    (1 << 9)

#define PDB_EVENT_STATUS        (0xFB)
#define PDB_EVENT_FAN0_PRESENT  (0x2C)
#define PDB_EVENT_FAN1_PRESENT  (0x2D)
#define PDB_EVENT_FAN2_PRESENT  (0x2E)
#define PDB_EVENT_FAN3_PRESENT  (0x2F)


//CMC SENSOR TABLE
enum {
  CM_SNR_FAN0_VOLT = 0x54,
  CM_SNR_FAN1_VOLT = 0x55,
  CM_SNR_FAN2_VOLT = 0x56,
  CM_SNR_FAN3_VOLT = 0x57,
  CM_SNR_FAN0_INLET_SPEED = 0x66,
  CM_SNR_FAN0_OUTLET_SPEED = 0x67,
  CM_SNR_FAN1_INLET_SPEED = 0x68,
  CM_SNR_FAN1_OUTLET_SPEED = 0x69,
  CM_SNR_FAN2_INLET_SPEED = 0x6A,
  CM_SNR_FAN2_OUTLET_SPEED = 0x6B,
  CM_SNR_FAN3_INLET_SPEED = 0x6C,
  CM_SNR_FAN3_OUTLET_SPEED = 0x6D,
  CM_SNR_P12V = 0x7C,
  CM_SNR_P3V = 0x80,
  CM_SNR_FAN0_CURR = 0xA0,
  CM_SNR_FAN1_CURR = 0xA1,
  CM_SNR_FAN2_CURR = 0xA2,
  CM_SNR_FAN3_CURR = 0xA3,
  CM_SNR_FAN0_POWER = 0xB0,
  CM_SNR_FAN1_POWER = 0xB1,
  CM_SNR_FAN2_POWER = 0xB2,
  CM_SNR_FAN3_POWER = 0xB3,
  CM_SNR_HSC_VIN = 0xC0,
  CM_SNR_HSC_IOUT = 0xC1,
  CM_SNR_HSC_TEMP = 0xC2,
  CM_SNR_HSC_PIN = 0xC3,
  CM_SNR_HSC_PEAK_IOUT = 0xC4,
  CM_SNR_HSC_PEAK_PIN = 0xC5,
};

enum {
  FAN_ID0,
  FAN_ID1,
  FAN_ID2,
  FAN_ID3,
  FAN_ID4,
  FAN_ID5,
  FAN_ID6,
  FAN_ID7,
  FAN_ALL = 0xFF,
};

// CM Mode
enum {
  CM_MODE_8S_0 = 0x3, // 8 Socket, Tray0 is primary
  CM_MODE_8S_1 = 0x2, // 8 Socket, Tray1 is primary
  CM_MODE_8S_2 = 0x1, // 8 Socket, Tray2 is primary
  CM_MODE_8S_3 = 0x0, // 8 Socket, Tray3 is primary
  CM_MODE_2S = 0x4, // 2 Socket mode.
  CM_MODE_4S_4OU = 0x5, // 4 Socket mode in 4OU
  CM_MODE_4S_2OU_1 = 0x6, // 4 Socket mode in 2OU with Tray 1 as primary
  CM_MODE_4S_2OU_0 = 0x7 // 4 Socket mode in 2OU with Tray 0 as primary
};

typedef struct {
  uint8_t fan_id ;
  uint8_t sdr;
} CMC_FAN_INFO;

typedef struct {
  uint8_t bus;
  uint8_t cm_addr;
  uint16_t bmc_addr;
} CMC_RW_INFO;


enum {
  PDB_EVENT_CM_RESET = 0,
  PDB_EVENT_PWR_CYCLE = 2,
  PDB_EVENT_SLED_CYCLE = 3,
  PDB_EVENT_RECONFIG_SYSTEM = 4,
  PDB_EVENT_CM_EXTERNAL_RESET = 5,
};

enum {
  COMP_BMC =0,
  COMP_BIOS,
  COMP_CM,
  COMP_VR,
  COMP_CPLD,
  COMP_NIC,
  COMP_MCU,
  COMP_CPU,  
};

int cmd_cmc_get_dev_id(ipmi_dev_id_t *dev_id);
int cmd_cmc_get_config_mode(uint8_t *mode);
int cmd_cmc_get_config_mode_ext(uint8_t *def_mode, uint8_t *cur_mode, uint8_t *ch_mode, uint8_t *jumpers);
int cmd_cmc_reset_bmc(uint8_t pos);
int cmd_cmc_get_mb_position(uint8_t *partion);
int cmd_cmc_get_sensor_value(uint8_t snr_num, uint8_t *value, uint8_t* rlen);
int cmd_cmc_set_system_mode(uint8_t mode, bool do_cycle);
int cmd_cmc_sled_cycle(void);
int lib_cmc_set_fan_pwm(uint8_t fan_num, uint8_t pwm);
int lib_cmc_get_fan_pwm(uint8_t fan_num, uint8_t* pwm);
int lib_cmc_get_fan_speed(uint8_t fan_id, uint16_t* speed);
int lib_cmc_set_fan_ctrl(uint8_t fan_mode, uint8_t* status);
int lib_cmc_get_fan_id(uint8_t fan_sdr);
int lib_cmc_power_cycle(void);
int lib_cmc_bios_update_ac(void);
int lib_cmc_set_block_command_flag(uint8_t index, uint8_t flag);
int lib_cmc_get_block_command_flag(uint8_t* rbuf, uint8_t* rlen);
int lib_cmc_get_block_index(uint8_t fru);
int lib_cmc_req_dc_on(void);
#endif

