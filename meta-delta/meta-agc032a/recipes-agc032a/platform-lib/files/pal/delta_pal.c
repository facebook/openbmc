 /*
  * This file contains functions and logics that depends on Wedge100 specific
  * hardware and kernel drivers. Here, some of the empty "default" functions
  * are overridden with simple functions that returns non-zero error code.
  * This is for preventing any potential escape of failures through the
  * default functions that will return 0 no matter what.
  */

// #define DEBUG
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <openbmc/log.h>
#include <openbmc/libgpio.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/sensor-correction.h>
#include <openbmc/misc-utils.h>
#include <facebook/wedge_eeprom.h>
#include <delta/delta_lib.h>
#include "pal_sensors.h"
#include "pal.h"

#define DELTA_SKIP 0

/* =============== obmc-pal =============== */

/* Board Info */
int
pal_get_board_id(uint8_t slot, uint8_t *req_data, uint8_t req_len, uint8_t *res_data, uint8_t *res_len)
{
	int board_id = 0, board_rev ;
	unsigned char *data = res_data;
	int completion_code = CC_UNSPECIFIED_ERROR;


  int i = 0, num_chips, num_pins;
  char device[64], path[32];
	gpiochip_desc_t *chips[GPIO_CHIP_MAX];
  gpiochip_desc_t *gcdesc;
  gpio_desc_t *gdesc;
  gpio_value_t value;

	num_chips = gpiochip_list(chips, ARRAY_SIZE(chips));
  if(num_chips < 0){
    *res_len = 0;
		return completion_code;
  }

  gcdesc = gpiochip_lookup(SCM_BRD_ID);
  if (gcdesc == NULL) {
    *res_len = 0;
		return completion_code;
  }

  num_pins = gpiochip_get_ngpio(gcdesc);
  gpiochip_get_device(gcdesc, device, sizeof(device));

  for(i = 0; i < num_pins; i++){
    sprintf(path, "%s%d", "BRD_ID_",i);
    gpio_export_by_offset(device,i,path);
    gdesc = gpio_open_by_shadow(path);
    if (gpio_get_value(gdesc, &value) == 0) {
      board_id  |= (((int)value)<<i);
    }
    gpio_unexport(path);
  }

	if(pal_get_board_rev(&board_rev) == -1){
    *res_len = 0;
		return completion_code;
  }

	*data++ = board_id;
	*data++ = board_rev;
	*data++ = slot;
	*data++ = 0x00; // 1S Server.
	*res_len = data - res_data;
	completion_code = CC_SUCCESS;

	return completion_code;
}

int
pal_get_board_type_rev(uint8_t *brd_type_rev){
  char path[LARGEST_DEVICE_NAME + 1];
  int brd_rev;
  uint8_t brd_type;
  int val;
  if( pal_get_board_rev(&brd_rev) != 0 ||
      pal_get_board_type(&brd_type) != 0 ){
        return CC_UNSPECIFIED_ERROR;
  } else if ( brd_type == BRD_TYPE_WEDGE400 ){
    switch ( brd_rev ) {
      case 0x00:
        /* For WEDGE400 EVT & EVT3 need to detect from SMBCPLD */
        snprintf(path, LARGEST_DEVICE_NAME, SMB_SYSFS, "board_ver");
        if (read_device(path, &val)) {
          return CC_UNSPECIFIED_ERROR;
        }
        if ( val == 0x00 ) {
          *brd_type_rev = BOARD_WEDGE400_EVT;
        } else {
          *brd_type_rev = BOARD_WEDGE400_EVT3;
        }
        break;
      case 0x02: *brd_type_rev = BOARD_WEDGE400_DVT; break;
      case 0x03: *brd_type_rev = BOARD_WEDGE400_DVT2_PVT_PVT2; break;
      case 0x04: *brd_type_rev = BOARD_WEDGE400_PVT3; break;
      default:
        *brd_type_rev = BOARD_UNDEFINED;
        return CC_UNSPECIFIED_ERROR;
    }
  } else if ( brd_type == BRD_TYPE_WEDGE400C ){
    switch ( brd_rev ) {
      case 0x00: *brd_type_rev = BOARD_WEDGE400C_EVT; break;
      case 0x01: *brd_type_rev = BOARD_WEDGE400C_EVT2; break;
      case 0x02: *brd_type_rev = BOARD_WEDGE400C_DVT; break;
      default:
        *brd_type_rev = BOARD_UNDEFINED;
        return CC_UNSPECIFIED_ERROR;
    }
  } else {
    *brd_type_rev = BOARD_UNDEFINED;
    return CC_UNSPECIFIED_ERROR;
  }
  return CC_SUCCESS;
}

int
pal_get_boot_order(uint8_t slot, uint8_t *req_data, uint8_t *boot, uint8_t *res_len) {
  int i;
  int j = 0;
  int ret;
  int msb, lsb;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char tstr[4] = {0};

  sprintf(key, "slot%d_boot_order", slot);
  ret = pal_get_key_value(key, str);
  if (ret) {
    *res_len = 0;
    return ret;
  }

  for (i = 0; i < 2*SIZE_BOOT_ORDER; i += 2) {
    sprintf(tstr, "%c\n", str[i]);
    msb = strtol(tstr, NULL, 16);

    sprintf(tstr, "%c\n", str[i+1]);
    lsb = strtol(tstr, NULL, 16);
    boot[j++] = (msb << 4) | lsb;
  }

  *res_len = SIZE_BOOT_ORDER;
  return 0;
}

void
pal_get_chassis_status(uint8_t slot, uint8_t *req_data, uint8_t *res_data, uint8_t *res_len) {

  char key[MAX_KEY_LEN];
  char buff[MAX_VALUE_LEN];
  int policy = 3;
  uint8_t status, ret;
  unsigned char *data = res_data;

  /* Platform Power Policy */
  sprintf(key, "%s", "server_por_cfg");
  if (pal_get_key_value(key, buff) == 0)
  {
    if (!memcmp(buff, "off", strlen("off")))
      policy = 0;
    else if (!memcmp(buff, "lps", strlen("lps")))
      policy = 1;
    else if (!memcmp(buff, "on", strlen("on")))
      policy = 2;
    else
      policy = 3;
  }

  /* Current Power State */
#if DELTA_SKIP
  ret = pal_get_server_power(FRU_SCM, &status);
#else
  ret = pal_get_server_power(0, &status);
#endif
  if (ret >= 0) {
    *data++ = status | (policy << 5);
  } else {
    /* load default */
    OBMC_WARN("ipmid: pal_get_server_power failed for server\n");
    *data++ = 0x00 | (policy << 5);
  }
  *data++ = 0x00;   /* Last Power Event */
  *data++ = 0x40;   /* Misc. Chassis Status */
  *data++ = 0x00;   /* Front Panel Button Disable */
  *res_len = data - res_data;
}

/* GUID
pal_get_dev_guid
pal_get_guid
pal_get_sys_guid
pal_populate_guid
pal_set_dev_guid
pal_set_guid
pal_set_sys_guid
*/

int
pal_get_dev_guid(uint8_t fru, char *guid) {

  pal_get_guid(OFFSET_DEV_GUID, (char *)g_dev_guid);
  memcpy(guid, g_dev_guid, GUID_SIZE);

  return 0;
}

int
pal_set_dev_guid(uint8_t slot, char *guid) {

  pal_populate_guid(g_dev_guid, guid);

  return pal_set_guid(OFFSET_DEV_GUID, (char *)g_dev_guid);
}

int
pal_get_sys_guid(uint8_t slot, char *guid) {

  return bic_get_sys_guid(IPMB_BUS, (uint8_t *)guid);
}

int
pal_set_sys_guid(uint8_t slot, char *str) {
  uint8_t guid[GUID_SIZE] = {0x00};

  pal_populate_guid(guid, str);

  return bic_set_sys_guid(IPMB_BUS, guid);
}

/* FRU */
int
pal_get_fru_health(uint8_t fru, uint8_t *value) {

  char cvalue[MAX_VALUE_LEN] = {0};
  char key[MAX_KEY_LEN] = {0};
  int ret;

  switch(fru) {
#if DELTA_SKIP
    case FRU_SCM:
      sprintf(key, "scm_sensor_health");
      break;
#endif
    case FRU_SMB:
      sprintf(key, "smb_sensor_health");
      break;
    case FRU_FCM:
      sprintf(key, "fcm_sensor_health");
      break;

    case FRU_PEM1:
        sprintf(key, "pem1_sensor_health");
        break;
    case FRU_PEM2:
        sprintf(key, "pem2_sensor_health");
        break;

    case FRU_PSU1:
        sprintf(key, "psu1_sensor_health");
        break;
    case FRU_PSU2:
        sprintf(key, "psu2_sensor_health");
        break;

    case FRU_FAN1:
        sprintf(key, "fan1_sensor_health");
        break;
    case FRU_FAN2:
        sprintf(key, "fan2_sensor_health");
        break;
    case FRU_FAN3:
        sprintf(key, "fan3_sensor_health");
        break;
    case FRU_FAN4:
        sprintf(key, "fan4_sensor_health");
        break;
    case FRU_FAN5:
        sprintf(key, "fan5_sensor_health");
        break;
    case FRU_FAN6:
        sprintf(key, "fan6_sensor_health");
        break;

    default:
      return -1;
  }

  ret = pal_get_key_value(key, cvalue);
  if (ret) {
    return ret;
  }

  *value = atoi(cvalue);
  *value = *value & atoi(cvalue);
  return 0;
}

int
pal_get_fru_id(char *str, uint8_t *fru) {
  if (!strcmp(str, "all")) {
    *fru = FRU_ALL;
  } else if (!strcmp(str, "smb")) {
    *fru = FRU_SMB;
#if DELTA_SKIP
  } else if (!strcmp(str, "scm")) {
    *fru = FRU_SCM;
#endif
  } else if (!strcmp(str, "pem1")) {
    *fru = FRU_PEM1;
  } else if (!strcmp(str, "pem2")) {
    *fru = FRU_PEM2;
  } else if (!strcmp(str, "psu1")) {
    *fru = FRU_PSU1;
  } else if (!strcmp(str, "psu2")) {
    *fru = FRU_PSU2;
  } else if (!strcmp(str, "fan1")) {
    *fru = FRU_FAN1;
  } else if (!strcmp(str, "fan2")) {
    *fru = FRU_FAN2;
  } else if (!strcmp(str, "fan3")) {
    *fru = FRU_FAN3;
  } else if (!strcmp(str, "fan4")) {
    *fru = FRU_FAN4;
  } else if (!strcmp(str, "fan5")) {
    *fru = FRU_FAN5;
  } else if (!strcmp(str, "fan6")) {
    *fru = FRU_FAN6;
  } else if (!strcmp(str, "bmc")) {
    *fru = FRU_BMC;
  } else if (!strcmp(str, "cpld")) {
    *fru = FRU_CPLD;
  } else if (!strcmp(str, "fpga")) {
    *fru = FRU_FPGA;
  } else {
    OBMC_WARN("pal_get_fru_id: Wrong fru#%s", str);
    return -1;
  }

  return 0;
}

int
pal_get_fru_list(char *list) {
  strcpy(list, pal_fru_list);
  return 0;
}

int
pal_get_fru_name(uint8_t fru, char *name) {
  switch(fru) {
    case FRU_SMB:
      strcpy(name, "smb");
      break;
#if DETLA_SKIP
    case FRU_SCM:
      strcpy(name, "scm");
      break;
#endif
    case FRU_FCM:
      strcpy(name, "fcm");
      break;
    case FRU_PEM1:
      strcpy(name, "pem1");
      break;
    case FRU_PEM2:
      strcpy(name, "pem2");
      break;
    case FRU_PSU1:
      strcpy(name, "psu1");
      break;
    case FRU_PSU2:
      strcpy(name, "psu2");
      break;
    case FRU_FAN1:
      strcpy(name, "fan1");
      break;
    case FRU_FAN2:
      strcpy(name, "fan2");
      break;
    case FRU_FAN3:
      strcpy(name, "fan3");
      break;
    case FRU_FAN4:
      strcpy(name, "fan4");
      break;
    case FRU_FAN5:
      strcpy(name, "fan5");
      break;
    case FRU_FAN6:
      strcpy(name, "fan6");
      break;
    default:
      if (fru > MAX_NUM_FRUS)
        return -1;
      sprintf(name, "fru%d", fru);
      break;
  }
  return 0;
}

int
pal_is_fru_prsnt(uint8_t fru, uint8_t *status) {
#if DELTA_SKIP
  int val,ext_prsnt;
#else
  int val = 0;
#endif

  char tmp[LARGEST_DEVICE_NAME];
  char path[LARGEST_DEVICE_NAME + 1];
  *status = 0;

  switch (fru) {
    case FRU_SMB:
      *status = 1;
      return 0;
#if DELTA_SKIP
    case FRU_SCM:
      snprintf(path, LARGEST_DEVICE_NAME, SMB_SYSFS, SCM_PRSNT_STATUS);
      break;
#endif
    case FRU_FCM:
      *status = 1;
      return 0;
    case FRU_PEM1:
      snprintf(tmp, LARGEST_DEVICE_NAME, SWPLD1_SYSFS, PEM_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 1);
      break;
    case FRU_PEM2:
      snprintf(tmp, LARGEST_DEVICE_NAME, SWPLD1_SYSFS, PEM_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 2);
      break;
    case FRU_PSU1:
      snprintf(tmp, LARGEST_DEVICE_NAME, SWPLD1_SYSFS, PSU_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 1);
      break;
    case FRU_PSU2:
      snprintf(tmp, LARGEST_DEVICE_NAME, SWPLD1_SYSFS, PSU_PRSNT_STATUS);
      snprintf(path, LARGEST_DEVICE_NAME, tmp, 2);
      break;
    case FRU_FAN1:
    case FRU_FAN2:
    case FRU_FAN3:
    case FRU_FAN4:
    case FRU_FAN5:
    case FRU_FAN6:
      snprintf(path, LARGEST_DEVICE_NAME, GPIO_VAL, fru - FRU_FAN1 + FAN_PRSNT_GPIO_START_POS);
      break;
    default:
      printf("unsupported fru id %d\n", fru);
      return -1;
    }

    if (read_device(path, &val)) {
      return -1;
    }

    if (val == 0x0) {
      *status = 1;
    } else {
      *status = 0;
      return 0;
    }

#if DELTA_SKIP
    if ( fru == FRU_PEM1 || fru == FRU_PSU1 ){
      ext_prsnt = pal_detect_i2c_device(22,0x18); // 0 present -1 absent
      if( fru == FRU_PEM1 && ext_prsnt == 0 ){ // for PEM 0x18 should present
        *status = 1;
      } else if ( fru == FRU_PSU1 && ext_prsnt < 0 ){ // for PSU 0x18 should absent
        *status = 1;
      } else {
        *status = 0;
      }
    }
    else if ( fru == FRU_PEM2 || fru == FRU_PSU2 ){
      ext_prsnt = pal_detect_i2c_device(23,0x18); // 0 present -1 absent
      if( fru == FRU_PEM2 && ext_prsnt == 0 ){ // for PEM 0x18 should present
        *status = 1;
      } else if ( fru == FRU_PSU2 && ext_prsnt < 0 ){ // for PSU 0x18 should absent
        *status = 1;
      } else {
        *status = 0;
      }
    }
#endif
    return 0;
}

int
pal_is_fru_ready(uint8_t fru, uint8_t *status) {
  int ret = 0;

  switch(fru) {
    case FRU_PEM1:
      if(!check_dir_exist(PEM1_LTC4282_DIR) && !check_dir_exist(PEM1_MAX6615_DIR)) {
        *status = 1;
      }
      break;
    case FRU_PEM2:
      if(!check_dir_exist(PEM2_LTC4282_DIR) && !check_dir_exist(PEM2_MAX6615_DIR)) {
        *status = 1;
      }
      break;
    case FRU_PSU1:
      if(!check_dir_exist(PSU1_DEVICE)) {
        *status = 1;
      }
      break;
    case FRU_PSU2:
      if(!check_dir_exist(PSU2_DEVICE)) {
        *status = 1;
      }
      break;
    default:
      *status = 1;

      break;
  }

  return ret;
}


/* Sensor */
/*
pal_get_event_sensor_name
pal_set_sensor_health
pal_get_fan_speed
pal_get_fw_info
pal_get_key_value
pal_get_last_pwr_state
pal_get_num_slots
pal_get_plat_sku_id
pal_get_platform_name
pal_get_poss_pcie_config
pal_get_server_power
pal_get_sysfw_ver
pal_inform_bic_mode
pal_ipmb_processing
pal_parse_sel
pal_post_handle
pal_sel_handler
pal_set_boot_order
pal_set_def_key_value
pal_set_key_value
pal_set_last_pwr_state
pal_set_power_restore_policy
pal_set_rst_btn
pal_set_server_power
pal_set_sysfw_ver
pal_update_ts_sled
*/


/* =============== board specified =============== */

/* Board Info */
int
pal_get_board_rev(int *rev) {
  char path[LARGEST_DEVICE_NAME + 1];
  int val_id_0, val_id_1, val_id_2;

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_SMB_REV_ID_0, "value");
  if (read_device(path, &val_id_0)) {
    return -1;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_SMB_REV_ID_1, "value");
  if (read_device(path, &val_id_1)) {
    return -1;
  }

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_SMB_REV_ID_2, "value");
  if (read_device(path, &val_id_2)) {
    return -1;
  }

  *rev = val_id_0 | (val_id_1 << 1) | (val_id_2 << 2);

  return 0;
}

int
pal_get_board_type(uint8_t *brd_type){
  char path[LARGEST_DEVICE_NAME + 1];
  int val;

  snprintf(path, LARGEST_DEVICE_NAME, GPIO_BMC_BRD_TPYE, "value");
  if (read_device(path, &val)) {
    return CC_UNSPECIFIED_ERROR;
  }

  if(val == 0x1){
    *brd_type = BRD_TYPE_WEDGE400;
    return CC_SUCCESS;
  }else if(val == 0x0){
    *brd_type = BRD_TYPE_WEDGE400C;
    return CC_SUCCESS;
  }else{
    return CC_UNSPECIFIED_ERROR;
  }
}

int
pal_get_cpld_board_rev(int *rev, const char *device) {
  char full_name[LARGEST_DEVICE_NAME + 1];

  snprintf(full_name, LARGEST_DEVICE_NAME, device, "board_ver");
  if (read_device(full_name, rev)) {
    return -1;
  }

  return 0;
}

int
pal_get_cpld_fpga_fw_ver(uint8_t fru, const char *device, uint8_t* ver) {
  int val = -1;
  char ver_path[PATH_MAX];
  char sub_ver_path[PATH_MAX];

  switch(fru) {
    case FRU_CPLD:
      if (!(strncmp(device, SWPLD1, strlen(SWPLD1)))) {
        snprintf(ver_path, sizeof(ver_path), SWPLD1_SYSFS, "swpld1_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path), SWPLD1_SYSFS, "swpld1_ver_type");
      } else if (!(strncmp(device, SWPLD2, strlen(SWPLD2)))) {
        snprintf(ver_path, sizeof(ver_path), SWPLD2_SYSFS, "swpld2_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path), SWPLD2_SYSFS, "swpld2_ver_type");
      } else if (!(strncmp(device, SWPLD3, strlen(SWPLD3)))) {
        snprintf(ver_path, sizeof(ver_path), SWPLD3_SYSFS, "swpld3_ver");
        snprintf(sub_ver_path, sizeof(sub_ver_path), SWPLD3_SYSFS, "swpld3_ver_type");
      } else {
        return -1;
      }
      break;
    case FRU_FPGA:
      return -1;
      break;
    default:
      return -1;
  }

  if (!read_device(ver_path, &val)) {
    ver[0] = (uint8_t)val;
  } else {
    return -1;
  }

  if (!read_device(sub_ver_path, &val)) {
    ver[1] = (uint8_t)val;
  } else {
    printf("[debug][ver:%s]\n", ver_path);
    printf("[debug][sub_ver:%s]\n", sub_ver_path);
    OBMC_INFO("[debug][ver:%s]\n", ver_path);
    OBMC_INFO("[debug][ver_type:%s]\n", sub_ver_path);
    return -1;
  }

  return 0;
}

/* GUID
pal_get_dev_guid
pal_get_guid
pal_get_sys_guid
pal_populate_guid
pal_set_dev_guid
pal_set_guid
pal_set_sys_guid
*/
// Write GUID into EEPROM
static int
pal_set_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_wr;
  char eeprom_path[FBW_EEPROM_PATH_SIZE];
  errno = 0;

  wedge_eeprom_path(eeprom_path);

  // Check for file presence
  if (access(eeprom_path, F_OK) == -1) {
      OBMC_ERROR(-1, "pal_set_guid: unable to access the %s file: %s",
          eeprom_path, strerror(errno));
      return errno;
  }

  // Open file
  fd = open(eeprom_path, O_WRONLY);
  if (fd == -1) {
    OBMC_ERROR(-1, "pal_set_guid: unable to open the %s file: %s",
        eeprom_path, strerror(errno));
    return errno;
  }

  // Seek the offset
  lseek(fd, offset, SEEK_SET);

  // Write GUID data
  bytes_wr = write(fd, guid, GUID_SIZE);
  if (bytes_wr != GUID_SIZE) {
    OBMC_ERROR(-1, "pal_set_guid: write to %s file failed: %s",
        eeprom_path, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

// Read GUID from EEPROM
static int
pal_get_guid(uint16_t offset, char *guid) {
  int fd = 0;
  ssize_t bytes_rd;
  char eeprom_path[FBW_EEPROM_PATH_SIZE];
  errno = 0;

  wedge_eeprom_path(eeprom_path);

  // Check if file is present
  if (access(eeprom_path, F_OK) == -1) {
      OBMC_ERROR(-1, "pal_get_guid: unable to access the %s file: %s",
          eeprom_path, strerror(errno));
      return errno;
  }

  // Open the file
  fd = open(eeprom_path, O_RDONLY);
  if (fd == -1) {
    OBMC_ERROR(-1, "pal_get_guid: unable to open the %s file: %s",
        eeprom_path, strerror(errno));
    return errno;
  }

  // seek to the offset
  lseek(fd, offset, SEEK_SET);

  // Read bytes from location
  bytes_rd = read(fd, guid, GUID_SIZE);
  if (bytes_rd != GUID_SIZE) {
    OBMC_ERROR(-1, "pal_get_guid: read to %s file failed: %s",
        eeprom_path, strerror(errno));
    goto err_exit;
  }

err_exit:
  close(fd);
  return errno;
}

// GUID based on RFC4122 format @ https://tools.ietf.org/html/rfc4122
static void
pal_populate_guid(uint8_t *guid, char *str) {
  unsigned int secs;
  unsigned int usecs;
  struct timeval tv;
  uint8_t count;
  uint8_t lsb, msb;
  int i, r;

  // Populate time
  gettimeofday(&tv, NULL);

  secs = tv.tv_sec;
  usecs = tv.tv_usec;
  guid[0] = usecs & 0xFF;
  guid[1] = (usecs >> 8) & 0xFF;
  guid[2] = (usecs >> 16) & 0xFF;
  guid[3] = (usecs >> 24) & 0xFF;
  guid[4] = secs & 0xFF;
  guid[5] = (secs >> 8) & 0xFF;
  guid[6] = (secs >> 16) & 0xFF;
  guid[7] = (secs >> 24) & 0x0F;

  // Populate version
  guid[7] |= 0x10;

  // Populate clock seq with randmom number
  //getrandom(&guid[8], 2, 0);
  srand(time(NULL));
  //memcpy(&guid[8], rand(), 2);
  r = rand();
  guid[8] = r & 0xFF;
  guid[9] = (r>>8) & 0xFF;

  // Use string to populate 6 bytes unique
  // e.g. LSP62100035 => 'S' 'P' 0x62 0x10 0x00 0x35
  count = 0;
  for (i = strlen(str)-1; i >= 0; i--) {
    if (count == 6) {
      break;
    }

    // If alphabet use the character as is
    if (isalpha(str[i])) {
      guid[15-count] = str[i];
      count++;
      continue;
    }

    // If it is 0-9, use two numbers as BCD
    lsb = str[i] - '0';
    if (i > 0) {
      i--;
      if (isalpha(str[i])) {
        i++;
        msb = 0;
      } else {
        msb = str[i] - '0';
      }
    } else {
      msb = 0;
    }
    guid[15-count] = (msb << 4) | lsb;
    count++;
  }

  // zero the remaining bytes, if any
  if (count != 6) {
    memset(&guid[10], 0, 6-count);
  }

}

/* Sensor */
/*
pal_get_sensor_name
pal_get_sensor_poll_interval
pal_get_sensor_threshold
pal_get_sensor_units
pal_get_sensor_util_timeout
pal_init_sensor_check
pal_sensor_assert_handle
pal_sensor_deassert_handle
pal_sensor_discrete_check
pal_sensor_discrete_read_raw
pal_sensor_read_raw
pal_sensor_threshold_flag
*/

/* FRU */
/*
pal_get_fru_discrete_list
pal_get_fru_sensor_list
*/

/* HW */
/*
pal_get_hand_sw
pal_get_hand_sw_physically
*/

/* POST */
/*
pal_post_display
pal_post_enable
pal_post_get_last
pal_post_get_last_and_len
*/

/* NONE */
/*
pal_is_debug_card_prsnt
pal_is_mcu_working
pal_key_check
pal_light_scm_led
pal_set_com_pwr_btn_n
pal_set_post_gpio_out
pal_set_th3_power
pal_store_crashdump
pal_switch_uart_mux
*/