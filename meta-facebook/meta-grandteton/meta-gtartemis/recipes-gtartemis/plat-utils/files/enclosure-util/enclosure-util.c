/*
 * enclosure-util
 * Copyright 2023-present Facebook. All Rights Reserved.
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
#include <syslog.h>
#include <string.h>
#include <openbmc/hal_fruid.h>
#include <openbmc/nvme-mi.h>
#include <openbmc/pal.h>
#include <libpldm-oem/pldm.h>

#define CMD_DRIVE_STATUS 0
#define CMD_DRIVE_HEALTH 1
#define NVME_BAD_HEALTH 1
#define NVME_SFLGS_MASK_BIT 0x28          // check bit 5,3
#define NVME_SMART_WARNING_MASK_BIT 0x1F  // check bit 0~4
#define MAX_SERIAL_NUM 20
#define SPE_SSD_NUM 6
#define MAX_PART_NUM 40

static void
print_usage_help(void) {
  printf("Usage: enclosure-util cb --drive-status all\n");
  printf("       enclosure-util cb --drive-health\n");
  printf("       enclosure-util cb_accl[1..12] --drive-status <all|dev1|dev2>\n");
  printf("       enclosure-util cb_accl[1..12] --drive-health\n");
  printf("       enclosure-util mc --drive-status <all|ssd1|ssd2|ssd3|ssd4>\n");
  printf("       enclosure-util mc --drive-health\n");
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
  t_key_value_pair asic_core_vol3_decoding;
  t_key_value_pair asic_core_vol4_decoding;
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

  printf("\n");
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

  nvme_temp_decode(ssd->temp, &temp_decoding);
  printf("%s: %s\n", temp_decoding.key, temp_decoding.value);
  
  nvme_pdlu_decode(ssd->pdlu, &pdlu_decoding);
  printf("%s: %s\n", pdlu_decoding.key, pdlu_decoding.value);

  nvme_vendor_decode(ssd->vendor, &vendor_decoding);
  printf("%s: %s\n", vendor_decoding.key, vendor_decoding.value);

  nvme_serial_num_decode(ssd->serial_num, &sn_decoding);
  printf("%s: %s\n", sn_decoding.key, sn_decoding.value);

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
      if (ssd->vendor == VENDOR_ID_BROARDCOM) {
        nvme_total_fw_version_decode(ssd->block_len_ver_area, ssd->fw_major_ver, ssd->fw_minor_ver, ssd->fw_additional_ver, &fw_version_decoding);
      } else {
        nvme_fw_version_decode(ssd->block_len_ver_area, ssd->fw_major_ver, ssd->fw_minor_ver, &fw_version_decoding);
      }
      printf("%s: %s\n", fw_version_decoding.key, fw_version_decoding.value);

      if (ssd->vendor == VENDOR_ID_BROARDCOM) {
        nvme_monitor_area_decode("ASIC Core1 Voltage", ssd->block_len_mon_area, ssd->asic_core_vol1, BRCM_VP_ASIC_CORE_VOL_UNIT, &asic_core_vol1_decoding);
        printf("%s: %s\n", asic_core_vol1_decoding.key, asic_core_vol1_decoding.value);
        nvme_monitor_area_decode("ASIC Core2 Voltage", ssd->block_len_mon_area, ssd->asic_core_vol2, BRCM_VP_ASIC_CORE_VOL_UNIT, &asic_core_vol2_decoding);
        printf("%s: %s\n", asic_core_vol2_decoding.key, asic_core_vol2_decoding.value);
        nvme_monitor_area_decode("ASIC Core3 Voltage", ssd->block_len_mon_area, ssd->asic_core_vol3, BRCM_VP_ASIC_CORE_VOL_UNIT, &asic_core_vol3_decoding);
        printf("%s: %s\n", asic_core_vol3_decoding.key, asic_core_vol3_decoding.value);
        nvme_monitor_area_decode("ASIC Core4 Voltage", ssd->block_len_mon_area, ssd->asic_core_vol4, BRCM_VP_ASIC_CORE_VOL_UNIT, &asic_core_vol4_decoding);
        printf("%s: %s\n", asic_core_vol4_decoding.key, asic_core_vol4_decoding.value);
      } else {
        nvme_monitor_area_decode("ASIC Core1 Voltage", ssd->block_len_mon_area, ssd->asic_core_vol1, ASIC_CORE_VOL_UNIT, &asic_core_vol1_decoding);
        printf("%s: %s\n", asic_core_vol1_decoding.key, asic_core_vol1_decoding.value);
        nvme_monitor_area_decode("ASIC Core2 Voltage", ssd->block_len_mon_area, ssd->asic_core_vol2, ASIC_CORE_VOL_UNIT, &asic_core_vol2_decoding);
        printf("%s: %s\n", asic_core_vol2_decoding.key, asic_core_vol2_decoding.value);
        nvme_monitor_area_decode("Module Power Rail1 Voltage", ssd->block_len_mon_area, ssd->power_rail_vol1, POWER_RAIL_VOL_UNIT, &power_rail_vol1_decoding);
        printf("%s: %s\n", power_rail_vol1_decoding.key, power_rail_vol1_decoding.value);
        nvme_monitor_area_decode("Module Power Rail2 Voltage", ssd->block_len_mon_area, ssd->power_rail_vol2, POWER_RAIL_VOL_UNIT, &power_rail_vol2_decoding);
        printf("%s: %s\n", power_rail_vol2_decoding.key, power_rail_vol2_decoding.value);
      }

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
  // since accelerator doesn't implement SMART WARNING, do not check it.
  if (ssd->ffi_0 == FFI_0_ACCELERATOR) {
    printf("Accelerator Not Support SMART WARNING\n");
    return 0;
  }
  if ((ssd->warning & NVME_SMART_WARNING_MASK_BIT) != NVME_SMART_WARNING_MASK_BIT) {
    return NVME_BAD_HEALTH;
  }
  if ((ssd->sflgs & NVME_SFLGS_MASK_BIT) != NVME_SFLGS_MASK_BIT) {
    return NVME_BAD_HEALTH;
  }

  return 0;
}

static int
read_nvme_data(uint8_t fru_id, uint8_t bus, uint8_t eid, uint8_t dev_id, uint8_t cmd) {
  int ret = 0;
  int offset_base = 0;
  uint8_t dev_comp = 0;
  char dev_name[MAX_DEV_NAME] = {0};
  char fru_name[MAX_FRU_NAME] = {0};
  uint8_t tbuf[MAX_TXBUF_SIZE] = {0};
  uint8_t rbuf[MAX_RXBUF_SIZE] = {0};
  uint8_t tlen = 0;
  size_t rlen = 0;
  ssd_data ssd;

  ret = pal_get_fru_name(fru_id, fru_name);
  if (ret < 0) {
    printf("Get Fru name failed for Fru: %u\n", fru_id);
    return -1;
  }

  ret = pal_get_dev_name(fru_id, dev_id, dev_name);
  if (ret < 0) {
    printf("Get Dev name failed for Fru: %u Dev ID: %u\n", fru_id, dev_id);
    return -1;
  }

  if (fru_id == FRU_MEB) {
    dev_comp = dev_id;
  } else {
    // Fru is ACCL1 ~ ACCL12
    /* Tansfer fru id + device id into component id
     FRU component id
     ACCL1 = 18
     ...
     ACCL12 = 29
     ACCL1 DEV1 = 30
     ACCL1 DEV2 = 31
     ...
     ACCL12 DEV1 = 52
     ACCL12 DEV2 = 53
    
    e.g. ACCL1 DEV1
     ID = base(30) + 2*(fru(18) - FRU_ACB_ACCL1(18)) + (dev(1) - 1) = 30
    e.g. ACCL12 DEV2
     ID = base(30) + 2*(fru(29) - FRU_ACB_ACCL1(18)) + (dev(2) - 1) = 53
  */
    dev_comp = (DEV_ID_BASE + 2*(fru_id - FRU_ACB_ACCL1) + (dev_id - 1));
  }


  if (cmd == CMD_DRIVE_STATUS) {
    printf("%s %s: ", fru_name, dev_name);
    do {

      tbuf[0] = dev_comp;
      tbuf[1] = 8; //read back 8 bytes
      tbuf[2] = 0x00; // offset 0x00
      tlen = 3;

      ret = oem_pldm_ipmi_send_recv(bus, eid,
                                  NETFN_OEM_1S_REQ, CMD_OEM_1S_BIC_BRIDGE,
                                  tbuf, tlen,
                                  rbuf, &rlen, true);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): pldm send offset: %u, read length: %d failed", __func__, tbuf[3], rlen);
        printf("NA");
        break;
      }

      ssd.sflgs = rbuf[offset_base + 1];
      ssd.warning = rbuf[offset_base + 2];
      ssd.temp = rbuf[offset_base + 3];
      ssd.pdlu = rbuf[offset_base + 4];

      tbuf[1] = 24 + offset_base;;  //read back 24 bytes
      tbuf[2] = 0x08;  // offset 08
      ret = oem_pldm_ipmi_send_recv(bus, eid,
                                  NETFN_OEM_1S_REQ, CMD_OEM_1S_BIC_BRIDGE,
                                  tbuf, tlen,
                                  rbuf, &rlen, true);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): pldm send offset: %u, read length: %d failed", __func__,tbuf[3],rlen);
        printf("NA");
        break;
      }
      ssd.vendor = (rbuf[offset_base + 1] << 8) | rbuf[offset_base + 2];
      memcpy(ssd.serial_num, &rbuf[offset_base + 3], MAX_SERIAL_NUM);

      tbuf[1] = 55; //read back
      tbuf[2] = 0x20;  // offset 32
      ret = oem_pldm_ipmi_send_recv(bus, eid,
                                  NETFN_OEM_1S_REQ, CMD_OEM_1S_BIC_BRIDGE,
                                  tbuf, tlen,
                                  rbuf, &rlen, true);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): pldm send offset: %u, read length: %d failed", __func__,tbuf[3],rlen);
        printf("NA");
        break;
      }
      ssd.block_len_module_id_area = rbuf[offset_base + 0];
      ssd.fb_defined = rbuf[offset_base + 1];
      memcpy(ssd.part_num, &rbuf[offset_base + 2], MAX_PART_NUM);
      ssd.meff = rbuf[offset_base + 42];
      ssd.ffi_0 = rbuf[offset_base + 43];

        if (ssd.fb_defined == 1) {
          if (ssd.ffi_0 == FFI_0_ACCELERATOR) {
            tbuf[1] = 8; //read back
            tbuf[2] = 0x60;  // offset 96
            ret = oem_pldm_ipmi_send_recv(bus, eid,
                                          NETFN_OEM_1S_REQ, CMD_OEM_1S_BIC_BRIDGE,
                                          tbuf, tlen,
                                          rbuf, &rlen, true);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): oem_pldm_ipmi_send_recv offset=%d read length=%d failed", __func__,tbuf[3],rlen);
              break;
            }
            ssd.block_len_module_stat_area = rbuf[offset_base + 0];
            ssd.module_helath = rbuf[offset_base + 1];
            ssd.lower_theshold = rbuf[offset_base + 2];
            ssd.upper_threshold = rbuf[offset_base + 3];
            ssd.power_state = rbuf[offset_base + 4];
            ssd.i2c_freq = rbuf[offset_base + 5];
            ssd.tdp_level = rbuf[offset_base + 6];

            tbuf[1] = 8; //read back
            tbuf[2] = 0x68;  // offset 104
            ret = oem_pldm_ipmi_send_recv(bus, eid,
                                          NETFN_OEM_1S_REQ, CMD_OEM_1S_BIC_BRIDGE,
                                          tbuf, tlen,
                                          rbuf, &rlen, true);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): oem_pldm_ipmi_send_recv offset=%d read length=%d failed", __func__,tbuf[3],rlen);
              break;
            }

            ssd.block_len_ver_area = rbuf[offset_base + 0];
            ssd.asic_version = rbuf[offset_base + 1];
            ssd.fw_major_ver = rbuf[offset_base + 2];
            ssd.fw_minor_ver = rbuf[offset_base + 3];
            ssd.fw_additional_ver = rbuf[offset_base + 4];

            tbuf[1] = 10; //read back
            tbuf[2] = 0x70;  // offset 112
            ret = oem_pldm_ipmi_send_recv(bus, eid,
                                          NETFN_OEM_1S_REQ, CMD_OEM_1S_BIC_BRIDGE,
                                          tbuf, tlen,
                                          rbuf, &rlen, true);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): oem_pldm_ipmi_send_recv offset=%d read length=%d failed", __func__,tbuf[3],rlen);
              break;
            }

            ssd.block_len_mon_area = rbuf[offset_base + 0];
            ssd.asic_core_vol1 = (rbuf[offset_base + 1] << 8) | rbuf[offset_base + 2];
            ssd.asic_core_vol2 = (rbuf[offset_base + 3] << 8) | rbuf[offset_base + 4];
            if (ssd.vendor == VENDOR_ID_BROARDCOM) {
              ssd.asic_core_vol3 = (rbuf[offset_base + 5] << 8) | rbuf[offset_base + 6];
              ssd.asic_core_vol4 = (rbuf[offset_base + 7] << 8) | rbuf[offset_base + 8];
            } else {
              ssd.power_rail_vol1 = (rbuf[offset_base + 5] << 8) | rbuf[offset_base + 6];
              ssd.power_rail_vol2 = (rbuf[offset_base + 7] << 8) | rbuf[offset_base + 8];
            }

            tbuf[1] = 10; //read back
            if (ssd.vendor == VENDOR_ID_BROARDCOM) {
              tbuf[3] = 0x84;  // offset 132
            } else {
              tbuf[2] = 0x7A;  // offset 122
            }
            ret = oem_pldm_ipmi_send_recv(bus, eid,
                                          NETFN_OEM_1S_REQ, CMD_OEM_1S_BIC_BRIDGE,
                                          tbuf, tlen,
                                          rbuf, &rlen, true);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): oem_pldm_ipmi_send_recv offset=%d read length=%d failed", __func__,tbuf[3],rlen);
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
            tbuf[1] = 9; //read back
            tbuf[2] = 0x57;  // offset 87
            ret = oem_pldm_ipmi_send_recv(bus, eid,
                                          NETFN_OEM_1S_REQ, CMD_OEM_1S_BIC_BRIDGE,
                                          tbuf, tlen,
                                          rbuf, &rlen, true);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): bic_ipmb_send offset=%d read length=%d failed", __func__,tbuf[3],rlen);
              break;
            }
            ssd.ssd_ver = rbuf[1];
            ssd.ssd_capacity = (rbuf[2] << 8) | rbuf[3];
            ssd.ssd_pwr = rbuf[4];
            ssd.ssd_sinfo_0 = rbuf[5];
          }
        }
      drive_status(&ssd);
    } while (0);
    printf("\n");

  } else if (cmd == CMD_DRIVE_HEALTH) {
    tbuf[0] = dev_comp;
    tbuf[1] = 8; // read back 8 bytes
    tbuf[2] = 0x00; // offset 0x00
    tlen = 3;
    rlen = 0;

    ret = oem_pldm_ipmi_send_recv(bus, eid,
                                  NETFN_OEM_1S_REQ, CMD_OEM_1S_BIC_BRIDGE,
                                  tbuf, tlen,
                                  rbuf, &rlen, true);
    if (ret != 0) {
      printf("%s-%s : %s\n", fru_name, dev_name, "NA");
      return 0;
    }
    
    ssd.sflgs = rbuf[1];
    ssd.warning = rbuf[2];
    ret = drive_health(&ssd);  // ret 1: Abnormal  0:Normal
    printf("%s-%s : %s\n", fru_name, dev_name, (ret == 0)?"Normal":"Abnormal");
  } else {
    printf("%s(): unknown cmd \n", __func__);
    return -1;
  }

  return 0;
}




int
main(int argc, char **argv) {
  int ret;
  uint8_t fru_id = 0;
  uint8_t root_fru = 0;
  uint8_t dev_id = 0;
  uint8_t fru_index = 0, dev_index = 0;
  uint8_t fru_start = 0, fru_end = 0;
  uint8_t dev_start = 0, dev_end = 0;
  uint8_t status = 0;
  bic_intf fru_bic_info = {0};
  char dev_name[MAX_DEV_NAME];
  char fru_name[MAX_FRU_NAME];

  if (argc != 3 && argc != 4) {
    print_usage_help();
    return -1;
  }

  ret = pal_get_fru_id(argv[1], &fru_id);
  if (ret < 0) {
    printf("Wrong Fru: %s\n", argv[1]);
    print_usage_help();
    return -1;
  }

  ret = pal_get_root_fru(fru_id, &root_fru);
  if (ret < 0) {
    printf("Get root Fru failed: %u\n", fru_id);
    print_usage_help();
    return -1;
  }

  fru_bic_info.fru_id = fru_id;
  pal_get_bic_intf(&fru_bic_info);

  // need to check root present (ACB/MEB)
  if (pal_is_fru_prsnt(root_fru, &status) == 0) {
    if (status == FRU_NOT_PRSNT) {
      printf("Fru:%u Not Present\n", fru_id);
      return -1;
    }
  } else {
    printf("Fru:%u Get Present Failed\n", fru_id);
  }

  switch (fru_id) {
    case FRU_ACB:
      fru_start = FRU_ACB_ACCL1;
      fru_end = FRU_ACB_ACCL12;
      dev_start = DEV_ID1;
      dev_end = ACCL_DEV_NUM;
      break;
    case FRU_ACB_ACCL1:
    case FRU_ACB_ACCL2:
    case FRU_ACB_ACCL3:
    case FRU_ACB_ACCL4:
    case FRU_ACB_ACCL5:
    case FRU_ACB_ACCL6:
    case FRU_ACB_ACCL7:
    case FRU_ACB_ACCL8:
    case FRU_ACB_ACCL9:
    case FRU_ACB_ACCL10:
    case FRU_ACB_ACCL11:
    case FRU_ACB_ACCL12:
      fru_start = fru_id;
      fru_end = fru_id;
      dev_start = DEV_ID1;
      dev_end = ACCL_DEV_NUM;
      break;
    case FRU_MEB:
      fru_start = fru_id;
      fru_end = fru_id;
      dev_start = FRU_MEB_JCN5;
      dev_end =  FRU_MEB_JCN8;
      break;
    default:
      return -1;
  }

  if ((argc == 4) && !strcmp(argv[2], "--drive-status")) {
    if (strcmp(argv[3], "all") == 0) {
      for (fru_index = fru_start ; fru_index <= fru_end; fru_index ++) {
        for ( dev_index = dev_start ; dev_index <= dev_end; dev_index ++) {
          if (pal_is_fru_prsnt(fru_index, &status) == 0 && status == FRU_PRSNT ) {
            read_nvme_data(fru_index, fru_bic_info.bus_id, fru_bic_info.bic_eid, dev_index, CMD_DRIVE_STATUS);
          } else {
            ret = pal_get_fru_name(fru_index, fru_name);
            if (ret < 0) {
              printf("Get FRU name failed for Fru: %s Dev: %s\n", argv[1], argv[3]);
              continue;
            }
            ret = pal_get_dev_name(fru_index, dev_index, dev_name);
            if (ret < 0) {
              printf("Get FRU name failed for Fru: %s Dev: %s\n", argv[1], argv[3]);
              continue;
            }
            printf("%s-%s Not Present\n", fru_name, dev_name);
          }
        }
      }
    } else {
      if (fru_id == FRU_ACB) {
         print_usage_help();
         return -1;
      }
      ret = pal_get_dev_id(argv[3], &dev_id);
      if (ret < 0) {
        printf("Get Dev ID failed for Fru: %s Dev: %s\n", argv[1], argv[3]);
        print_usage_help();
        return -1;
      }
      read_nvme_data(fru_bic_info.fru_id, fru_bic_info.bus_id, fru_bic_info.bic_eid, dev_id, CMD_DRIVE_STATUS);
    }
  } else if ((argc == 3) && !strcmp(argv[2], "--drive-health")) {
    for (fru_index = fru_start ; fru_index <= fru_end; fru_index ++) {
      for ( dev_index = dev_start ; dev_index <= dev_end; dev_index ++) {
        if (pal_is_fru_prsnt(fru_index, &status) == 0 && status == FRU_PRSNT ) {
          read_nvme_data(fru_index, fru_bic_info.bus_id, fru_bic_info.bic_eid, dev_index, CMD_DRIVE_HEALTH);
        } else {
          ret = pal_get_fru_name(fru_index, fru_name);
          if (ret < 0) {
            printf("Get FRU name failed for Fru: %s Dev: %s\n", argv[1], argv[3]);
            continue;
          }
          ret = pal_get_dev_name(fru_index, dev_index, dev_name);
          if (ret < 0) {
            printf("Get FRU name failed for Fru: %s Dev: %s\n", argv[1], argv[3]);
            continue;
          }
          printf("%s-%s Not Present\n", fru_name, dev_name);
        }
      }
    }
  } else {
    print_usage_help();
    return -1;
  }
  
  return 0;
}
