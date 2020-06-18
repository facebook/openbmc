#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include "bic_capsule.h"
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

#define MAX_RETRY 30
#define MUX_SWITCH_CPLD 0x07
#define MUX_SWITCH_PCH 0x03
#define GPIO_RST_USB_HUB 0x10
#define GPIO_HIGH 1
#define GPIO_LOW 0

#define UPDATE_INTENT_OFFSET 0x12
#define BIOS_UPDATE_INTENT 0x41
#define CPLD_UPDATE_INTENT 0x04
#define BIOS_UPDATE_RCVY_INTENT 0x02
#define CPLD_UPDATE_RCVY_INTENT 0x20

int CapsuleComponent::update(string image) {
  int ret;
  uint8_t status;
  int retry_count = 0;
  uint8_t tbuf[2] = {0}, rbuf[1] = {0};
  uint8_t tlen = 1, rlen = 1;
  char path[128];
  int i2cfd = 0, retry=0;

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
    bic_set_gpio(slot_id, GPIO_RST_USB_HUB, GPIO_HIGH);
    sleep(3);
    bic_switch_mux_for_bios_spi(slot_id, MUX_SWITCH_CPLD);
    sleep(1);
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_SET);
    if (ret != 0) {
      return -1;
    }
    bic_switch_mux_for_bios_spi(slot_id, MUX_SWITCH_PCH);
    bic_set_gpio(slot_id, GPIO_RST_USB_HUB, GPIO_LOW);
    sleep(1);
    tbuf[0] = UPDATE_INTENT_OFFSET;
    if (fw_comp == FW_BIOS_CAPSULE) {
      tbuf[1] = BIOS_UPDATE_INTENT;
    } else if (fw_comp == FW_CPLD_CAPSULE) {
      tbuf[1] = CPLD_UPDATE_INTENT;
    } else if (fw_comp == FW_BIOS_RCVY_CAPSULE) {
      tbuf[1] = BIOS_UPDATE_RCVY_INTENT;
    } else if (fw_comp == FW_CPLD_RCVY_CAPSULE) {
      tbuf[1] = CPLD_UPDATE_RCVY_INTENT;
    } else {
      return -1;
    }
    // update intent
    i2cfd = open(path, O_RDWR);
    if ( i2cfd < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to open %s", __func__, path);
    }
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
    if ( i2cfd > 0 ) close(i2cfd);

  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int CapsuleComponent::fupdate(string image) {
  return FW_STATUS_NOT_SUPPORTED;
}

int CapsuleComponent::print_version() {
  return FW_STATUS_NOT_SUPPORTED;
}

#endif
