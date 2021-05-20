/*
 * bic-util
 *
 * Copyright 2021-present Facebook. All Rights Reserved.
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <ctype.h>
#include <facebook/bic.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <facebook/fbgc_common.h>
#include <facebook/fbgc_gpio.h>
#include <sys/time.h>
#include <time.h>

#define MAX_FILE_CMD_BUF     1024
#define MAX_USB_TRANSFER_LEN 64
#define BIT_VALUE(list, index) \
           ((((uint8_t*)&list)[index/8]) >> (index % 8)) & 0x1\

static const char *option_list[] = {
  "--get_gpio",
  "--set_gpio [gpio_num] [value]",
  "--get_gpio_config",
  "--set_gpio_config $gpio_num $value",
  "--check_status",
  "--get_dev_id",
  "--reset",
  "--get_post_code",
  "--get_sdr",
  "--read_sensor",
  "--perf_test [loop_count] (0 to run forever)",
  "--clear_cmos",
  "--file [path]",
  "--check_usb_port",
};

static void
print_usage_help(void) {
  int i = 0;

  printf("Usage: bic-util server <[0..n]data_bytes_to_send>\n");
  printf("Usage: bic-util server <option>\n");
  printf("       option:\n");
  for (i = 0; i < sizeof(option_list) / sizeof(option_list[0]); i++) {
    printf("       %s\n", option_list[i]);
  }
}

// Check BIC status
static int
util_check_status() {
  int ret = 0;
  uint8_t status = 0;

  // BIC status is only valid if 12V-on. check this first
  ret = pal_get_server_12v_power(FRU_SERVER, &status);
  if ( (ret < 0) || (SERVER_12V_OFF == status) ) {
    ret = pal_is_fru_prsnt(FRU_SERVER, &status);

    if (ret < 0) {
       printf("unable to check BIC status\n");
       return ret;
    }

    if (status == FRU_ABSENT) {
      printf("Slot is empty, unable to check BIC status\n");
    } else {
      printf("Slot is 12V-off, unable to check BIC status\n");
    }
    ret = 0;
  } else {
    if (is_bic_ready() == STATUS_BIC_READY) {
      printf("BIC status ok\n");
      ret = 0;
    } else {
      printf("Error: BIC not ready\n");
      ret = -1;
    }
  }
  return ret;
}

// Test to Get device ID
static int
util_get_device_id() {
  int ret = 0;
  ipmi_dev_id_t id = {0};

  ret = bic_get_dev_id(&id);
  if (ret != 0) {
    printf("util_get_device_id: bic_get_dev_id returns %d\n", ret);
    return ret;
  }

  // Print response
  printf("Device ID: 0x%X\n", id.dev_id);
  printf("Device Revision: 0x%X\n", id.dev_rev);
  printf("Firmware Revision: 0x%X:0x%X\n", id.fw_rev1, id.fw_rev2);
  printf("IPMI Version: 0x%X\n", id.ipmi_ver);
  printf("Device Support: 0x%X\n", id.dev_support);
  printf("Manufacturer ID: 0x%X:0x%X:0x%X\n", id.mfg_id[2], id.mfg_id[1], id.mfg_id[0]);
  printf("Product ID: 0x%X:0x%X\n", id.prod_id[1], id.prod_id[0]);
  printf("Aux. FW Rev: 0x%X:0x%X:0x%X:0x%X\n", id.aux_fw_rev[0], id.aux_fw_rev[1],id.aux_fw_rev[2],id.aux_fw_rev[3]);

  return ret;
}

// reset BIC
static int
util_bic_reset() {
  int ret = 0;
  ret = bic_reset();
  printf("Performing BIC reset, status %d\n", ret);
  return ret;
}

static bool
util_is_numeric(char **argv) {
  int j = 0;
  int len = 0;

  if (argv == NULL) {
    return false;
  }
  for (int i = 0; i < 2; i++) { //check netFn cmd
    len = strlen(argv[i]);
    if (len > 2 && argv[i][0] == '0' && (argv[i][1] == 'x' || argv[i][1] == 'X')) {
      j = 2;
      for (; j < len; j++) {
        if (!isxdigit(argv[i][j]))
          return false;
      }
    } else {
      j = 0;
      for (; j < len; j++) {
        if (!isdigit(argv[i][j]))
          return false;
      }
    }
  }

  return true;
}

static int
process_command(int argc, char **argv) {
  int i = 0, ret = 0, retry = 2;
  uint8_t tbuf[MAX_IPMB_REQ_LEN] = {0x00};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  if (argv == NULL) {
    printf("Command is missing\n");
    return -1;
  }
  for (i = 0; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  while (retry >= 0) {
    ret = bic_ipmb_wrapper(tbuf[0]>>2, tbuf[1], &tbuf[2], tlen-2, rbuf, &rlen);
    if (ret == 0) {
      break;
    }
    retry--;
  }
  if (ret != 0) {
    printf("BIC no response!\n");
    return ret;
  }

  for (i = 0; i < rlen; i++) {
    if (!(i % 16) && i) {
      printf("\n");
    }

    printf("%02X ", rbuf[i]);
  }
  printf("\n");

  return 0;
}

static int
process_file(char *path) {
#define MAX_ARG_NUM 64
  FILE *fp = NULL;
  int cmd_length = 0;
  char buf[MAX_FILE_CMD_BUF] = {0};
  char *str = NULL, *next = NULL, *del=" \n";
  char *cmd_buf[MAX_ARG_NUM] = {NULL};

  if (path == NULL) {
    printf("%s Invalid parameter: path is null\n", __func__);
    return -1;
  }
  if (!(fp = fopen(path, "r"))) {
    syslog(LOG_WARNING, "Failed to open %s", path);
    return -1;
  }

  while (fgets(buf, sizeof(buf), fp) != NULL) {
    str = strtok_r(buf, del, &next);
    for (cmd_length = 0; cmd_length < MAX_ARG_NUM && str; cmd_length++, str = strtok_r(NULL, del, &next)) {
      if (str[0] == '#')
        break;

      if ((!cmd_length) && (!strcmp(str, "echo"))) {
        if ((*next) != 0) {
          printf("%s", next);
        } else {
          printf("\n");
        }
        break;
      }
      cmd_buf[cmd_length] = str;
    }
    if (cmd_length <= 1) {
      continue;
    }

    process_command(cmd_length, cmd_buf);
  }
  fclose(fp);

  return 0;
}

static int
util_get_gpio() {
  int ret = 0;
  uint8_t i = 0;
  uint8_t gpio_pin_cnt = fbgc_get_bic_gpio_list_size();
  bic_gpio_t gpio = {0};

  ret = bic_get_gpio(&gpio);
  if (ret < 0) {
    printf("%s() bic_get_gpio returns %d\n", __func__, ret);
    return ret;
  }
  // Print the gpio index, name and value
  for (i = 0; i < gpio_pin_cnt; i++) {    
    printf("%d %s: %d\n",i , fbgc_get_bic_gpio_name(i), BIT_VALUE(gpio, i));
  }

  return ret;
}

static int
util_set_gpio(uint8_t gpio_num, uint8_t gpio_val) {
  uint8_t gpio_pin_cnt = fbgc_get_bic_gpio_list_size();
  int ret = -1;

  if (gpio_num > gpio_pin_cnt) {
    printf("Invalid GPIO pin number %d\n", gpio_num);
    return ret;
  }
  printf("setting [%d] %s to %d\n", gpio_num, fbgc_get_bic_gpio_name(gpio_num), gpio_val);

  ret = bic_set_gpio(gpio_num, gpio_val);
  if (ret < 0) {
    printf("%s() bic_set_gpio returns %d\n", __func__, ret);
  }

  return ret;
}

static int
util_get_gpio_config() {
  int ret = 0;
  uint8_t i = 0;
  uint8_t gpio_pin_cnt = fbgc_get_bic_gpio_list_size();
  bic_gpio_config_t gpio_config = {0}; 

  // Print the gpio index, name and value
  for (i = 0; i < gpio_pin_cnt; i++) {
    ret = bic_get_gpio_config(i, (uint8_t *)&gpio_config);
    if (ret < 0) {
      printf("Failed to get %s config\n\n", fbgc_get_bic_gpio_name(i));
      continue;
    }

    printf("gpio_config for pin#%d (%s):\n", i, fbgc_get_bic_gpio_name(i));
    printf("Direction: %s", gpio_config.dir > 0 ? "Output," : "Input, ");
    printf(" Interrupt: %s", gpio_config.ie > 0 ? "Enabled, " : "Disabled,");
    printf(" Trigger: %s", gpio_config.edge ? "Level " : "Edge ");
    if (gpio_config.trig == 0x0) {
      printf("Trigger,  Edge: %s\n", "Falling Edge");
    } else if (gpio_config.trig == 0x1) {
      printf("Trigger,  Edge: %s\n", "Rising Edge");
    } else if (gpio_config.trig == 0x2) {
      printf("Trigger,  Edge: %s\n", "Both Edges");
    } else  {
      printf("Trigger, Edge: %s\n", "Reserved");
    }
  }

  return 0;
}

static int
util_set_gpio_config(uint8_t gpio_num, uint8_t config_val) {
  int ret = 0;
  uint8_t gpio_pin_cnt = fbgc_get_bic_gpio_list_size();

  if ( gpio_num > gpio_pin_cnt ) {
    printf("Invalid GPIO pin number %d\n", gpio_num);
    return ret;
  }

  printf("Setting GPIO [%d]%s config to 0x%02x\n", gpio_num, fbgc_get_bic_gpio_name(gpio_num), config_val);
  ret = bic_set_gpio_config(gpio_num, config_val);
  if (ret < 0) {
    printf("%s() bic_set_gpio_config returns %d\n", __func__, ret);
  }

  return 0;
}

static int
util_perf_test(int loopCount) {
  enum cmd_profile_type {
    CMD_AVG_DURATION = 0x0,
    CMD_MIN_DURATION,
    CMD_MAX_DURATION,
    CMD_NUM_SAMPLES,
    CMD_PROFILE_NUM
  };
  long double cmd_profile[CMD_PROFILE_NUM] = {0};
  int ret = 0;
  ipmi_dev_id_t id = {0};
  int i = 0;
  long double elapsedTime = 0;
  struct timeval tv1, tv2;

  cmd_profile[CMD_MIN_DURATION] = 3000000;
  cmd_profile[CMD_MAX_DURATION] = 0;
  cmd_profile[CMD_NUM_SAMPLES]  = 0;
  cmd_profile[CMD_AVG_DURATION] = 0;

  printf("bic-util: perf test for ");
  if (loopCount != 0){
    printf("%d cycles\n", loopCount);
  } else {
    printf("infinite cycles\n");
  }
  
  while(1) {
    gettimeofday(&tv1, NULL);

    ret = bic_get_dev_id(&id);
    if (ret) {
      printf("util_perf_test: bic_get_dev_id returns %d, loop=%d\n", ret, i);
      return ret;
    }

    gettimeofday(&tv2, NULL);

    elapsedTime = (((long double)(tv2.tv_usec - tv1.tv_usec) / 1000000 +
                   (long double)(tv2.tv_sec - tv1.tv_sec)) * 1000000);

    cmd_profile[CMD_AVG_DURATION] += elapsedTime;
    cmd_profile[CMD_NUM_SAMPLES] += 1;
    if (cmd_profile[CMD_MAX_DURATION] < elapsedTime) {
      cmd_profile[CMD_MAX_DURATION] = elapsedTime;
    }

    if (cmd_profile[CMD_MIN_DURATION] > elapsedTime && elapsedTime > 0) {
      cmd_profile[CMD_MIN_DURATION] = elapsedTime;
    }
    ++i;

    if ((i & 0xfff) == 0) {
      printf("stats: loop %d num cmds=%d, avg duration(us)=%d, min duration(us)=%d, max duration(us)=%d\n",
            i,
            (int)cmd_profile[CMD_NUM_SAMPLES],
            (int)(cmd_profile[CMD_AVG_DURATION]/cmd_profile[CMD_NUM_SAMPLES]),
            (int)cmd_profile[CMD_MIN_DURATION],
            (int)cmd_profile[CMD_MAX_DURATION]
          );
    }

    if ((loopCount != 0) && (i==loopCount)) {
      printf("stats after loop %d\n num cmds=%d, avg duration(us)=%d, min duration(us)=%d, max duration(us)=%d\n",
            i,
            (int)cmd_profile[CMD_NUM_SAMPLES],
            (int)(cmd_profile[CMD_AVG_DURATION]/cmd_profile[CMD_NUM_SAMPLES]),
            (int)cmd_profile[CMD_MIN_DURATION],
            (int)cmd_profile[CMD_MAX_DURATION]
          );
      break;
    }
  }

  return ret;
}

static int
util_read_sensor() {
  int ret = 0;
  int i = 0;
  ipmi_sensor_reading_t sensor = {0};

  for (i = 0; i < MAX_SENSOR_NUM; i++) {
    ret = bic_get_sensor_reading(i, &sensor);
    if (ret < 0 ) {
      continue;
    }
    printf("sensor num: 0x%02X: value: 0x%02X, flags: 0x%02X, status: 0x%02X, ext_status: 0x%02X\n",
            i, sensor.value, sensor.flags, sensor.status, sensor.ext_status);
  }
  printf("\n");
  return ret;
}

static int
util_get_sdr() {
#define LAST_RECORD_ID 0xFFFF
#define BYTES_ENTIRE_RECORD 0xFF
  int ret = 0;
  uint8_t rlen = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  ipmi_sel_sdr_req_t req = {0};
  ipmi_sel_sdr_res_t *res = (ipmi_sel_sdr_res_t *) rbuf;

  req.rsv_id = 0;
  req.rec_id = 0;
  req.offset = 0;
  req.nbytes = BYTES_ENTIRE_RECORD;
  for ( ;req.rec_id != LAST_RECORD_ID; ) {
    ret = bic_get_sdr(&req, res, &rlen);
    if (ret) {
      printf("util_get_sdr:bic_get_sdr returns %d\n", ret);
      continue;
    }

    sdr_full_t *sdr = (sdr_full_t *)res->data;

    printf("type: %d, ", sdr->type);
    printf("sensor num: 0x%02X, ", sdr->sensor_num);
    printf("type: 0x%02X, ", sdr->sensor_type);
    printf("evt_read_type: 0x%02X, ", sdr->evt_read_type);
    printf("m_val: 0x%02X, ", sdr->m_val);
    printf("m_tol: 0x%02X, ", sdr->m_tolerance);
    printf("b_val: 0x%02X, ", sdr->b_val);
    printf("b_acc: 0x%02X, ", sdr->b_accuracy);
    printf("acc_dir: 0x%02X, ", sdr->accuracy_dir);
    printf("rb_exp: 0x%02X,\n", sdr->rb_exp);

    req.rec_id = res->next_rec_id;
  }
  printf("This record is LAST record\n");
  printf("\n");
  
  return ret;
}

static int
util_get_postcode() {
  int ret = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0x0};
  uint8_t rlen = 0;
  int i = 0;

  ret = bic_get_80port_record(sizeof(rbuf), rbuf, &rlen);
  if (ret < 0) {
    printf("%s: Failed to get POST code from BIC\n", __func__);
    return ret;
  }

  printf("%s: returns %d bytes\n", __func__, rlen);
  for (i = 0; i < rlen; i++) {
    if (!(i % 16) && i)
      printf("\n");

    printf("%02X ", rbuf[i]);
  }
  printf("\n");
  return ret;
}

static int
util_bic_clear_cmos() {
  int ret = 0;
  uint8_t power_status = 0;
  uint8_t stage = 0;

  ret = pal_get_server_power(FRU_SERVER, &power_status);
  if (ret < 0) {
    printf("Failed to get server power status\n");
    goto out;
  } else if (power_status == SERVER_POWER_ON) {
    printf("Can't performing CMOS clear while server is powered ON\n");
    goto out;
  }
  ret = fbgc_common_get_system_stage(&stage);
  if ((ret < 0) || (stage == STAGE_PRE_EVT) || (stage == STAGE_EVT)) {
    printf("Clear CMOS via bic-util is not supported for system before DVT, please use bios-util instead\n");
    goto out;
  }
  ret = bic_clear_cmos();
  printf("Performing CMOS clear, status %d\n", ret);
  ret = pal_set_nic_perst(NIC_PE_RST_HIGH);
  if (ret < 0) {
    printf("Failed to set nic card PERST high\n");
  }
out:
  return ret;
}

int
bic_comp_init_usb_dev(usb_dev* udev) {
#define TI_VENDOR_ID_SB  0x1CBE
#define TI_PRODUCT_ID 0x0007
  int ret = 0;
  int index = 0;
  char found = 0;
  ssize_t cnt = 0;
  uint16_t vendor_id = TI_VENDOR_ID_SB;
  int recheck = MAX_CHECK_USB_DEV_TIME;

  if (udev == NULL) {
    syslog(LOG_ERR, "%s(): usb device is missing", __func__);
  }
  ret = libusb_init(NULL);
  if (ret < 0) {
    printf("Failed to initialise libusb\n");
    return -1;
  } else {
    printf("Init libusb Successful!\n");
  }

  do {
    cnt = libusb_get_device_list(NULL, &udev->devs);
    if (cnt < 0) {
      printf("There are no USB devices on bus\n");
      return -1;
    }
    index = 0;
    while ((udev->dev = udev->devs[index++]) != NULL) {
      ret = libusb_get_device_descriptor(udev->dev, &udev->desc);
      if (ret < 0) {
        printf("Failed to get device descriptor -- exit\n");
        libusb_free_device_list(udev->devs,1);
        return -1;
      }

      ret = libusb_open(udev->dev,&udev->handle);
      if (ret < 0) {
        printf("Error opening device -- exit\n");
        libusb_free_device_list(udev->devs,1);
        return -1;
      }

      if( (vendor_id == udev->desc.idVendor) && (TI_PRODUCT_ID == udev->desc.idProduct) ) {
        ret = libusb_get_string_descriptor_ascii(udev->handle, udev->desc.iManufacturer, (unsigned char*) udev->manufacturer, sizeof(udev->manufacturer));
        if (ret < 0) {
          printf("Error get Manufacturer string descriptor -- exit\n");
          libusb_free_device_list(udev->devs,1);
          return -1;
        }
        ret = libusb_get_port_numbers(udev->dev, udev->path, sizeof(udev->path));
        if (ret < 0) {
          printf("Error get port number\n");
          libusb_free_device_list(udev->devs,1);
          return -1;
        }

        printf("%04x:%04x (bus %d, device %d)",udev->desc.idVendor, udev->desc.idProduct, libusb_get_bus_number(udev->dev), libusb_get_device_address(udev->dev));
        printf(" path: %d", udev->path[0]);
        for (index = 1; index < ret; index++) {
          printf(".%d", udev->path[index]);
        }
        printf("\n");

        ret = libusb_get_string_descriptor_ascii(udev->handle, udev->desc.iProduct, (unsigned char*) udev->product, sizeof(udev->product));
        if (ret < 0) {
          printf("Error get Product string descriptor -- exit\n");
          libusb_free_device_list(udev->devs,1);
          return -1;
        }

        printf("Manufactured : %s\n",udev->manufacturer);
        printf("Product : %s\n",udev->product);
        printf("----------------------------------------\n");
        printf("Device Descriptors:\n");
        printf("Vendor ID : %x\n",udev->desc.idVendor);
        printf("Product ID : %x\n",udev->desc.idProduct);
        printf("Serial Number : %x\n",udev->desc.iSerialNumber);
        printf("Size of Device Descriptor : %d\n",udev->desc.bLength);
        printf("Type of Descriptor : %d\n",udev->desc.bDescriptorType);
        printf("USB Specification Release Number : %d\n",udev->desc.bcdUSB);
        printf("Device Release Number : %d\n",udev->desc.bcdDevice);
        printf("Device Class : %d\n",udev->desc.bDeviceClass);
        printf("Device Sub-Class : %d\n",udev->desc.bDeviceSubClass);
        printf("Device Protocol : %d\n",udev->desc.bDeviceProtocol);
        printf("Max. Packet Size : %d\n",udev->desc.bMaxPacketSize0);
        printf("No. of Configuraions : %d\n",udev->desc.bNumConfigurations);

        found = 1;
        break;
      }
    }

    if (found != 1) {
      sleep(3);
    } else {
      break;
    }
  } while ((--recheck) > 0);


  if (found == 0) {
    printf("Device NOT found -- exit\n");
    libusb_free_device_list(udev->devs,1);
    return -1;
  }

  ret = libusb_get_configuration(udev->handle, &udev->config);
  if (ret != 0) {
    printf("Error in libusb_get_configuration -- exit\n");
    libusb_free_device_list(udev->devs,1);
    return -1;
  }

  printf("Configured value : %d\n", udev->config);
  if (udev->config != 1) {
    libusb_set_configuration(udev->handle, 1);
    if (ret != 0) {
      printf("Error in libusb_set_configuration -- exit\n");
      libusb_free_device_list(udev->devs,1);
      return -1;
    }
    printf("Device is in configured state!\n");
  }

  libusb_free_device_list(udev->devs, 1);

  if(libusb_kernel_driver_active(udev->handle, udev->ci) == 1) {
    printf("Kernel Driver Active\n");
    if(libusb_detach_kernel_driver(udev->handle, udev->ci) == 0) {
      printf("Kernel Driver Detached!");
    } else {
      printf("Couldn't detach kernel driver -- exit\n");
      libusb_free_device_list(udev->devs,1);
      return -1;
    }
  }

  ret = libusb_claim_interface(udev->handle, udev->ci);
  if (ret < 0) {
    printf("Couldn't claim interface -- exit. err:%s\n", libusb_error_name(ret));
    libusb_free_device_list(udev->devs,1);
    return -1;
  }
  printf("Claimed Interface: %d, EP addr: 0x%02X\n", udev->ci, udev->epaddr);

  active_config(udev->dev, udev->handle);

  return 0;
}

static int
util_check_usb_port() {
  int ret = -1;
  usb_dev  bic_udev;
  usb_dev* udev = &bic_udev;
  int transferlen = MAX_USB_TRANSFER_LEN;
  int transferred = 0;
  uint8_t tbuf[MAX_USB_TRANSFER_LEN] = {0};
  int retries = 3;

  udev->ci = 1;
  udev->epaddr = 0x1;

  // init usb device
  ret = bic_comp_init_usb_dev(udev);
  if (ret < 0) {
    return ret;
  }

  printf("Input test data, USB timeout: 3000ms\n");
  while (true) {
    ret = libusb_bulk_transfer(udev->handle, udev->epaddr, tbuf, transferlen, &transferred, 3000);
    if(((ret != 0) || (transferlen != transferred))) {
      printf("Error in transferring data! err = %d and transferred = %d(expected data length 64)\n",ret ,transferred);
      printf("Retry since  %s\n", libusb_error_name(ret));
      retries--;
      if (!retries) {
        ret = -1;
        break;
      }
      msleep(100);
    } else
      break;
  }

  if (ret != 0) {
    printf("Check USB port Failed\n");
  } else {
    printf("Check USB port Successful\n");
  }

  printf("\n");
  return ret;
}

int
main(int argc, char **argv) {
  uint8_t status = 0;
  int ret = 0;
  uint8_t gpio_num = 0;
  uint8_t gpio_val = 0;

  if (argc < 3) {
    goto err_exit;
  }
  if (strcmp(argv[1], "server") != 0 ) {
    goto err_exit;
  }
  ret = pal_is_fru_prsnt(FRU_SERVER, &status);
  if (ret < 0 || status == FRU_ABSENT) {
    printf("Server is not present!\n");
    return -1;
  }

  if ( strncmp(argv[2], "--", 2) == 0 ) {
    if ( strcmp(argv[2], "--get_gpio") == 0 ) {
      return util_get_gpio();
    } else if ( strcmp(argv[2], "--set_gpio") == 0 ) {
      if ( argc != 5 ) {
        goto err_exit;
      }
      gpio_num = atoi(argv[3]);
      gpio_val = atoi(argv[4]);
      if (gpio_val > 1 ) {
        goto err_exit;
      }
      return util_set_gpio(gpio_num, gpio_val);
    } else if ( strcmp(argv[2], "--get_gpio_config") == 0 ) {
      return util_get_gpio_config();
    } else if ( strcmp(argv[2], "--set_gpio_config") == 0 ) {
      if ( argc != 5 ) {
        goto err_exit;
      }
      gpio_num = atoi(argv[3]);
      gpio_val = (uint8_t)strtol(argv[4], NULL, 0);
      return util_set_gpio_config(gpio_num, gpio_val);
    } else if ( strcmp(argv[2], "--check_status") == 0 ) {
      return util_check_status();
    } else if ( strcmp(argv[2], "--get_dev_id") == 0 ) {
      return util_get_device_id();
    } else if ( strcmp(argv[2], "--get_sdr") == 0 ) {
      return util_get_sdr();
    } else if ( strcmp(argv[2], "--read_sensor") == 0 ) {
      return util_read_sensor();
    } else if ( strcmp(argv[2], "--reset") == 0 ) {
      return util_bic_reset();
    } else if ( strcmp(argv[2], "--get_post_code") == 0 ) {
      return util_get_postcode();
    } else if ( strcmp(argv[2], "--perf_test") == 0 ) {
      if (argc != 4) {
        goto err_exit;
      } else{
        return util_perf_test(atoi(argv[3]));
      }
    } else if (!strcmp(argv[2], "--clear_cmos")) {
      if (argc != 3) {
        goto err_exit;
      }
      return util_bic_clear_cmos();
    } else if (strcmp(argv[2], "--file") == 0) {
      if ( argc != 4 ) {
        goto err_exit;
      }
      return process_file(argv[3]);
    } else if ( strcmp(argv[2], "--check_usb_port") == 0 ) {
      if ( argc != 3 ) {
        goto err_exit;
      }
      return util_check_usb_port();
    } else {
      printf("Invalid option: %s\n", argv[2]);
      goto err_exit;
    }
  } else if (argc >= 4) {
    if (util_is_numeric(argv + 2)) {
      return process_command((argc - 2), (argv + 2));
    } else {
      goto err_exit;
    }
  } else {
    goto err_exit;
  }

err_exit:
  print_usage_help();
  return -1;
}
