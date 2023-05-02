#include <stdio.h>
#include <syslog.h>
#include "pal.h"
#include <openbmc/obmc-i2c.h>
#include <openbmc/kv.h>

int
parse_mce_error_sel(uint8_t fru, uint8_t *event_data, char *error_log) {
  uint8_t *ed = &event_data[3];
  uint8_t bank_num;
  uint8_t error_type = ((ed[1] & 0x60) >> 5);
  char temp_log[512] = {0};

  strcpy(error_log, "");
  if ((ed[0] & 0x0F) == 0x0B) { //Uncorrectable
    switch (error_type) {
      case 0x00:
        strcat(error_log, "Uncorrected Recoverable Error, ");
        break;
      case 0x01:
        strcat(error_log, "Uncorrected Thread Fatal Error, ");
        break;
      case 0x02:
        strcat(error_log, "Uncorrected System Fatal Error, ");
        break;
      default:
        strcat(error_log, "Unknown, ");
        break;
    }
  } else if((ed[0] & 0x0F) == 0x0C) { //Correctable
    switch (error_type) {
      case 0x00:
        strcat(error_log, "Correctable Error, ");
        break;
      case 0x01:
        strcat(error_log, "Deferred Error, ");
        break;
      default:
        strcat(error_log, "Unknown, ");
        break;
    }
  }
  bank_num = ed[1] & 0x1F;
  snprintf(temp_log, sizeof(temp_log), "Bank Number %d, ", bank_num);
  strcat(error_log, temp_log);

  snprintf(temp_log, sizeof(temp_log), "CPU %d, Core %d", ((ed[2] & 0xE0) >> 5), (ed[2] & 0x1F));
  strcat(error_log, temp_log);

  return 0;
}

int
pal_parse_sel(uint8_t fru, uint8_t *sel, char *error_log)
{
  uint8_t snr_num = sel[11];
  uint8_t *event_data = &sel[10];
  bool parsed = true;

  strcpy(error_log, "");
  switch(snr_num) {
    case MACHINE_CHK_ERR:
      parse_mce_error_sel(fru, event_data, error_log);
      parsed = true;
      break;
    default:
      parsed = false;
      break;
  }

  if (parsed == true) {
    return 0;
  }

  pal_parse_sel_helper(fru, sel, error_log);
  return 0;
}

int
pal_control_mux_to_target_ch(uint8_t bus, uint8_t mux_addr, uint8_t channel)
{
  int ret = PAL_ENOTSUP;
  int fd;
  char fn[32];
  uint8_t retry;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if(fd < 0) {
    syslog(LOG_WARNING,"[%s] Cannot open bus %d", __func__, bus);
    ret = PAL_ENOTSUP;
    goto error_exit;
  }

  ret = pal_flock_retry(fd);
  if (ret == -1) {
   syslog(LOG_WARNING, "[%s] Failed to flock: %s", __func__, strerror(errno));
   goto error_exit;
  }

  tbuf[0] = 1 << channel;
  retry = 3;

  while(retry > 0) {
    ret = i2c_rdwr_msg_transfer(fd, mux_addr, tbuf, 1, rbuf, 0);
    if (PAL_EOK == ret){
      break;
    }
    msleep(50);
    retry--;
  }

  if(retry == 0) {
    syslog(LOG_WARNING,"[%s] Cannot switch the mux on bus %d", __func__, bus);
  }

  if (pal_unflock_retry(fd) == -1) {
   syslog(LOG_WARNING, "[%s] Failed to unflock: %s", __func__, strerror(errno));
  }

error_exit:
  if (fd > 0){
    close(fd);
  }

  return ret;
}

int
pal_clear_psb_cache(void) {
  char key[MAX_KEY_LEN] = {0};
  snprintf(key, sizeof(key), "slot%d_psb_config_raw", FRU_MB);
  return kv_del(key, KV_FPERSIST);
}
