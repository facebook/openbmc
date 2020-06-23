/*
 * enclosure-util
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <openbmc/nvme-mi.h>
#include <openbmc/pal.h>

#define MAX_DRIVE_NUM 2
#define MAX_GP_DRIVE_NUM 6
#define MAX_GPV2_DRIVE_NUM 12
#define CMD_DRIVE_STATUS 0
#define CMD_DRIVE_HEALTH 1
#define MAX_SERIAL_NUM 20
#define MAX_PART_NUM 40

#define I2C_DEV_GP_1 "/dev/i2c-1"
#define I2C_DEV_GP_3 "/dev/i2c-5"
#define I2C_GP_MUX_ADDR 0x71

/* NVMe-MI SSD Status Flag bit mask */
#define NVME_SFLGS_MASK_BIT 0x28          // check bit 5,3

/* NVMe-MI SSD SMART Critical Warning */
#define NVME_SMART_WARNING_MASK_BIT 0x1F  // check bit 0~4

#define SPRINGHILL_M2_OFFSET_BASE 1 // one byte for FBID

#define NVME_BAD_HEALTH 1

static uint8_t m_slot_id = 0;
static uint8_t m_slot_type = 0xFF;

static void
print_usage_help(void) {
  printf("Usage: enclosure-util <slot1|slot2|slot3|slot4> --drive-status <number, all>\n");
  printf("       enclosure-util <slot1|slot2|slot3|slot4> --drive-health\n");
}

static int
drive_status(ssd_data *ssd) {
  t_status_flags status_flag_decoding;
  t_smart_warning smart_warning_decoding;
  t_key_value_pair temp_decoding;
  t_key_value_pair pdlu_decoding;
  t_key_value_pair vendor_decoding;
  t_key_value_pair sn_decoding;
  t_key_value_pair pn_decoding;
  t_key_value_pair meff_decoding;
  t_key_value_pair ffi_0_decoding;
  t_key_value_pair sinfo_0_decoding;
  t_key_value_pair module_helath_decoding;
  t_key_value_pair lower_thermal_temp_decoding;
  t_key_value_pair upper_thermal_temp_decoding;
  t_key_value_pair power_state_decoding;
  t_key_value_pair i2c_freq_decoding;
  t_key_value_pair tdp_level_decoding;
  t_key_value_pair asic_version_decoding;
  t_key_value_pair fw_version_decoding;
  t_key_value_pair asic_core_vol1_decoding;
  t_key_value_pair asic_core_vol2_decoding;
  t_key_value_pair power_rail_vol1_decoding;
  t_key_value_pair power_rail_vol2_decoding;
  t_key_value_pair asic_error_type_decoding;
  t_key_value_pair module_error_type_decoding;
  t_key_value_pair warning_flag_decoding;
  t_key_value_pair interrupt_flag_decoding;
  t_key_value_pair max_asic_temp_decoding;
  t_key_value_pair total_int_mem_err_count_decoding;
  t_key_value_pair total_ext_mem_err_count_decoding;
  t_key_value_pair smbus_err_decoding;

  nvme_vendor_decode(ssd->vendor, &vendor_decoding);
  printf("%s: %s\n", vendor_decoding.key, vendor_decoding.value);

  nvme_serial_num_decode(ssd->serial_num, &sn_decoding);
  printf("%s: %s\n", sn_decoding.key, sn_decoding.value);

  nvme_temp_decode(ssd->temp, &temp_decoding);
  printf("%s: %s\n", temp_decoding.key, temp_decoding.value);

  nvme_pdlu_decode(ssd->pdlu, &pdlu_decoding);
  printf("%s: %s\n", pdlu_decoding.key, pdlu_decoding.value);

  nvme_sflgs_decode(ssd->sflgs, &status_flag_decoding);
  printf("%s: %s\n", status_flag_decoding.self.key, status_flag_decoding.self.value);
  printf("    %s: %s\n", status_flag_decoding.read_complete.key, status_flag_decoding.read_complete.value);
  printf("    %s: %s\n", status_flag_decoding.ready.key, status_flag_decoding.ready.value);
  printf("    %s: %s\n", status_flag_decoding.functional.key, status_flag_decoding.functional.value);
  printf("    %s: %s\n", status_flag_decoding.reset_required.key, status_flag_decoding.reset_required.value);
  printf("    %s: %s\n", status_flag_decoding.port0_link.key, status_flag_decoding.port0_link.value);
  printf("    %s: %s\n", status_flag_decoding.port1_link.key, status_flag_decoding.port1_link.value);

  nvme_smart_warning_decode(ssd->warning, &smart_warning_decoding);
  printf("%s: %s\n", smart_warning_decoding.self.key, smart_warning_decoding.self.value);
  printf("    %s: %s\n", smart_warning_decoding.spare_space.key, smart_warning_decoding.spare_space.value);
  printf("    %s: %s\n", smart_warning_decoding.temp_warning.key, smart_warning_decoding.temp_warning.value);
  printf("    %s: %s\n", smart_warning_decoding.reliability.key, smart_warning_decoding.reliability.value);
  printf("    %s: %s\n", smart_warning_decoding.media_status.key, smart_warning_decoding.media_status.value);
  printf("    %s: %s\n", smart_warning_decoding.backup_device.key, smart_warning_decoding.backup_device.value);

  if (ssd->fb_defined == 0x01) {
    nvme_part_num_decode(ssd->block_len_module_id_area, ssd->part_num, &pn_decoding);
    printf("%s: %s\n", pn_decoding.key, pn_decoding.value);
    nvme_meff_decode(ssd->block_len_module_id_area, ssd->meff, &meff_decoding);
    printf("%s: %s\n", meff_decoding.key, meff_decoding.value);
    nvme_ffi_0_decode(ssd->block_len_module_id_area, ssd->ffi_0, &ffi_0_decoding);
    printf("%s: %s\n", ffi_0_decoding.key, ffi_0_decoding.value);

    if (ssd->ffi_0 == FFI_0_ACCELERATOR) {
      nvme_raw_data_prase("Module health", ssd->block_len_module_stat_area, ssd->module_helath, &module_helath_decoding);
      printf("%s: %s\n", module_helath_decoding.key, module_helath_decoding.value);
      nvme_lower_threshold_temp_decode(ssd->block_len_module_stat_area, ssd->lower_theshold, &lower_thermal_temp_decoding);
      printf("%s: %s\n", lower_thermal_temp_decoding.key, lower_thermal_temp_decoding.value);
      nvme_upper_threshold_temp_decode(ssd->block_len_module_stat_area, ssd->upper_threshold, &upper_thermal_temp_decoding);
      printf("%s: %s\n", upper_thermal_temp_decoding.key, upper_thermal_temp_decoding.value);

      nvme_power_state_decode(ssd->block_len_module_stat_area, ssd->power_state, &power_state_decoding);
      printf("%s: %s\n", power_state_decoding.key, power_state_decoding.value);
      nvme_i2c_freq_decode (ssd->block_len_module_stat_area, ssd->i2c_freq, &i2c_freq_decoding);
      printf("%s: %s\n", i2c_freq_decoding.key, i2c_freq_decoding.value);
      nvme_tdp_level_decode (ssd->block_len_module_stat_area, ssd->tdp_level, &tdp_level_decoding);
      printf("%s: %s\n", tdp_level_decoding.key, tdp_level_decoding.value);

      nvme_raw_data_prase("ASIC version", ssd->block_len_ver_area, ssd->asic_version, &asic_version_decoding);
      printf("%s: %s\n", asic_version_decoding.key, asic_version_decoding.value);
      nvme_fw_version_decode(ssd->block_len_ver_area, ssd->fw_major_ver, ssd->fw_minor_ver, &fw_version_decoding);
      printf("%s: %s\n", fw_version_decoding.key, fw_version_decoding.value);

      nvme_monitor_area_decode("ASIC Core1 Voltage", ssd->block_len_mon_area, ssd->asic_core_vol1, ASIC_CORE_VOL_UNIT, &asic_core_vol1_decoding);
      printf("%s: %s\n", asic_core_vol1_decoding.key, asic_core_vol1_decoding.value);
      nvme_monitor_area_decode("ASIC Core2 Voltage", ssd->block_len_mon_area, ssd->asic_core_vol2, ASIC_CORE_VOL_UNIT, &asic_core_vol2_decoding);
      printf("%s: %s\n", asic_core_vol2_decoding.key, asic_core_vol2_decoding.value);
      nvme_monitor_area_decode("Module Power Rail1 Voltage", ssd->block_len_mon_area, ssd->power_rail_vol1, POWER_RAIL_VOL_UNIT, &power_rail_vol1_decoding);
      printf("%s: %s\n", power_rail_vol1_decoding.key, power_rail_vol1_decoding.value);
      nvme_monitor_area_decode("Module Power Rail2 Voltage", ssd->block_len_mon_area, ssd->power_rail_vol2, POWER_RAIL_VOL_UNIT, &power_rail_vol2_decoding);
      printf("%s: %s\n", power_rail_vol2_decoding.key, power_rail_vol2_decoding.value);

      nvme_raw_data_prase("ASIC Error Type Report", ssd->block_len_err_ret_area, ssd->asic_error_type, &asic_error_type_decoding);
      printf("%s: %s\n", asic_error_type_decoding.key, asic_error_type_decoding.value);
      nvme_raw_data_prase("Module Error Type Report", ssd->block_len_err_ret_area, ssd->module_error_type, &module_error_type_decoding);
      printf("%s: %s\n", module_error_type_decoding.key, module_error_type_decoding.value);
      nvme_raw_data_prase("Warning Flag", ssd->block_len_err_ret_area, ssd->warning_flag, &warning_flag_decoding);
      printf("%s: %s\n", warning_flag_decoding.key, warning_flag_decoding.value);
      nvme_raw_data_prase("Interrupt Flag", ssd->block_len_err_ret_area, ssd->interrupt_flag, &interrupt_flag_decoding);
      printf("%s: %s\n", interrupt_flag_decoding.key, interrupt_flag_decoding.value);
      nvme_max_asic_temp_decode(ssd->block_len_err_ret_area, ssd->max_asic_temp, &max_asic_temp_decoding);
      printf("%s: %s\n", max_asic_temp_decoding.key, max_asic_temp_decoding.value);
      nvme_total_int_mem_err_count_decode(ssd->block_len_err_ret_area, ssd->total_int_mem_err_count, &total_int_mem_err_count_decoding);
      printf("%s: %s\n", total_int_mem_err_count_decoding.key, total_int_mem_err_count_decoding.value);
      nvme_total_ext_mem_err_count_decode(ssd->block_len_err_ret_area, ssd->total_ext_mem_err_count, &total_ext_mem_err_count_decoding);
      printf("%s: %s\n", total_ext_mem_err_count_decoding.key, total_ext_mem_err_count_decoding.value);
      nvme_smbus_err_decode(ssd->block_len_err_ret_area, ssd->smbus_err, &smbus_err_decoding);
      printf("%s: %s\n", smbus_err_decoding.key, smbus_err_decoding.value);
    } else if (ssd->ffi_0 == FFI_0_STORAGE) {
      printf("%s: 0x%02X\n", "Storage version", ssd->ssd_ver);
      printf("%s: %d GB\n", "Storage Capacity", ssd->ssd_capacity);
      printf("%s: %d W\n", "Storage Power", ssd->ssd_pwr);
      nvme_sinfo_0_decode (ssd->ssd_sinfo_0,&sinfo_0_decoding);
      printf("%s: %s\n", sinfo_0_decoding.key, sinfo_0_decoding.value);
    }
  }

  return 0;
}

static int
drive_health(ssd_data *ssd) {
  if (ssd->fb_defined != 0x01 || ssd->ffi_0 != FFI_0_ACCELERATOR) {
    // since accelerator doesn't implement SMART WARNING, do not check it.
    if ((ssd->warning & NVME_SMART_WARNING_MASK_BIT) != NVME_SMART_WARNING_MASK_BIT)
      return NVME_BAD_HEALTH;
  }
  if ((ssd->sflgs & NVME_SFLGS_MASK_BIT) != NVME_SFLGS_MASK_BIT)
    return NVME_BAD_HEALTH;

  return 0;
}

static int
read_bic_nvme_data(uint8_t slot_id, uint8_t drv_num, uint8_t cmd) {
  int ret = 0, offset_base = 0;
  int rlen = 0;
  float tdp_val = 0;
  uint8_t bus, wbuf[8], rbuf[64];
  char stype_str[32] = {0};
  ssd_data ssd;

  if (m_slot_type == SLOT_TYPE_SERVER) {
    bus = 0x3;
    wbuf[0] = 1 << (drv_num - 1);
    sprintf(stype_str, "Server Board");
  } else {  // SLOT_TYPE_GPV2
    bus = (2 + drv_num/2) * 2 + 1;
    wbuf[0] = 1 << (drv_num%2);
    sprintf(stype_str, "Glacier Point V2");
  }

  // MUX
  ret = bic_master_write_read(slot_id, bus, 0xe2, wbuf, 1, rbuf, 0);
  if (ret != 0) {
    syslog(LOG_DEBUG, "%s(): bic_master_write_read failed", __func__);
    return ret;
  }

  if (cmd == CMD_DRIVE_STATUS) {
    memset(&ssd, 0x00, sizeof(ssd_data));
    printf("%s %u Drive%d\n", stype_str, slot_id, drv_num);

    do {
      wbuf[0] = 0x00;  // offset 00
      rlen = 8;
      ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
        break;
      }

      // Judge whether this device is SpringHill M.2. via FBID.
      // TODO: If SpringHill M.2. can follow NVMe-MI I2C transaction, we will remove this judgement.
      if (rbuf[0] == wbuf[0]) {
        offset_base = SPRINGHILL_M2_OFFSET_BASE;
      }
      ssd.sflgs = rbuf[offset_base + 1];
      ssd.warning = rbuf[offset_base + 2];
      ssd.temp = rbuf[offset_base + 3];
      ssd.pdlu = rbuf[offset_base + 4];

      wbuf[0] = 0x08;  // offset 08
      rlen = 24 + offset_base;
      ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
        break;
      }
      ssd.vendor = (rbuf[offset_base + 1] << 8) | rbuf[offset_base + 2];
      memcpy(ssd.serial_num, &rbuf[offset_base + 3], MAX_SERIAL_NUM);

      if (m_slot_type == SLOT_TYPE_GPV2) {
        wbuf[0] = 0x20;  // offset 32
        rlen = 55 + offset_base;
        ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
        if (ret != 0) {
          syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
          break;
        }
        ssd.block_len_module_id_area = rbuf[offset_base + 0];
        ssd.fb_defined = rbuf[offset_base + 1];
        memcpy(ssd.part_num, &rbuf[offset_base + 2], MAX_PART_NUM);
        ssd.meff = rbuf[offset_base + 42];
        ssd.ffi_0 = rbuf[offset_base + 43];

        if (ssd.fb_defined == 1) {
          if (ssd.ffi_0 == FFI_0_ACCELERATOR) {
            wbuf[0] = 0x60;  // offset 96
            rlen = 8 + offset_base;
            ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
              break;
            }
            ssd.block_len_module_stat_area = rbuf[offset_base + 0];
            ssd.module_helath = rbuf[offset_base + 1];
            ssd.lower_theshold = rbuf[offset_base + 2];
            ssd.upper_threshold = rbuf[offset_base + 3];
            ssd.power_state = rbuf[offset_base + 4];
            ssd.i2c_freq = rbuf[offset_base + 5];

            // Formula provided by SPH
            if (ssd.vendor == VENDOR_ID_INTEL) {
              tdp_val = rbuf[offset_base + 7] + rbuf[offset_base + 6] / 250;
              if (tdp_val > 13) {
                ssd.tdp_level = TDP_LEVEL3;
              } else if (tdp_val >= 10.5) {
                ssd.tdp_level = TDP_LEVEL2;
              } else {
                ssd.tdp_level = TDP_LEVEL1;
              } 
            } else {
              ssd.tdp_level = rbuf[offset_base + 6];
            }

            wbuf[0] = 0x68;  // offset 104
            rlen = 8 + offset_base;
            ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
              break;
            }
            ssd.block_len_ver_area = rbuf[offset_base + 0];
            ssd.asic_version = rbuf[offset_base + 1];
            ssd.fw_major_ver = rbuf[offset_base + 2];
            ssd.fw_minor_ver = rbuf[offset_base + 3];

            wbuf[0] = 0x70;  // offset 112
            rlen = 10 + offset_base;
            ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
              break;
            }
            ssd.block_len_mon_area = rbuf[offset_base + 0];
            ssd.asic_core_vol1 = (rbuf[offset_base + 1] << 8) | rbuf[offset_base + 2];
            ssd.asic_core_vol2 = (rbuf[offset_base + 3] << 8) | rbuf[offset_base + 4];
            ssd.power_rail_vol1 = (rbuf[offset_base + 5] << 8) | rbuf[offset_base + 6];
            ssd.power_rail_vol2 = (rbuf[offset_base + 7] << 8) | rbuf[offset_base + 8];

            wbuf[0] = 0x7A;  // offset 122
            rlen = 10 + offset_base;
            ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
              break;
            }
            ssd.block_len_err_ret_area = rbuf[offset_base + 0];
            ssd.asic_error_type = rbuf[offset_base + 1];
            ssd.module_error_type = rbuf[offset_base + 2];
            ssd.warning_flag = rbuf[offset_base + 3];
            ssd.interrupt_flag = rbuf[offset_base + 4];
            ssd.max_asic_temp = rbuf[offset_base + 5];
            ssd.total_int_mem_err_count = rbuf[offset_base + 6];
            ssd.total_ext_mem_err_count = rbuf[offset_base + 7];
            ssd.smbus_err = rbuf[offset_base + 8];
          } else if (ssd.ffi_0 == FFI_0_STORAGE) {
            wbuf[0] = 0x57;  // offset 87
            rlen = 9;
            ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
              break;
            }
            ssd.ssd_ver = rbuf[1];
            ssd.ssd_capacity = (rbuf[2] << 8) | rbuf[3];
            ssd.ssd_pwr = rbuf[4];
            ssd.ssd_sinfo_0 = rbuf[5];
          }
        }
      }

      drive_status(&ssd);
    } while (0);

    printf("\n");
    return ret;
  }
  else if (cmd == CMD_DRIVE_HEALTH) {
    memset(&ssd, 0x00, sizeof(ssd_data));

    wbuf[0] = 0x00;  // offset 00
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, 8);
    if (ret != 0) {
      syslog(LOG_DEBUG, "%s(): bic_master_write_read failed", __func__);
      return ret;
    }
    // Judge whether this device is SpringHill M.2. via FBID.
    // TODO: If SpringHill M.2. can follow NVMe-MI I2C transaction, we will remove this judgement.
    if (rbuf[1] == wbuf[0]) {
      offset_base = SPRINGHILL_M2_OFFSET_BASE;
    }
    ssd.sflgs = rbuf[offset_base + 1];
    ssd.warning = rbuf[offset_base + 2];

    if (m_slot_type == SLOT_TYPE_GPV2) {
      wbuf[0] = 0x20;  // offset 32
      rlen = 55 + offset_base;
      ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, rlen);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,wbuf[0],rlen);
        return ret;
      }
      ssd.fb_defined = rbuf[offset_base + 1];
      ssd.ffi_0 = rbuf[offset_base + 43];
    } else {
      ssd.fb_defined = 0;
      ssd.ffi_0 = 0;
    }

    ret = drive_health(&ssd);
    return ret;
  }
  else {
    syslog(LOG_DEBUG, "%s(): unknown cmd", __func__);
    return -1;
  }

  return 0;
}

static int
read_gp_nvme_data(uint8_t slot_id, uint8_t drv_num, uint8_t cmd) {
  int ret = 0;
  uint8_t channel = 0;
  char *device;
  ssd_data ssd;

  // MUX
  switch (drv_num) {
    case 0:
      channel = MUX_CH_1;
      break;
    case 1:
      channel = MUX_CH_0;
      break;
    case 2:
      channel = MUX_CH_4;
      break;
    case 3:
      channel = MUX_CH_3;
      break;
    case 4:
      channel = MUX_CH_2;
      break;
    case 5:
      channel = MUX_CH_5;
      break;
    default:
      return -1;
  }

  device = (slot_id == 1) ? I2C_DEV_GP_1 : I2C_DEV_GP_3;
  ret = fby2_mux_control(device, I2C_GP_MUX_ADDR, channel);
  if (ret != 0) {
     syslog(LOG_DEBUG, "%s: fby2_mux_control failed", __func__);
     return ret;
  }

  if (cmd == CMD_DRIVE_STATUS) {
    memset(&ssd, 0x00, sizeof(ssd_data));
    printf("Glacier Point %u Drive%d\n", slot_id, drv_num);

    do {
      ret = nvme_sflgs_read(device, &ssd.sflgs);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): nvme_sflgs_read failed", __func__);
        break;
      }

      ret = nvme_smart_warning_read(device, &ssd.warning);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): nvme_smart_warning_read failed", __func__);
        break;
      }

      ret = nvme_temp_read(device, &ssd.temp);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): nvme_temp_read failed", __func__);
        break;
      }

      ret = nvme_pdlu_read(device, &ssd.pdlu);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): nvme_pdlu_read failed", __func__);
        break;
      }

      ret = nvme_vendor_read(device, &ssd.vendor);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): nvme_vendor_read failed", __func__);
        break;
      }

      ret = nvme_serial_num_read(device, ssd.serial_num, MAX_SERIAL_NUM);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): nvme_serial_num_read failed", __func__);
        break;
      }

      drive_status(&ssd);
    } while (0);

    printf("\n");
    return ret;
  }
  else if (cmd == CMD_DRIVE_HEALTH) {
    memset(&ssd, 0x00, sizeof(ssd_data));

    if (nvme_sflgs_read(device, &ssd.sflgs)) {
      syslog(LOG_DEBUG, "%s(): nvme_sflgs_read failed", __func__);
      return -1;
    }

    if (nvme_smart_warning_read(device, &ssd.warning)) {
      syslog(LOG_DEBUG, "%s(): nvme_smart_warning_read failed", __func__);
      return -1;
    }

    ret = drive_health(&ssd);
    return ret;
  }
  else {
    syslog(LOG_DEBUG, "%s(): unknown cmd", __func__);
    return -1;
  }

  return 0;
}

static void
ssd_monitor_enable(uint8_t slot_id, uint8_t slot_type, uint8_t en) {
  if ((slot_type == SLOT_TYPE_SERVER) || (slot_type == SLOT_TYPE_GPV2)) {
    if (en) {  // enable sensor monitor
      bic_disable_sensor_monitor(slot_id, 0);
    } else {   // disable sensor monitor
      bic_disable_sensor_monitor(slot_id, 1);
      msleep(100);
    }
  }
  else if (slot_type == SLOT_TYPE_GP) {
    if (en) {  // enable sensor monitor
      fby2_disable_gp_m2_monior(slot_id, 0);
    } else {   // disable sensor monitor
      fby2_disable_gp_m2_monior(slot_id, 1);
      msleep(100);
    }
  }
}

static void
enclosure_sig_handler(int sig) {
  if (m_slot_id) {
    ssd_monitor_enable(m_slot_id, m_slot_type, 1);
  }
}

int
main(int argc, char **argv) {
  int ret;
  uint8_t slot_id, slot_type = 0xFF;
  uint8_t i, drv_start = 1, drv_end = MAX_DRIVE_NUM;
  char *end = NULL;
  int (*read_nvme_data)(uint8_t,uint8_t,uint8_t);
  struct sigaction sa;

  if (argc < 3) {
    print_usage_help();
    return -1;
  }

  if (!strcmp(argv[1], "slot1")) {
    slot_id = 1;
  } else if (!strcmp(argv[1] , "slot2")) {
    slot_id = 2;
  } else if (!strcmp(argv[1] , "slot3")) {
    slot_id = 3;
  } else if (!strcmp(argv[1] , "slot4")) {
    slot_id = 4;
  } else {
    print_usage_help();
    return -1;
  }

  slot_type = fby2_get_slot_type(slot_id);
  if (slot_type == SLOT_TYPE_SERVER) {
    drv_start = 1;  // 1-based
    drv_end = MAX_DRIVE_NUM;
    read_nvme_data = read_bic_nvme_data;
  } else if (slot_type == SLOT_TYPE_GP) {
    drv_start = 0;  // 0-based
    drv_end = MAX_GP_DRIVE_NUM - 1;
    read_nvme_data = read_gp_nvme_data;
  } else if (slot_type == SLOT_TYPE_GPV2) {
    drv_start = 0;  // 0-based
    drv_end = MAX_GPV2_DRIVE_NUM - 1;
    read_nvme_data = read_bic_nvme_data;
  } else {
    return -1;
  }
  m_slot_type = slot_type;
  m_slot_id = slot_id;

  sa.sa_handler = enclosure_sig_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  if ((argc == 4) && !strcmp(argv[2], "--drive-status")) {
    if (!strcmp(argv[3], "all")) {
      ssd_monitor_enable(slot_id, slot_type, 0);
      for (i = drv_start; i <= drv_end; i++) {
        read_nvme_data(slot_id, i, CMD_DRIVE_STATUS);
      }
      ssd_monitor_enable(slot_id, slot_type, 1);
    }
    else {
      errno = 0;
      ret = strtol(argv[3], &end, 0);
      if (errno || *end || (ret < drv_start) || (ret > drv_end)) {
        print_usage_help();
        return -1;
      }

      ssd_monitor_enable(slot_id, slot_type, 0);
      read_nvme_data(slot_id, (uint8_t)ret, CMD_DRIVE_STATUS);
      ssd_monitor_enable(slot_id, slot_type, 1);
    }
  }
  else if ((argc == 3) && !strcmp(argv[2], "--drive-health")) {
    ssd_monitor_enable(slot_id, slot_type, 0);
    for (i = drv_start; i <= drv_end; i++) {
      ret = read_nvme_data(slot_id, i, CMD_DRIVE_HEALTH);
      printf("M.2-%u: %s\n", i, (ret == 0)?"Normal":((ret == NVME_BAD_HEALTH)?"Abnormal":"NA"));
    }
    ssd_monitor_enable(slot_id, slot_type, 1);
  }
  else {
    print_usage_help();
    return -1;
  }

  return 0;
}
