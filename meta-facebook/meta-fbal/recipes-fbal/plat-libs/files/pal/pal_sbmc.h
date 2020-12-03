#ifndef __PAL_SBMC_H__
#define __PAL_SBMC_H__

int cmd_set_smbc_restore_power_policy(uint8_t policy, uint8_t t_bmc_addr);
int cmd_smbc_chassis_ctrl(uint8_t cmd, uint8_t t_bmc_addr);
int cmd_smbc_get_glbcpld_ver(uint8_t t_bmc_addr, uint8_t *ver);
int cmd_mb0_bridge_to_cc(uint8_t ipmi_cmd, uint8_t ipmi_netfn, uint8_t *data, uint8_t data_len);
int cmd_mb0_set_cc_reset(uint8_t t_bmc_addr);
#endif

