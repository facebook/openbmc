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
pal_check_psb_error(uint8_t head, uint8_t last) {
  if(head == 0xEE) {
    switch (last) {
      case 0x03:
        syslog(LOG_CRIT, "PSB Event(EE0003) Error in BIOS Directory Table - Too many directory entries");
        break;
      case 0x04:
        syslog(LOG_CRIT, "PSB Event(EE0004) Internal error - Invalid parameter");
        break;
      case 0x05:
        syslog(LOG_CRIT, "PSB Event(EE0005) Internal error - Invalid data length");
        break;
      case 0x0B:
        syslog(LOG_CRIT, "PSB Event(EE000B) Internal error - Out of resources");
        break;
      case 0x10:
        syslog(LOG_CRIT, "PSB Event(EE0010) Error in BIOS Directory Table - Reset image destination invalid");
        break;
      case 0x13:
        syslog(LOG_CRIT, "PSB Event(EE0013) Error retrieving FW header during FW validation");
        break;
      case 0x14:
        syslog(LOG_CRIT, "PSB Event(EE0014) Invalid key size");
        break;
      case 0x18:
        syslog(LOG_CRIT, "PSB Event(EE0018) Error validating binary");
        break;
      case 0x22:
        syslog(LOG_CRIT, "PSB Event(EE0022) P0: BIOS signing key entry not found");
        break;
      case 0x3E:
        syslog(LOG_CRIT, "PSB Event(EE003E) P0: Error reading fuse info");
        break;
      case 0x62:
        syslog(LOG_CRIT, "PSB Event(EE0062) Internal error - Error mapping fuse");
        break;
      case 0x64:
        syslog(LOG_CRIT, "PSB Event(EE0064) P0: Timeout error attempting to fuse");
        break;
      case 0x69:
        syslog(LOG_CRIT, "PSB Event(EE0069) BIOS OEM key revoked");
        break;
      case 0x6C:
        syslog(LOG_CRIT, "PSB Event(EE006C) P0: Error in BIOS Directory Table - Reset image not found");
        break;
      case 0x6F:
        syslog(LOG_CRIT, "PSB Event(EE006F) P0: OEM BIOS Signing Key Usage flag violation");
        break;
      case 0x78:
        syslog(LOG_CRIT, "PSB Event(EE0078) P0: BIOS RTM Signature entry not found");
        break;
      case 0x79:
        syslog(LOG_CRIT, "PSB Event(EE0079) P0: BIOS Copy to DRAM failed");
        break;
      case 0x7A:
        syslog(LOG_CRIT, "PSB Event(EE007A) P0: BIOS RTM Signature verification failed");
        break;
      case 0x7B:
        syslog(LOG_CRIT, "PSB Event(EE007B) P0: OEM BIOS Signing Key failed signature verification");
        break;
      case 0x7C:
        syslog(LOG_CRIT, "PSB Event(EE007C) P0: Platform Vendor ID and/or Model ID binding violation");
        break;
      case 0x7D:
        syslog(LOG_CRIT, "PSB Event(EE007D) P0: BIOS Copy bit is unset for reset image");
        break;
      case 0x7E:
        syslog(LOG_CRIT, "PSB Event(EE007E) P0: Requested fuse is already blown, reblow will cause ASIC malfunction");
        break;
      case 0x7F:
        syslog(LOG_CRIT, "PSB Event(EE007F) P0: Error with actual fusing operation");
        break;
      case 0x80:
        syslog(LOG_CRIT, "PSB Event(EE0080) P1: Error reading fuse info");
        break;
      case 0x81:
        syslog(LOG_CRIT, "PSB Event(EE0081) P1: Platform Vendor ID and/or Model ID binding violation");
        break;
      case 0x82:
        syslog(LOG_CRIT, "PSB Event(EE0082) P1: Requested fuse is already blown, reblow will cause ASIC malfunction");
        break;
      case 0x83:
        syslog(LOG_CRIT, "PSB Event(EE0083) P1: Error with actual fusing operation");
        break;
      case 0x92:
        syslog(LOG_CRIT, "PSB Event(EE0092) P0: Error in BIOS Directory Table - Firmware type not found");
        break;
      default:
        break;
    }
  }

  return 0;
}

int pal_udbg_get_frame_total_num() {
  return 5;
}

int
pal_clear_psb_cache(void) {
  char key[MAX_KEY_LEN] = {0};
  snprintf(key, sizeof(key), "slot%d_psb_config_raw", FRU_MB);
  return kv_del(key, KV_FPERSIST);
}
