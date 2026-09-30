#ifndef __PAL_POWER_H__
#define __PAL_POWER_H__

#define SET_SERVER_PWR_DELAY 3
#define SERVER_AC_CYCLE_DELAY 2

// UIC FPGA command
#define CMD_CODE_NIC_POWER_CONTROL 0x3

// BS FPGA command
#ifdef CONFIG_GRANDCANYON2
#define BS_CHECK_BIC_STANDBY_MAX_RETRY        (6)

#define BS_FPGA_SERVER_CHECK_BIC_STBY_PWR_RDY 0x03
#define BS_FPGA_SERVER_POWER_CTRL 0x22
#define BS_FPGA_SERVER_POWER_STATUS 0x04
#define BS_FPGA_E1S0_POWER_CTRL 0x0D
#define BS_FPGA_E1S1_POWER_CTRL 0x0C

#define E1S_POWER_ADD       0x00  // bit 4
#define E1S_POWER_REMOVE    0x10  // bit 4

#define BS_FPGA_SERVER_CHECK_BIC_STBY_PWR_RDY_BIT 0x04

// BMC CPLD I2C Configuration
#define BMC_CPLD_I2C_BUS            3
#define BMC_CPLD_SLAVE_ADDR         0x0f

// Register Addresses
#define BMC_CPLD_RESET_REG          0x01    // Reset control register

// Register 0x01 Bit Masks (for reference)
#define CPLD_BIT_RST_BTN_OUT        0x01    // bit 0: Reset Button Output

// Reset Control Values
// bit 0 = 0: Reset low
// bit 0 = 1: Reset high
#define BMC_CPLD_RESET_LOW       0x32    // (bit 0 = 0)
#define BMC_CPLD_RESET_HIGH      0x33    // (bit 0 = 1)
#define BMC_CPLD_RESET_DELAY        1       // Hold time in seconds

// ES FPGA (Server CPLD) DC Power Status - GC2 specific
// Device: I2C Bus 3 (I2C_ES_FPGA_BUS), 7-bit addr 0x0F (ES_FPGA_SLAVE_ADDR)
#define ES_FPGA_DC_POWER_STATUS_OFFSET  0x0F    // Register offset for DC power status
#define ES_FPGA_DC_POWER_STATUS_BIT     1       // Bit 1: 1=DC_ON, 0=DC_OFF

#else
#define BS_FPGA_SERVER_POWER_CTRL 0x0F
#define BS_FPGA_SERVER_POWER_STATUS 0x04
#define BS_FPGA_E1S0_POWER_CTRL 0x0D
#define BS_FPGA_E1S1_POWER_CTRL 0x0C
#define E1S_POWER_ADD       0x00
#define E1S_POWER_REMOVE    0x01

#endif

#define SERVER_POWER_BTN_HIGH 1
#define SERVER_POWER_BTN_LOW  0



enum {
  POWER_STATUS_ALREADY_OK = 1,
  POWER_STATUS_OK = 0,
  POWER_STATUS_ERR = -1,
  POWER_STATUS_FRU_ERR = -2,
};

enum {
  DEVICE_POWER_OFF = 0x0,
  DEVICE_POWER_ON = 0x1,
};

typedef enum {
  NIC_VAUX_MODE = 0, // Standby mode
  NIC_VMAIN_MODE,
} nic_power_control_mode;

typedef struct {
  uint8_t nic_power_control_cmd_code;
  uint8_t nic_power_control_mode;
} SET_NIC_POWER_MODE_CMD;

int pal_set_server_power(uint8_t fru, uint8_t cmd);
int pal_get_server_power(uint8_t fru, uint8_t *status);
int pal_get_server_12v_power(uint8_t fru, uint8_t *status);
int pal_sled_cycle(void);
uint8_t pal_set_power_restore_policy(uint8_t slot, uint8_t *pwr_policy, uint8_t *res_data);
void pal_get_chassis_status(uint8_t fru, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len);
int pal_server_power_ctrl(uint8_t action);
int pal_set_pwr_btn(uint8_t val);
int pal_server_power_cycle();
int pal_set_dev_power_status(uint8_t dev_id, uint8_t cmd);
int pal_host_power_off_pre_actions();
int pal_host_power_off_post_actions();
int pal_host_power_on_pre_actions();
int pal_host_power_on_post_actions();
int pal_restore_host_power_on_pre_actions();

#ifdef CONFIG_GRANDCANYON2
bool pal_power_transition_event_filtered(const char *key_suffix,
                                         bool include_on_transition,
                                         const char *token);
#endif

#endif
