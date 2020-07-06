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

int BmcCpldCapsuleComponent::update(string image)
{
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
  char *file;
  char path[128];
  string bmc_location_str;
  string comp = component();

  file = (char *)image.c_str();
  if (access(file, F_OK) == -1) {
    printf("Missing firmware file\n");
    return -1;
  }

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
      syslog(LOG_CRIT, "Updating BMC Capsule, Target to Recovery Region on %s. File: %s", bmc_location_str.c_str(), file);
    } else {
      tbuf[1] = BMC_INTENT_VALUE;
      syslog(LOG_CRIT, "Updating BMC Capsule, Target to Active Region on %s. File: %s", bmc_location_str.c_str(), file);
    }

    snprintf(cmd, sizeof(cmd), "/usr/sbin/flashcp -v %s %s", file, dev);
    if (system(cmd) != 0) {
        syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
        return FW_STATUS_FAILURE;
    }

    if (comp == "bmc_cap_rcvy") {
      syslog(LOG_CRIT, "Updated BMC Capsule, Target to Recovery Region on %s. File: %s", bmc_location_str.c_str(), file);
    } else {
      syslog(LOG_CRIT, "Updated BMC Capsule, Target to Active Region on %s. File: %s", bmc_location_str.c_str(), file);
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
      syslog(LOG_CRIT, "Updating CPLD Capsule, Target to Recovery Region on %s. File: %s", bmc_location_str.c_str(), file);
    } else {
      tbuf[1] = CPLD_INTENT_VALUE;
      syslog(LOG_CRIT, "Updating CPLD Capsule, Target to Active Region on %s. File: %s", bmc_location_str.c_str(), file);
    }

    snprintf(cmd, sizeof(cmd), "/usr/sbin/flashcp -v %s %s", file, dev);
    if (system(cmd) != 0) {
        syslog(LOG_WARNING, "[%s] %s failed\n", __func__, cmd);
        return FW_STATUS_FAILURE;
    }

    if (comp == "cpld_cap_rcvy") {
      syslog(LOG_CRIT, "Updated CPLD Capsule, Target to Recovery Region on %s. File: %s", bmc_location_str.c_str(), file);
    } else {
      syslog(LOG_CRIT, "Updated CPLD Capsule, Target to Active Region on %s. File: %s", bmc_location_str.c_str(), file);
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

int BmcCpldCapsuleComponent::fupdate(string image) 
{
  return FW_STATUS_NOT_SUPPORTED;
}

int BmcCpldCapsuleComponent::print_version()
{
  return FW_STATUS_NOT_SUPPORTED;
}