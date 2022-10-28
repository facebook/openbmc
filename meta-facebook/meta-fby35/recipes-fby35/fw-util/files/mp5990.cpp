#include <cstdio>
#include <syslog.h>
#include <unistd.h>
#include "mp5990.h"
#include <facebook/bic.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>

#ifndef MAX_RETRY
#define MAX_RETRY 3
#endif

using namespace std;

int MP5990Component::get_ver_str(string& s) {
  uint16_t checksum = 0;
  uint8_t tbuf[1] = {0}, rbuf[2] = {0};
  int ret = 0;
  int i2cfd = -1;
  int i = 0;

  tbuf[0] = MP5990_REG_CHECKSUM;
  switch (fru_id) {
    case FRU_BMC:
      i2cfd = i2c_cdev_slave_open(bus, addr, I2C_SLAVE_FORCE_CLAIM);
      if (i2cfd < 0) {
        syslog(LOG_WARNING, "%s() Failed to open %d", __func__, bus);
        return -1;
      }    
      for (i = 0; i < MAX_RETRY; i++) {        
        ret = i2c_rdwr_msg_transfer(i2cfd, addr << 1, tbuf, 1, rbuf, 2);
        if (ret == 0) {
          break;
        }
        usleep(10 * 1000);
      }
      if (i2cfd >= 0) {
        close(i2cfd);
      }
      if (i == MAX_RETRY) {
        return FW_STATUS_FAILURE;
      }
      break;
    case FRU_SLOT1:
    case FRU_SLOT2:
    case FRU_SLOT3:
    case FRU_SLOT4:
      ret = bic_master_write_read(fru_id, (bus << 1) | 1, addr, tbuf, 1, rbuf, 2);
      if (ret < 0) {
        return FW_STATUS_FAILURE;
      }
      break;
    default:
      return FW_STATUS_NOT_SUPPORTED;
  }
  checksum = (rbuf[1] << 8) | rbuf[0];
  stringstream ss;
  ss << uppercase << hex << checksum;
  s = ss.str();

  return FW_STATUS_SUCCESS;
}

int MP5990Component::print_version() {
  string ver("");
  try {
    if (pal_is_slot_server(fru_id) == true){
      server.ready();
    }
    if ( get_ver_str(ver) < 0 ) {
      throw string("Error in getting the version of HSC");
    }
    cout << "MP5990 HSC Checksum: " << ver << endl;
  } catch(string& err) {
    printf("MP5990 HSC Checksum: NA (%s)\n", err.c_str());
  }

  return FW_STATUS_SUCCESS;
}

int MP5990Component::get_version(json& j) {
  string ver("");
  try {
    if (pal_is_slot_server(fru_id) == true){
      server.ready();
    }
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of HSC";
    }
    j["VERSION"] = ver;
  } catch(string& err) {
    j["VERSION"] = "NA";
  }
  return FW_STATUS_SUCCESS;
}
