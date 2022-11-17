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
#include <facebook/fby3_common.h>

#define CMD_DRIVE_STATUS 0
#define CMD_DRIVE_HEALTH 1
#define NVME_BAD_HEALTH 1
#define NVME_SFLGS_MASK_BIT 0x28          // check bit 5,3
#define NVME_SMART_WARNING_MASK_BIT 0x1F  // check bit 0~4
#define MAX_SERIAL_NUM 20
#define SPE_SSD_NUM 6
#define MAX_PART_NUM 40

char path[64] = {0};
int fd;
static uint8_t m_slot_id = 0;
static uint8_t type_2ou = UNKNOWN_BOARD;

static void
print_usage_help(void) {
  printf("Usage: enclosure-util <slot1|slot2|slot3|slot4> --drive-status <all|1U-dev[0..3]|2U-dev[0..13]>\n");
  printf("       enclosure-util <slot1|slot2|slot3|slot4> --drive-health\n");
  if ( pal_is_exp() == PAL_EOK ) {
    printf("Usage: enclosure-util <slot1-2U-top|slot1-2U-bot> --drive-status <all|2U-dev[0..13]>\n");
    printf("       enclosure-util <slot1-2U-top|slot1-2U-bot> --drive-health\n");
  }
}

static int
ssd_monitor_enable(uint8_t slot_id, uint8_t intf, bool enable) {
  int ret = 0;
  ret = bic_enable_ssd_sensor_monitor(slot_id, enable, intf);
  if ( enable == false ) {
    msleep(100);
  }
  return ret;
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
  if ( type_2ou != GPV3_MCHP_BOARD && type_2ou != GPV3_BRCM_BOARD && type_2ou != CWC_MCHP_BOARD ) { // Not GPv3
    // since accelerator doesn't implement SMART WARNING, do not check it.
    if ((ssd->warning & NVME_SMART_WARNING_MASK_BIT) != NVME_SMART_WARNING_MASK_BIT)
      return NVME_BAD_HEALTH;
  }
  
  if ((ssd->sflgs & NVME_SFLGS_MASK_BIT) != NVME_SFLGS_MASK_BIT)
    return NVME_BAD_HEALTH;

  return 0;
}

static int
get_mapping_parameter(uint8_t device_id, uint8_t type_1ou, uint8_t *bus, uint8_t *intf, uint8_t *mux) {
  
  const uint8_t INTF[] = { 
    0,              // N/A
    FEXP_BIC_INTF,  // DEV_ID0_1OU
    FEXP_BIC_INTF,  // DEV_ID1_1OU
    FEXP_BIC_INTF,  // DEV_ID2_1OU
    FEXP_BIC_INTF,  // DEV_ID3_1OU
    REXP_BIC_INTF,  // DEV_ID0_2OU
    REXP_BIC_INTF,  // DEV_ID1_2OU
    REXP_BIC_INTF,  // DEV_ID2_2OU
    REXP_BIC_INTF,  // DEV_ID3_2OU
    REXP_BIC_INTF,  // DEV_ID4_2OU
    REXP_BIC_INTF   // DEV_ID5_2OU
  };

  // M.2 1OU device ->   3  2  1  0
  // E1S 1OU device ->   0  1  2  3
  const uint8_t BUS_EDSFF1U[] = { 0, 6, 4, 3, 2, 0, 0, 0, 0, 0, 0};
  const uint8_t BUS_AST_VF[] = { 0, 5, 4, 3, 2, 0, 0, 0, 0, 0, 0};
  const uint8_t BUS_1OU_2OU[] = { 0, 2, 2, 4, 4, 2, 2, 4, 4, 6, 6};
  const uint8_t MUX_1OU_2OU[] = { 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};

  *intf = INTF[device_id];
  
  if (type_1ou == EDSFF_1U) {
    *bus = BUS_EDSFF1U[device_id];
  } else if (type_1ou== VERNAL_FALLS_AST1030) {
    *bus = BUS_AST_VF[device_id];
  } else {
    *bus = BUS_1OU_2OU[device_id];
    *mux = MUX_1OU_2OU[device_id];
  }
  
  return 0;
}

static int
read_nvme_data(uint8_t fru_id, uint8_t slot_id, uint8_t device_id, uint8_t cmd) {
  int ret = 0;
  int offset_base = 0;
  char str[32];
  uint8_t type_1ou = 0;
  uint8_t intf = 0;
  uint8_t mux = 0;
  uint8_t bus = 0;
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[64] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  float tdp_val = 0;
  static const uint8_t spe_ssd_bus[SPE_SSD_NUM] = {2, 3, 4, 5, 6, 7};
  ssd_data ssd;
  
  memset(&ssd, 0x00, sizeof(ssd_data));
  //prevent the invalid access
  if ( type_2ou != GPV3_MCHP_BOARD && type_2ou != GPV3_BRCM_BOARD && type_2ou != E1S_BOARD && type_2ou != CWC_MCHP_BOARD ) {
    ret = bic_get_card_type(slot_id, GET_1OU, &type_1ou);
    if ((ret != 0) || (type_1ou != VERNAL_FALLS_AST1030)) {
      bic_get_1ou_type(slot_id, &type_1ou);
    }
    get_mapping_parameter(device_id, type_1ou, &bus, &intf, &mux);
  }

  fby3_common_dev_name(device_id, str);

  if (type_2ou == E1S_BOARD) {
    bus = spe_ssd_bus[device_id - DEV_ID0_2OU];
    intf = REXP_BIC_INTF;
  } else if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD ) {
    // mux select
    ret = pal_gpv3_mux_select(slot_id, device_id);
    if (ret) {
      return ret; // fail to select mux
    }
    bus = get_gpv3_bus_number(device_id);
    intf = REXP_BIC_INTF;
  } else if ( type_2ou == CWC_MCHP_BOARD ) {
    ret = pal_gpv3_mux_select(fru_id, device_id);
    if (ret) {
      return ret;
    }
    bus = get_gpv3_bus_number(device_id);
    if (fru_id == FRU_2U_TOP) {
      intf = RREXP_BIC_INTF1;
    } else if (fru_id == FRU_2U_BOT) {
      intf = RREXP_BIC_INTF2;
    }
  } else if (type_1ou != EDSFF_1U && type_1ou != VERNAL_FALLS_AST1030) {
    // E1S no need but 1/2OU need to stop sensor monitor then switch mux
    tbuf[0] = (bus << 1) + 1;
    tbuf[1] = 0x02; // mux address
    tbuf[2] = 0;    // read back 8 bytes
    tbuf[3] = 0x00; // offset 00
    tbuf[4] = mux;
    tlen = 5;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
    if (ret != 0) {
      printf("slot%u-%s : %s\n", slot_id, str, "NA");
      syslog(LOG_DEBUG, "%s(): bic_ipmb_send offset=%d read length=%d failed", __func__,tbuf[3],rlen);
      return -1;
    }
  }

  if (cmd == CMD_DRIVE_STATUS) {
    printf("slot%u-%s : ", slot_id, str);
    do {
      tbuf[0] = (bus << 1) + 1;
      tbuf[1] = 0xD4;
      tbuf[2] = 8; //read back 8 bytes
      tbuf[3] = 0x00;  // offset 00
      tlen = 4;
      ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): bic_ipmb_send offset=%d read length=%d failed", __func__,tbuf[3],rlen);
        printf("NA");
        break;
      }

      ssd.sflgs = rbuf[offset_base + 1];
      ssd.warning = rbuf[offset_base + 2];
      ssd.temp = rbuf[offset_base + 3];
      ssd.pdlu = rbuf[offset_base + 4];

      tbuf[2] = 24 + offset_base;;  // read back 24 bytes
      tbuf[3] = 0x08;  // offset 08
      ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): bic_ipmb_send offset=%d read length=%d failed", __func__,tbuf[3],rlen);
        printf("NA");
        break;
      }
      ssd.vendor = (rbuf[offset_base + 1] << 8) | rbuf[offset_base + 2];
      memcpy(ssd.serial_num, &rbuf[offset_base + 3], MAX_SERIAL_NUM);

      if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD || type_2ou == CWC_MCHP_BOARD ) {
        tbuf[2] = 55; //read back
        tbuf[3] = 0x20;  // offset 32
        ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
        if (ret != 0) {
          syslog(LOG_DEBUG, "%s(): bic_ipmb_send offset=%d read length=%d failed", __func__,tbuf[3],rlen);
          break;
        }
        ssd.block_len_module_id_area = rbuf[offset_base + 0];
        ssd.fb_defined = rbuf[offset_base + 1];
        memcpy(ssd.part_num, &rbuf[offset_base + 2], MAX_PART_NUM);
        ssd.meff = rbuf[offset_base + 42];
        ssd.ffi_0 = rbuf[offset_base + 43];

        if (ssd.fb_defined == 1) {
          if (ssd.ffi_0 == FFI_0_ACCELERATOR) {
            tbuf[2] = 8; //read back
            tbuf[3] = 0x60;  // offset 96
            ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): bic_ipmb_send offset=%d read length=%d failed", __func__,tbuf[3],rlen);
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

            tbuf[2] = 8; //read back
            tbuf[3] = 0x68;  // offset 104
            ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): bic_ipmb_send offset=%d read length=%d failed", __func__,tbuf[3],rlen);
              break;
            }
            ssd.block_len_ver_area = rbuf[offset_base + 0];
            ssd.asic_version = rbuf[offset_base + 1];
            ssd.fw_major_ver = rbuf[offset_base + 2];
            ssd.fw_minor_ver = rbuf[offset_base + 3];
            ssd.fw_additional_ver = rbuf[offset_base + 4];

            tbuf[2] = 10; //read back
            tbuf[3] = 0x70;  // offset 112
            ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): bic_ipmb_send offset=%d read length=%d failed", __func__,tbuf[3],rlen);
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
            tbuf[2] = 10; //read back
            if (ssd.vendor == VENDOR_ID_BROARDCOM) {
              tbuf[3] = 0x84;  // offset 132
            } else {
              tbuf[3] = 0x7A;  // offset 122
            }
            ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
            if (ret != 0) {
              syslog(LOG_DEBUG, "%s(): bic_ipmb_send offset=%d read length=%d failed", __func__,tbuf[3],rlen);
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
            tbuf[2] = 9; //read back
            tbuf[3] = 0x57;  // offset 87
            ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
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
      }


      drive_status(&ssd);
    } while (0);
    printf("\n");

  } else if (cmd == CMD_DRIVE_HEALTH) {
    tbuf[0] = (bus << 1) + 1;
    tbuf[1] = 0xD4;
    tbuf[2] = 0x08; // read back 8 bytes
    tbuf[3] = 0x00; // offset 00
    tlen = 4;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
    if (ret != 0) {
      printf("slot%u-%s : %s\n", slot_id, str, "NA");
      return 0;
    }
    
    ssd.sflgs = rbuf[1];
    ssd.warning = rbuf[2];
    ret = drive_health(&ssd);  // ret 1: Abnormal  0:Normal
    printf("slot%u-%s : %s\n", slot_id, str, (ret == 0)?"Normal":"Abnormal");
  } else {
    printf("%s(): unknown cmd \n", __func__);
    return -1;
  }

  return 0;
}

static void
enclosure_sig_handler(int sig) {
  if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD ) { // Config C or Config D GPv3
    if ( ssd_monitor_enable(m_slot_id, REXP_BIC_INTF, true) < 0 ) {
      printf("err: failed to enable SSD monitoring");
    }
  } else if (type_2ou == CWC_MCHP_BOARD) {
    if ( ssd_monitor_enable(m_slot_id, RREXP_BIC_INTF1, true) < 0 ) {
      printf("err: failed to enable 2U-top SSD monitoring");
    }
    if ( ssd_monitor_enable(m_slot_id, RREXP_BIC_INTF2, true) < 0 ) {
      printf("err: failed to enable 2U-bot SSD monitoring");
    }
  }
  if (flock(fd, LOCK_UN)) {
    syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
  }
  close(fd);
  remove(path);
}

int
main(int argc, char **argv) {
  int ret;
  int present = 0;
  uint8_t fru_id = 0;
  uint8_t slot_id = 0;
  uint8_t dev_id = 0;
  uint8_t device_start = 0, device_end = 0;
  uint8_t is_slot_present = 0;
  struct sigaction sa;
  uint8_t intf = 0;
  uint8_t m2_config = CONFIG_UNKNOWN;
  uint8_t bmc_location = 0;

  if (argc != 3 && argc != 4) {
    print_usage_help();
    return -1;
  }

  ret = pal_get_fru_id(argv[1], &fru_id);
  if (ret < 0) {
    printf("Wrong fru: %s\n", argv[1]);
    print_usage_help();
    return -1;
  }
  if ( fru_id == FRU_2U_TOP ) {
    intf = RREXP_BIC_INTF1;
  } else if ( fru_id == FRU_2U_BOT ) {
    intf = RREXP_BIC_INTF2;
  } else  {
    intf = REXP_BIC_INTF;
  }

  pal_get_fru_slot(fru_id, &slot_id);
  m_slot_id = slot_id;

  // need to check slot present
  ret = pal_is_fru_prsnt(fru_id, &is_slot_present); 
  if (ret < 0 || is_slot_present == 0) {
    printf("%s is not present!\n", argv[1]);
    return -1;
  }

  sa.sa_handler = enclosure_sig_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  sprintf(path, SLOT_SENSOR_LOCK, slot_id);
  fd = open(path, O_CREAT | O_RDWR, 0666);
  ret = flock(fd, LOCK_EX | LOCK_NB);
  if (ret) {
    if(EWOULDBLOCK == errno) {
      printf("the lock file is already using, please wait \n");
      return -1;
    }
  }

  // check 1/2OU present status
  ret = bic_is_m2_exp_prsnt(slot_id);
  if ( ret < 0 ) {
    printf("%s() Cannot get the m2 prsnt status\n", __func__);
    goto exit;
  }

  present = (uint8_t)ret;

  if ( (present & PRESENT_2OU) == PRESENT_2OU ) {
    if ( fby3_common_get_2ou_board_type(slot_id, &type_2ou) < 0) {
      printf("Failed to get slot%d 2ou board type\n",type_2ou);
      goto exit;
    }
  }

  if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD || type_2ou == CWC_MCHP_BOARD ) { // Config C or Config D GPv3
    fby3_common_get_bmc_location(&bmc_location);
    if (bmc_location == NIC_BMC) { //NIC BMC
      switch (type_2ou) {
        case GPV3_MCHP_BOARD:
          ret = pal_get_m2_config(FRU_2U, &m2_config);
          break;
        case CWC_MCHP_BOARD:
          if (fru_id == FRU_SLOT1) {
            printf("Only slot1-2U-top and slot1-2U-bot are supported!\n");
            goto exit;
          }
          ret = pal_get_m2_config(fru_id, &m2_config);
          break;
        default:
          break;
      }
      if (ret < 0) {
        printf("fru: %u M.2 config read failed\n", fru_id);
        goto exit;
      }
    } else {
      m2_config = CONFIG_D_GPV3;
    }

    device_start = DEV_ID0_2OU;
    device_end = DEV_ID13_2OU;
    if ( ssd_monitor_enable(slot_id, intf, false) < 0 ) {
      printf("err: failed to disable SSD monitoring\n");
      goto exit;
    }
  } else if ( type_2ou == E1S_BOARD ) {
    device_start = DEV_ID0_2OU;
    device_end = DEV_ID5_2OU;
  } else if ( type_2ou == DP_RISER_BOARD ) {
    printf("DP not support \n");
    goto exit;
  } else if ( present == PRESENT_1OU ) {
    device_start = DEV_ID0_1OU;
    device_end = DEV_ID3_1OU;
  } else if ( present == PRESENT_2OU ) {
    device_start = DEV_ID0_2OU;
    device_end = DEV_ID5_2OU;
  } else if ( present == (PRESENT_1OU + PRESENT_2OU) ) {
    device_start = DEV_ID0_1OU;
    device_end = DEV_ID5_2OU;
  } else {
    printf("Please check the board config \n");
    goto exit;
  }
 
  if ((argc == 4) && !strcmp(argv[2], "--drive-status")) {
    if (!strcmp(argv[3], "all")) {
      if (m2_config == CONFIG_C_CWC_DUAL) {
        // Since it is dual m2 config, get m2 dev data from dev slot 1, 3, 5, 7, 9, 11
        // There are two E1.S devices 12, 13
        device_start = DEV_ID0_2OU;
        device_end = DEV_ID11_2OU;
        for (int i = device_start + 1 ; i <= device_end; i +=2) {
          read_nvme_data(fru_id, slot_id, i, CMD_DRIVE_STATUS);
        }
        read_nvme_data(fru_id, slot_id, DEV_ID12_2OU, CMD_DRIVE_STATUS);
        read_nvme_data(fru_id, slot_id, DEV_ID13_2OU, CMD_DRIVE_STATUS);
      } else {
        for (int i = device_start; i <= device_end; i++) {
          read_nvme_data(fru_id, slot_id, i, CMD_DRIVE_STATUS);
        }
      }
    } else {
      ret = pal_get_dev_id(argv[3], &dev_id);
      if (ret < 0) {
        printf("pal_get_dev_id failed for %s %s\n", argv[1], argv[3]);
        print_usage_help();
        goto exit;
      }
      if ((dev_id < device_start) || (dev_id > device_end)) {
        printf("Please check the board config \n");
        goto exit;
      }
      read_nvme_data(fru_id, slot_id, dev_id, CMD_DRIVE_STATUS);
    }
  } else if ((argc == 3) && !strcmp(argv[2], "--drive-health")) {
    if (m2_config == CONFIG_C_CWC_DUAL) {
      // Since it is dual m2 config, get m2 dev data from dev slot 1, 3, 5, 7, 9, 11
      // There are two E1.S devices 12, 13
      device_start = DEV_ID0_2OU;
      device_end = DEV_ID11_2OU;
      for (int i = device_start + 1 ; i <= device_end; i +=2) {
        read_nvme_data(fru_id, slot_id, i, CMD_DRIVE_HEALTH);
      }
      read_nvme_data(fru_id, slot_id, DEV_ID12_2OU, CMD_DRIVE_HEALTH);
      read_nvme_data(fru_id, slot_id, DEV_ID13_2OU, CMD_DRIVE_HEALTH);
    } else {
      for (int i = device_start; i <= device_end; i++) {
        read_nvme_data(fru_id, slot_id, i, CMD_DRIVE_HEALTH);
      }
    }
  } else {
    print_usage_help();
    goto exit;
  }
  
exit:
  if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD || type_2ou == CWC_MCHP_BOARD ) { // Config C or Config D GPv3
    if ( ssd_monitor_enable(slot_id, intf, true) < 0 ) {
      printf("err: failed to enable SSD monitoring");
    }
  }
  ret = flock(fd, LOCK_UN);
  if (ret != 0) {
    syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
  }
  close(fd);
  remove(path);
  return 0;
}
