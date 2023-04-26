#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "dimm.h"

#define PMIC_ERR_INJ_REG 0x35
#define ERR_PATTERN_LEN  7
#define PMIC_WRITE_PROTECT_BIT 2

#define STR_SWA_OV     "SWAout_OV"
#define STR_SWB_OV     "SWBout_OV"
#define STR_SWC_OV     "SWCout_OV"
#define STR_SWD_OV     "SWDout_OV"
#define STR_BULK_OV    "VinB_OV"
#define STR_MGMT_OV    "VinM_OV"
#define STR_SWA_UV     "SWAout_UV"
#define STR_SWB_UV     "SWBout_UV"
#define STR_SWC_UV     "SWCout_UV"
#define STR_SWD_UV     "SWDout_UV"
#define STR_BULK_UV    "VinB_UV"
#define STR_SWITCHOVER "Vin_switchover"
#define STR_HIGH_TEMP  "high_temp"
#define STR_PG_1V8     "Vout_1v8_PG"
#define STR_HIGH_CURR  "high_current"
#define STR_CURR_LIMIT "current_limit"
#define STR_CRIT_TEMP  "critical_temp_shutdown"

enum {
  TYPE_UNDEF = 0,
  TYPE_SWITCHOVER,
  TYPE_CRIT_TEMP,
  TYPE_HIGH_TEMP,
  TYPE_PG_1V8,
  TYPE_HIGH_CURR,
  TYPE_CAMP_INPUT,
  TYPE_CURR_LIMIT,
};

enum {
  VOLT_UNDEF = 0,
  VOLT_OV = 0,
  VOLT_UV,
};

enum {
  RAIL_UNDEF = 0,
  RAIL_SWA_OUT,
  RAIL_SWB_OUT,
  RAIL_SWC_OUT,
  RAIL_SWD_OUT,
  RAIL_VIN_BULK,
  RAIL_VIN_MGMT,
  RAIL_ALL,
};

enum {
  ERR_INJ_DISABLE = 0,
  ERR_INJ_ENABLE,
};

// PMIC error injection register R35
typedef struct {
  uint8_t err_type:3;
  uint8_t uv_ov_select:1;
  uint8_t rail:3;
  uint8_t enable:1;
} err_inject_reg;

// PMIC has 6 register to reflect the errors (R05, R06, R08, R09, R0A, R0B)
typedef struct {
  uint8_t pattern[ERR_PATTERN_LEN];
  bool camp;
  const char *err_str;
  err_inject_reg einj_reg;
} pmic_err_info;

static const uint8_t pmic_err_pattern_idx[ERR_PATTERN_LEN] = {0x5, 0x6, 0x8, 0x9, 0xA, 0xB, 0x33};
static const pmic_err_info pmic_err[MAX_PMIC_ERR_TYPE] = {
  // R05, R06,  R08,  R09,  R0A,	R0B,  R33,   CAMP,  err_str,        err_type,    uv/ov,   rail,         enable
  {{0x42, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00}, true,  STR_SWA_OV,     {TYPE_UNDEF, VOLT_OV, RAIL_SWA_OUT, ERR_INJ_ENABLE}},
  {{0x22, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00}, true,  STR_SWB_OV,     {TYPE_UNDEF, VOLT_OV, RAIL_SWB_OUT, ERR_INJ_ENABLE}},
  {{0x12, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00}, true,  STR_SWC_OV,     {TYPE_UNDEF, VOLT_OV, RAIL_SWC_OUT, ERR_INJ_ENABLE}},
  {{0x0A, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}, true,  STR_SWD_OV,     {TYPE_UNDEF, VOLT_OV, RAIL_SWD_OUT, ERR_INJ_ENABLE}},
  {{0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, true,  STR_BULK_OV,    {TYPE_UNDEF, VOLT_OV, RAIL_VIN_BULK, ERR_INJ_ENABLE}},
  {{0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00}, false, STR_MGMT_OV,    {TYPE_UNDEF, VOLT_OV, RAIL_VIN_MGMT, ERR_INJ_ENABLE}},
  {{0x42, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00}, true,  STR_SWA_UV,     {TYPE_UNDEF, VOLT_UV, RAIL_SWA_OUT, ERR_INJ_ENABLE}},
  {{0x22, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00}, true,  STR_SWB_UV,     {TYPE_UNDEF, VOLT_UV, RAIL_SWB_OUT, ERR_INJ_ENABLE}},
  {{0x12, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00}, true,  STR_SWC_UV,     {TYPE_UNDEF, VOLT_UV, RAIL_SWC_OUT, ERR_INJ_ENABLE}},
  {{0x0A, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00}, true,  STR_SWD_UV,     {TYPE_UNDEF, VOLT_UV, RAIL_SWD_OUT, ERR_INJ_ENABLE}},
  {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08}, true,  STR_BULK_UV,    {TYPE_UNDEF, VOLT_UV, RAIL_VIN_BULK, ERR_INJ_ENABLE}},
  {{0x00, 0x00, 0x00, 0x10, 0x02, 0x00, 0x00}, false, STR_SWITCHOVER, {TYPE_SWITCHOVER, VOLT_UNDEF, RAIL_UNDEF, ERR_INJ_ENABLE}},
  {{0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00}, false, STR_HIGH_TEMP,  {TYPE_HIGH_TEMP, VOLT_UNDEF, RAIL_UNDEF, ERR_INJ_ENABLE}},
  {{0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00}, false, STR_PG_1V8,     {TYPE_PG_1V8, VOLT_UNDEF, RAIL_UNDEF, ERR_INJ_ENABLE}},
  {{0x00, 0x00, 0x00, 0x0F, 0x02, 0x00, 0x00}, false, STR_HIGH_CURR,  {TYPE_HIGH_CURR, VOLT_UNDEF, RAIL_UNDEF, ERR_INJ_ENABLE}},
  {{0x00, 0x00, 0x00, 0x00, 0x02, 0xF0, 0x00}, false, STR_CURR_LIMIT, {TYPE_CURR_LIMIT, VOLT_UNDEF, RAIL_UNDEF, ERR_INJ_ENABLE}},
  {{0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00}, true,  STR_CRIT_TEMP,  {TYPE_CRIT_TEMP, VOLT_UNDEF, RAIL_UNDEF, ERR_INJ_ENABLE}}
};

int
pmic_err_index(const char *str) {
  int idx;

  for (idx = 0; idx < MAX_PMIC_ERR_TYPE; idx++) {
    if (strcmp(pmic_err[idx].err_str, str) == 0)
      return idx;
  }

  return -1;
}

int
pmic_err_name(uint8_t idx, char *str) {

  memcpy(str, pmic_err[idx].err_str, strlen(pmic_err[idx].err_str));
  return 0;
}


static int
pmic_rdwr_with_retry(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint8_t offset,
                     uint8_t tlen, uint8_t rlen, uint8_t *buf) {
  uint8_t i, retry, len = 1, xfer;
  int value = -1;
  int (*pF)(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t*);

  if (buf == NULL) {
    return -1;
  }

  if (rlen > 0) {
    pF = util_read_pmic;
    len = rlen;
  } else if (tlen > 0) {
    pF = util_write_pmic;
    len = tlen;
  } else {
    return -1;
  }

  for (i = 0; i < len;) {
    for (retry = 0; retry <= max_retries; retry++) {
      xfer = ((len - i) < MAX_DIMM_SMB_XFER_LEN) ? (len - i) : MAX_DIMM_SMB_XFER_LEN;
      value = pF(fru_id, cpu, dimm, offset + i, xfer, &buf[i]);
      if ((value >= 0) || (retry == max_retries)) {
        break;
      }
      usleep(retry_intvl * 1000);
    }
    if (value <= 0) {
      return -1;
    }

    i += xfer;
  }

  return 0;
}

int
pmic_list_err(uint8_t fru_id, uint8_t cpu, uint8_t dimm, const char **err_list, uint8_t *err_cnt) {
  uint8_t data[64] = {0};
  uint8_t err_idx, reg_idx;
  uint8_t reg, pattern;

  if (err_list == NULL || err_cnt == NULL) {
    return -1;
  }

  // read R05 ~ R0B
  if (pmic_rdwr_with_retry(fru_id, cpu, dimm, 0x05, 0, 7, &data[5]) != 0) {
    return -1;
  }

  // read R33
  if (pmic_rdwr_with_retry(fru_id, cpu, dimm, 0x33, 0, 1, &data[0x33]) != 0) {
    return -1;
  }

  *err_cnt = 0;
  for (err_idx = 0; err_idx < MAX_PMIC_ERR_TYPE; err_idx++) {
    // scan each byte of pattern
    for (reg_idx = 0; reg_idx < ERR_PATTERN_LEN; reg_idx++) {
      reg = pmic_err_pattern_idx[reg_idx];
      pattern = pmic_err[err_idx].pattern[reg_idx];
      if ((data[reg] & pattern) != pattern) {  // pattern not match
        break;
      }
    }
    if (reg_idx == ERR_PATTERN_LEN) {  // pattern match
      err_list[(*err_cnt)++] = pmic_err[err_idx].err_str;
    }
  }

  return 0;
}

int
pmic_inject_err(uint8_t fru_id, uint8_t cpu, uint8_t dimm, uint8_t option) {
  uint8_t data[8] = {0};

  // Read PMIC R2F register to get write protect mode status
  if (pmic_rdwr_with_retry(fru_id, cpu, dimm, 0x2F, 0, 1, data) != 0) {
    return -1;
  }
  if ((data[0] & (1 << PMIC_WRITE_PROTECT_BIT)) == 0) {
    printf("Inject failed! Please disable DIMM %s write protect before error injection\n", get_dimm_label(cpu, dimm));
    return -1;
  }
  memcpy(data, &pmic_err[option].einj_reg, 1);
  if (pmic_rdwr_with_retry(fru_id, cpu, dimm, PMIC_ERR_INJ_REG, 1, 0, data) != 0) {
    return -1;
  }

  return 0;
}

int
pmic_clear_err(uint8_t fru_id, uint8_t cpu, uint8_t dimm_num) {
  uint8_t data[1] = {0};
  int ret = 0;
  // Set R39 to clear registers R04 ~ R07
  data[0] = 0x74;
  if (pmic_rdwr_with_retry(fru_id, cpu, dimm_num, 0x39, 1, 0, data) != 0) {
    ret = -1;
  }
  // Set R14 to clear registers R08 ~ R0B, R33
  data[0] = 0x01;
  if (pmic_rdwr_with_retry(fru_id, cpu, dimm_num, 0x14, 1, 0, data) != 0) {
    ret = -1;
  }
  return ret;
}
