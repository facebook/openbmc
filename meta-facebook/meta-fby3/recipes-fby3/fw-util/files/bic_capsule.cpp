#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <sys/stat.h>
#include "bic_capsule.h"
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

#define MAX_RETRY 30
#define MUX_SWITCH_CPLD 0x07
#define MUX_SWITCH_PCH 0x03
#define GPIO_HIGH 1
#define GPIO_LOW 0
#define RST_RSMRST_BMC_N_IDX 71

#define UPDATE_INTENT_OFFSET 0x12
#define BIOS_UPDATE_INTENT 0x41
#define CPLD_UPDATE_INTENT 0x04
#define BIOS_UPDATE_RCVY_INTENT 0x02
#define CPLD_UPDATE_RCVY_INTENT 0x20
#define POLLING_INTERVAL 10
#define CPLD_UPDATE_TIMEOUT 20
#define BIOS_UPDATE_TIMEOUT 50
#define PLATFORM_STATE_ENTER_T0 0x09

#define BIT_VALUE(list, index) \
           ((((uint8_t*)&list)[index/8]) >> (index % 8)) & 0x1\

image_info CapsuleComponent::check_image(string image, bool force) {
  string fru_name = fru();
  string slot_str = "slot";
  size_t slot_found = fru_name.find(slot_str);
  struct stat f_stat;

  image_info image_sts = {"", false};

  if (fw_comp != FW_CPLD_CAPSULE && fw_comp != FW_CPLD_RCVY_CAPSULE) {
    image_sts.result = true;
    return image_sts;
  }

  //create a new tmp file
  image_sts.new_path = image + "-tmp";

  //open the binary
  int fd_r = open(image.c_str(), O_RDONLY);
  if (fd_r < 0) {
    cerr << "Cannot open " << image << " for reading" << endl;
    return image_sts;
  }

  // create a tmp file for writing.
  int fd_w = open(image_sts.new_path.c_str(), O_WRONLY | O_CREAT, 0666);
  if (fd_w < 0) {
    cerr << "Cannot write to " << image_sts.new_path << endl;
    close(fd_r);
    return image_sts;
  }

  if (stat(image.c_str(), &f_stat) == -1) {
    return image_sts;
  }
  long size = (long)f_stat.st_size;

  uint8_t *memblock = new uint8_t [size];//data_size + signed byte
  uint8_t signed_byte = 0;
  size_t r_b = read(fd_r, memblock, size);
  size_t w_b = 0;

  signed_byte = memblock[size - 1] & 0xf;
  r_b = r_b - 1;  //only write data to tmp file

  w_b = write(fd_w, memblock, r_b);

  //check size
  if ( r_b != w_b ) {
    cerr << "Cannot create the tmp file - " << image_sts.new_path << endl;
    cerr << "Read: " << r_b << " Write: " << w_b << endl;
    image_sts.result = false;
  }

  if ( force == false ) {
    //CPLD is located on class 2(NIC_EXP)
    if (signed_byte == BICDL && (slot_found != string::npos)) {
      image_sts.result = true;
    } else {
      cout << "image is not a valid CPLD image for " << fru_name << endl;
    }
  }

  //release the resource
  close(fd_r);
  close(fd_w);
  delete[] memblock;

  return image_sts;
}

int CapsuleComponent::bic_update_capsule(string image) {
  int ret;
  uint8_t status;
  uint8_t tbuf[2] = {0}, rbuf[1] = {0};
  uint8_t tlen = 1, rlen = 1;
  char path[128];
  int i2cfd = 0, retry=0;
  bic_gpio_t gpio = {0};
  uint32_t check_update_time = 0, time_cnt = 0, update_timeout = 0, retry_count = 0;
  uint8_t plt_state = 0, panic_cnt_o = 0, panic_cnt_n = 0;
  int is_check_panic_cnt = 1;

  // Check PFR provision status
  snprintf(path, sizeof(path), "/dev/i2c-%d", (slot_id+SLOT_BUS_BASE));
  i2cfd = open(path, O_RDWR);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %s", __func__, path);
  }
  retry = 0;
  tbuf[0] = UFM_STATUS_OFFSET;
  tlen = 1;
  while (retry < RETRY_TIME) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if ( i2cfd > 0 ) close(i2cfd);

  if (!(rbuf[0] & UFM_PROVISIONED_MSK)) {
    cerr << "Server " << +slot_id << " is un-provisioned. Stopping the update!" << endl;
    return -1;
  }

  try {
    server.ready();
    pal_set_server_power(slot_id, SERVER_GRACEFUL_SHUTDOWN);

    //Checking Server Power Status to make sure Server is really Off
    while (retry_count < MAX_RETRY) {
      ret = pal_get_server_power(slot_id, &status);
      if ( (ret == 0) && (status == SERVER_POWER_OFF) ){
        break;
      } else {
        retry_count++;
        sleep(2);
      }
    }
    if (retry_count == MAX_RETRY) {
      cerr << "Failed to Power Off Server " << +slot_id << ". Stopping the update!" << endl;
      return -1;
    }

    me_recovery(slot_id, RECOVERY_MODE);
    bic_set_gpio(slot_id, RST_USB_HUB_N, GPIO_HIGH);
    sleep(3);
    bic_switch_mux_for_bios_spi(slot_id, MUX_SWITCH_CPLD);
    sleep(1);
    auto start = chrono::steady_clock::now();
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_SET);
    if (ret != 0) {
      return -1;
    }
    bic_switch_mux_for_bios_spi(slot_id, MUX_SWITCH_PCH);
    bic_set_gpio(slot_id, RST_USB_HUB_N, GPIO_LOW);
    sleep(1);

    i2cfd = open(path, O_RDWR);
    if ( i2cfd < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to open %s", __func__, path);
    }

    // Take panic count before update
    retry = 0;
    tlen = 1;
    tbuf[0] = PANIC_CNT_OFFSET;
    tlen = 1;
    while (retry < RETRY_TIME) {
      ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
      if ( ret < 0 ) {
        retry++;
        msleep(100);
      } else {
        break;
      }
    }
    panic_cnt_o = rbuf[0];
    if ( retry == RETRY_TIME ) return -1;

    // Defined update configuration
    tbuf[0] = UPDATE_INTENT_OFFSET;
    if (fw_comp == FW_BIOS_CAPSULE) {
      tbuf[1] = BIOS_UPDATE_INTENT;
      check_update_time = 1;
      update_timeout = BIOS_UPDATE_TIMEOUT;
    } else if (fw_comp == FW_CPLD_CAPSULE) {
      tbuf[1] = CPLD_UPDATE_INTENT;
      check_update_time = 1;
      update_timeout = CPLD_UPDATE_TIMEOUT;
    } else if (fw_comp == FW_BIOS_RCVY_CAPSULE) {
      tbuf[1] = BIOS_UPDATE_RCVY_INTENT;
      check_update_time = 2;
      update_timeout = BIOS_UPDATE_TIMEOUT*2;
    } else if (fw_comp == FW_CPLD_RCVY_CAPSULE) {
      tbuf[1] = CPLD_UPDATE_RCVY_INTENT;
      check_update_time = 2;
      update_timeout = CPLD_UPDATE_TIMEOUT*2;
    } else {
      return -1;
    }

    // Update intent to CPLD
    retry = 0;
    tlen = 2;
    while (retry < RETRY_TIME) {
      ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, NULL, 0);
      if ( ret < 0 ) {
        retry++;
        msleep(100);
      } else {
        break;
      }
    }
    if ( retry == RETRY_TIME ) return -1;

    ret = pal_check_pfr_mailbox(slot_id);
    if ( ret < 0 ) {
      return -1;
    }

    // Check update status
    for (retry_count = 0; retry_count < check_update_time; retry_count++) {
      if (retry_count == 0) {
        printf("Capsule active update is ongoing ...\n");
      } else {
        printf("Capsule recovery update is ongoing ...\n");
      }
      time_cnt = 0;
      // Timeout mechanism to avoid updating failed
      while (time_cnt < update_timeout) {
        ret = bic_get_gpio(slot_id, &gpio);
        if ( ret < 0 ) {
          printf("%s() bic_get_gpio returns %d\n", __func__, ret);
          return ret;
        }

        if (BIT_VALUE(gpio, RST_RSMRST_BMC_N_IDX)) {
          // Take platform state
          retry = 0;
          tlen = 1;
          tbuf[0] = PLATFORM_STATE_OFFSET;
          tlen = 1;
          while (retry < RETRY_TIME) {
            ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
            if ( ret < 0 ) {
              retry++;
              msleep(100);
            } else {
              break;
            }
          }
          plt_state = rbuf[0];

          // Take panic count after update
          retry = 0;
          tlen = 1;
          tbuf[0] = PANIC_CNT_OFFSET;
          tlen = 1;
          while (retry < RETRY_TIME) {
            ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
            if ( ret < 0 ) {
              retry++;
              msleep(100);
            } else {
              break;
            }
          }
          panic_cnt_n = rbuf[0];
        }

        // When PLATFORM_STATE_ENTER_T0 is 0x09 and panic count is changed, it means that firmware update successfully.
        if (fw_comp != FW_CPLD_CAPSULE) {
          if ((fw_comp == FW_CPLD_RCVY_CAPSULE) && (retry_count == 0)) {
            is_check_panic_cnt = 1;
          } else {
            is_check_panic_cnt = (panic_cnt_n > panic_cnt_o);
          }
        }
        if ((plt_state >= PLATFORM_STATE_ENTER_T0) && (is_check_panic_cnt)) {
          break;
        } else {
          time_cnt++;
          sleep(POLLING_INTERVAL);
        }
      }

      if (time_cnt == update_timeout) {
        printf("Get PFR update status timeout \n");
        return -1;
      }

      panic_cnt_o = panic_cnt_n;
    }

    auto end = chrono::steady_clock::now();
    cout << "Elapsed time:  " << chrono::duration_cast<chrono::seconds>(end - start).count() << "   sec." << endl;

    pal_set_server_power(slot_id, SERVER_POWER_ON);

    ret = pal_check_pfr_mailbox(slot_id);
    if ( ret < 0 ) {
      return -1;
    }

    if ( i2cfd > 0 ) close(i2cfd);

  } catch(string err) {
    if ( i2cfd > 0 ) close(i2cfd);
    return FW_STATUS_NOT_SUPPORTED;
  }

  return ret;
}

int CapsuleComponent::update(string image) {
  int ret;
  image_info image_sts = check_image(image, false);

  if ( image_sts.result == false ) {
    remove(image_sts.new_path.c_str());
    return FW_STATUS_FAILURE;
  }

  //use the new path
  image = image_sts.new_path;

  ret = bic_update_capsule(image);

  //remove the tmp file
  remove(image.c_str());
  return ret;
}

int CapsuleComponent::fupdate(string image) {
  int ret;

  ret = bic_update_capsule(image);

  //remove the tmp file
  remove(image.c_str());
  return ret;
}

int CapsuleComponent::get_pfr_recovery_ver_str(string& s) {
  int ret = 0;
  char ver[32] = {0};
  uint32_t ver_reg = 0x60;
  uint8_t tbuf[1] = {0x00};
  uint8_t rbuf[4] = {0x00};
  uint8_t tlen = 1;
  uint8_t rlen = 4;
  int i2cfd = 0;

  memcpy(tbuf, (uint8_t *)&ver_reg, tlen);
  string i2cdev = "/dev/i2c-" + to_string((slot_id+SLOT_BUS_BASE));

  if ((i2cfd = open(i2cdev.c_str(), O_RDWR)) < 0) {
    printf("Failed to open %s\n", i2cdev.c_str());
    return -1;
  }

  if (ioctl(i2cfd, I2C_SLAVE, CPLD_INTENT_CTRL_ADDR) < 0) {
    printf("Failed to talk to slave@0x%02X\n", CPLD_INTENT_CTRL_ADDR);
  } else {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
    snprintf(ver, sizeof(ver), "%02X%02X%02X%02X", rbuf[3], rbuf[2], rbuf[1], rbuf[0]);
  }

  if ( i2cfd > 0 ) close(i2cfd);
  s = string(ver);
  return ret;
}

int CapsuleComponent::print_version() {
  int ret, i2cfd = 0, retry=0;;
  uint8_t tbuf[2] = {0}, rbuf[1] = {0};
  uint8_t tlen = 1, rlen = 1;
  char path[128];
  string fru_name = fru();
  string ver("");

  // IF PFR active , get the recovery capsule firmware version
  // Check PFR provision status
  snprintf(path, sizeof(path), "/dev/i2c-%d", (slot_id + SLOT_BUS_BASE));
  i2cfd = open(path, O_RDWR);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %s", __func__, path);
    return -1;
  }
  retry = 0;
  tbuf[0] = UFM_STATUS_OFFSET;
  tlen = 1;
  while (retry < RETRY_TIME) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_INTENT_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if ( i2cfd > 0 ) close(i2cfd);

  if (!(rbuf[0] & UFM_PROVISIONED_MSK)) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  try {
    if (fw_comp == FW_CPLD_RCVY_CAPSULE) {
      if ( get_pfr_recovery_ver_str(ver) < 0 ) {
        throw "Error in getting the version of " + fru_name;
      }
      cout << "SB CPLD Recovery Capsule Version: " << ver << endl;
    } else {
      return FW_STATUS_NOT_SUPPORTED;
    }
  } catch(string& err) {
    printf("SB CPLD Version: NA (%s)\n", err.c_str());
  }

  return 0;
}

#endif
