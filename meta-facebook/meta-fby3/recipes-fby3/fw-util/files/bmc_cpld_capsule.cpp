#include <string>
#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <syslog.h>
#include <openbmc/obmc-i2c.h>
#include "bmc_cpld_capsule.h"
#include <facebook/bic.h>
#include <openbmc/pal.h>

using namespace std;

image_info BmcCpldCapsuleComponent::check_image(string image, bool force) {
  uint8_t bmc_location = 0;
  string fru_name = fru();
  string bmc_str = "bmc";
  size_t bmc_found = fru_name.find(bmc_str);
  struct stat f_stat;
  string comp = component();

  image_info image_sts = {"", false};

 if (comp != "cpld_cap" && comp != "cpld_cap_rcvy") {
    image_sts.result = true;
    return image_sts;
  }

  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    printf("Failed to initialize the fw-util\n");
    exit(EXIT_FAILURE);
  }

  //create a new tmp image
  image_sts.new_path = image + "-tmp";

  //open the binary
  int fd_r = open(image.c_str(), O_RDONLY);
  if (fd_r < 0) {
    cerr << "Cannot open " << image << " for reading" << endl;
    return image_sts;
  }

  // create a tmp image for writing.
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
  r_b = r_b - 1;  //only write data to tmp image

  w_b = write(fd_w, memblock, r_b);

  //check size
  if ( r_b != w_b ) {
    cerr << "Cannot create the tmp image - " << image_sts.new_path << endl;
    cerr << "Read: " << r_b << " Write: " << w_b << endl;
    image_sts.result = false;
  }

  if ( force == false ) {
    //CPLD is located on class 2(NIC_EXP)
    if ( bmc_location == NIC_BMC ) {
      if ( (signed_byte == NICEXP) && (bmc_found != string::npos) ) {
        image_sts.result = true;
      } else {
        cout << "image is not a valid CPLD image for " << fru_name << endl;
      }
    } else if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
      if ( (signed_byte == BICBB) && (bmc_found != string::npos) ) {
        image_sts.result = true;
      } else {
        cout << "image is not a valid CPLD image for " << fru_name << endl;
      }
    }
  }

  //release the resource
  close(fd_r);
  close(fd_w);
  delete[] memblock;

  return image_sts;
}

int BmcCpldCapsuleComponent::bmc_update_capsule(string image) {
  FILE *fp;
  int i2cfd = 0;
  int ret = 0;
  int retry;
  uint8_t bmc_location = 0;
  uint8_t tbuf[2] = {0}, rdbuf[1] = {0};
  uint8_t tlen = 2, rlen = 1;
  int pfr_adress = CPLD_INTENT_CTRL_ADDR;
  int mtd_no;
  char rbuf[256], mtd_name[32], dev[16] = {0}, cmd[256] = {0};
  char path[128];
  string bmc_location_str;
  string comp = component();

  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    printf("Failed to initialize the fw-util\n");
    return FW_STATUS_FAILURE;
  }

  if ( bmc_location == NIC_BMC ) {
    bmc_location_str = "NIC Expansion";
    snprintf(path, sizeof(path), NIC_CPLD_CTRL_BUS);
  } else {
    bmc_location_str = "baseboard";
    snprintf(path, sizeof(path), BB_CPLD_CTRL_BUS);
  }

  // Check PFR provision status
  i2cfd = open(path, O_RDWR);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %s", __func__, path);
    return -1;
  }

  retry = 0;
  tlen = 1;
  tbuf[0] = UFM_STATUS_OFFSET;
  while (retry < RETRY_TIME) {
    ret = i2c_rdwr_msg_transfer(i2cfd, pfr_adress, tbuf, tlen, rdbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if ( i2cfd > 0 ) close(i2cfd);
  if (retry == RETRY_TIME) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    return -1;
  }

  if (!(rdbuf[0] & UFM_PROVISIONED_MSK)) {
    printf("BMC is un-provisioned. Stopping the update! \n");
    return -1;
  }

  tbuf[0] = CPLD_INTENT_CTRL_OFFSET;
  printf("Start to update capsule...\n");
  if (comp == "bmc_cap" || comp == "bmc_cap_rcvy") {
    if ((fp = fopen("/proc/mtd", "r"))) {
      while (fgets(rbuf, sizeof(rbuf), fp)) {
        if ((sscanf(rbuf, "mtd%d: %*x %*x %s", &mtd_no, mtd_name) == 2) &&
            !strcmp("\"stg-bmc\"", mtd_name)) {
          sprintf(dev, "/dev/mtd%d", mtd_no);
          break;
        }
      }
      fclose(fp);
    }

    if (!dev[0] || !(fp = fopen(dev, "rb"))) {
      printf("stg-bmc not found\n");
      return FW_STATUS_FAILURE;
    }
    fclose(fp);

    if (comp == "bmc_cap_rcvy") {
      tbuf[1] = BMC_INTENT_RCVY_VALUE;
      syslog(LOG_CRIT, "Updating BMC Capsule, Target to Recovery Region on %s. File: %s", bmc_location_str.c_str(), image.c_str());
    } else {
      tbuf[1] = BMC_INTENT_VALUE;
      syslog(LOG_CRIT, "Updating BMC Capsule, Target to Active Region on %s. File: %s", bmc_location_str.c_str(), image.c_str());
    }

    snprintf(cmd, sizeof(cmd), "/usr/sbin/flashcp -v %s %s", image.c_str(), dev);
    if (system(cmd) != 0) {
        syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
        return FW_STATUS_FAILURE;
    }

    if (comp == "bmc_cap_rcvy") {
      syslog(LOG_CRIT, "Updated BMC Capsule, Target to Recovery Region on %s. File: %s", bmc_location_str.c_str(), image.c_str());
    } else {
      syslog(LOG_CRIT, "Updated BMC Capsule, Target to Active Region on %s. File: %s", bmc_location_str.c_str(), image.c_str());
    }
  } else if (comp == "cpld_cap" || comp == "cpld_cap_rcvy") {
    if ((fp = fopen("/proc/mtd", "r"))) {
      while (fgets(rbuf, sizeof(rbuf), fp)) {
        if ((sscanf(rbuf, "mtd%d: %*x %*x %s", &mtd_no, mtd_name) == 2) &&
            !strcmp("\"stg-cpld\"", mtd_name)) {
          sprintf(dev, "/dev/mtd%d", mtd_no);
          break;
        }
      }
      fclose(fp);
    }

    if (!dev[0] || !(fp = fopen(dev, "rb"))) {
      printf("stg-cpld not found\n");
      return FW_STATUS_FAILURE;
    }

    if (comp == "cpld_cap_rcvy") {
      tbuf[1] = CPLD_INTENT_RCVY_VALUE;
      syslog(LOG_CRIT, "Updating CPLD Capsule, Target to Recovery Region on %s. File: %s", bmc_location_str.c_str(), image.c_str());
    } else {
      tbuf[1] = CPLD_INTENT_VALUE;
      syslog(LOG_CRIT, "Updating CPLD Capsule, Target to Active Region on %s. File: %s", bmc_location_str.c_str(), image.c_str());
    }

    snprintf(cmd, sizeof(cmd), "/usr/sbin/flashcp -v %s %s", image.c_str(), dev);
    if (system(cmd) != 0) {
        syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
        return FW_STATUS_FAILURE;
    }

    if (comp == "cpld_cap_rcvy") {
      syslog(LOG_CRIT, "Updated CPLD Capsule, Target to Recovery Region on %s. File: %s", bmc_location_str.c_str(), image.c_str());
    } else {
      syslog(LOG_CRIT, "Updated CPLD Capsule, Target to Active Region on %s. File: %s", bmc_location_str.c_str(), image.c_str());
    }
  } else {
    return FW_STATUS_NOT_SUPPORTED;
  }

  // Update Intent
  i2cfd = open(path, O_RDWR);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %s", __func__, path);
    return -1;
  }

  retry = 0;
  tlen = 2;
  while (retry < RETRY_TIME) {
    ret = i2c_rdwr_msg_transfer(i2cfd, pfr_adress, tbuf, tlen, NULL, 0);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      break;
    }
  }
  if ( i2cfd > 0 ) close(i2cfd);
  if (retry == RETRY_TIME) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    return -1;
  }

  // If intent failed
  // Check the error code
  sleep(2);
  ret = pal_check_pfr_mailbox(FRU_BMC);
  if ( ret < 0 ) {
      return -1;
  }

  return 0;
}

int BmcCpldCapsuleComponent::update(string image) {
  int ret;
  image_info image_sts = check_image(image, false);

  if ( image_sts.result == false ) {
    remove(image_sts.new_path.c_str());
    return FW_STATUS_FAILURE;
  }

  //use the new path
  image = image_sts.new_path;

  ret = bmc_update_capsule(image);

  //remove the tmp image
  remove(image.c_str());
  return ret;
}

int BmcCpldCapsuleComponent::fupdate(string image) {
  int ret;

  ret = bmc_update_capsule(image);

  //remove the tmp image
  remove(image.c_str());
  return ret;
}

int BmcCpldCapsuleComponent::get_pfr_recovery_ver_str(string& s) {
  int ret = 0;
  char ver[32] = {0};
  uint32_t ver_reg = 0x60;
  uint8_t tbuf[1] = {0x00};
  uint8_t rbuf[4] = {0x00};
  uint8_t tlen = 1;
  uint8_t rlen = 4;
  int i2cfd = 0;

  memcpy(tbuf, (uint8_t *)&ver_reg, tlen);
  string i2cdev = "/dev/i2c-" + to_string(bus);

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

int BmcCpldCapsuleComponent::print_version()
{
  int ret, i2cfd = 0, retry=0;;
  uint8_t tbuf[2] = {0}, rbuf[1] = {0};
  uint8_t tlen = 1, rlen = 1;
  char path[128];
  string comp = component();
  string ver("");

  // IF PFR active , get the recovery capsule firmware version
  // Check PFR provision status
  string i2cdev = "/dev/i2c-" + to_string(bus);
  i2cfd = open(i2cdev.c_str(), O_RDWR);
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
    if (comp == "cpld_cap_rcvy") {
      if ( get_pfr_recovery_ver_str(ver) < 0 ) {
        throw "Error in getting the version of BMC CPLD Recovery Capsule";
      }
      cout << "BMC CPLD Recovery Capsule Version: " << ver << endl;
    } else {
      return FW_STATUS_NOT_SUPPORTED;
    }
  } catch(string& err) {
    printf("BMC CPLD Recovery Capsule Version: NA (%s)\n", err.c_str());
  }

  return 0;
}