#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "dimm.h"
#include "dimm-util-plat.h"

const uint8_t pmic_err_pattern_idx[ERR_PATTERN_LEN] = {0x5, 0x6, 0x8, 0x9, 0xA, 0xB};
pmic_err_info pmic_err[MAX_PMIC_ERR_TYPE] = {
  // R05, R06,  R08,  R09,  R0A,	R0B,   CAMP,  err_str,        err_type,    uv/ov,   rail,         enable
  {{0x02, 0x08, 0x00, 0x00, 0x82, 0x00}, true,  STR_SWA_OV,     {TYPE_UNDEF, VOLT_OV, RAIL_SWA_OUT, ERR_INJ_ENABLE}},
  {{0x02, 0x02, 0x00, 0x00, 0x22, 0x00}, true,  STR_SWB_OV,     {TYPE_UNDEF, VOLT_OV, RAIL_SWB_OUT, ERR_INJ_ENABLE}},
  {{0x02, 0x01, 0x00, 0x00, 0x12, 0x00}, true,  STR_SWC_OV,     {TYPE_UNDEF, VOLT_OV, RAIL_SWC_OUT, ERR_INJ_ENABLE}},
  {{0x04, 0x00, 0x01, 0x00, 0x02, 0x00}, true,  STR_BULK_OV,    {TYPE_UNDEF, VOLT_OV, RAIL_VIN_BULK, ERR_INJ_ENABLE}},
  {{0x02, 0x80, 0x00, 0x00, 0x02, 0x08}, true,  STR_SWA_UV,     {TYPE_UNDEF, VOLT_UV, RAIL_SWA_OUT, ERR_INJ_ENABLE}},
  {{0x02, 0x20, 0x00, 0x00, 0x02, 0x02}, true,  STR_SWB_UV,     {TYPE_UNDEF, VOLT_UV, RAIL_SWB_OUT, ERR_INJ_ENABLE}},
  {{0x02, 0x10, 0x00, 0x00, 0x02, 0x01}, true,  STR_SWC_UV,     {TYPE_UNDEF, VOLT_UV, RAIL_SWC_OUT, ERR_INJ_ENABLE}},
  {{0x00, 0x00, 0x00, 0x80, 0x02, 0x00}, false, STR_HIGH_TEMP,  {TYPE_HIGH_TEMP, VOLT_UNDEF, RAIL_UNDEF, ERR_INJ_ENABLE}},
  {{0x00, 0x00, 0x00, 0x20, 0x02, 0x00}, false, STR_PG_1V8,     {TYPE_PG_1V8, VOLT_UNDEF, RAIL_UNDEF, ERR_INJ_ENABLE}},
  {{0x00, 0x00, 0x00, 0x0B, 0x02, 0x00}, false, STR_HIGH_CURR,  {TYPE_HIGH_CURR, VOLT_UNDEF, RAIL_UNDEF, ERR_INJ_ENABLE}},
  {{0x00, 0x00, 0x00, 0x00, 0x02, 0xB0}, false, STR_CURR_LIMIT, {TYPE_CURR_LIMIT, VOLT_UNDEF, RAIL_UNDEF, ERR_INJ_ENABLE}},
  {{0x03, 0x00, 0x40, 0x00, 0x02, 0x00}, true,  STR_CRIT_TEMP,  {TYPE_CRIT_TEMP, VOLT_UNDEF, RAIL_UNDEF, ERR_INJ_ENABLE}}
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

  if (err_list == NULL || err_cnt == NULL) {
    return -1;
  }

  // read R05 ~ R0B
  if (pmic_rdwr_with_retry(fru_id, cpu, dimm, 0x05, 0, 7, &data[5]) != 0) {
    return -1;
  }

  *err_cnt = 0;
  for (err_idx = 0; err_idx < MAX_PMIC_ERR_TYPE; err_idx++) {
    // scan each byte of pattern
    for (reg_idx = 0; reg_idx < ERR_PATTERN_LEN; reg_idx++) {
      uint8_t reg = pmic_err_pattern_idx[reg_idx];
      uint8_t pattern = pmic_err[err_idx].pattern[reg_idx];
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

  // Set R35 to clear registers R04 ~ R07
  data[0] = 0;
  if (pmic_rdwr_with_retry(fru_id, cpu, dimm_num, 0x35, 1, 0, data) != 0) {
    ret = -1;
  }
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
