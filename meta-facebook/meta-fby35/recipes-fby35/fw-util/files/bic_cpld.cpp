#include <cstdio>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/kv.h>
#include <facebook/fby35_common.h>
#include <facebook/bic.h>
#include <facebook/bic_ipmi.h>
#include "bic_cpld.h"

using std::string;
#define MAX10_RPD_SIZE 0x23000

image_info CpldComponent::check_image(const string& image, bool force) {
  int ret = 0;
  uint8_t board_id = 0, board_rev = 0;
  image_info image_sts = {"", false, false};

  if (force == true) {
    image_sts.result = true;
  }

  if (fw_comp == FW_CPLD) {
    if (fby35_common_get_slot_type(slot_id) == SERVER_TYPE_HD) {
      board_id = BOARD_ID_HD;
    } else {
      board_id = BOARD_ID_SB;
    }
    ret = get_board_rev(slot_id, BOARD_ID_SB, &board_rev);
  } else {  // CPLD on BIC Baseboard (Class 2)
    ret = get_board_rev(0, BOARD_ID_BB, &board_rev);
  }
  if (ret < 0) {
    cerr << "Failed to get board revision ID" << endl;
    return image_sts;
  }

  if (fby35_common_is_valid_img(image.c_str(), fw_comp, board_id, board_rev) == true) {
    image_sts.result = true;
    image_sts.sign = true;
  }

  return image_sts;
}

// Refresh CPLD to activate new image in case CPLD can not do 12V cycle when fw is broken
int CpldComponent::cpld_refresh(uint8_t bus_id, uint8_t addr) {
  int retry = 3;
  int i2cfd = -1;
  int ret = 0;
  uint8_t tbuf[3] = {0}, rbuf[1] = {0}; 
  
  i2cfd = i2c_cdev_slave_open(bus_id, addr, I2C_SLAVE_FORCE_CLAIM);
  if (i2cfd < 0) {
    syslog(LOG_WARNING, "%s() Failed to open %d", __func__, bus_id);
    return -1;
  }    
  while (retry-- > 0) {
    tbuf[0] = 0x79; // CPLD register
    ret = i2c_rdwr_msg_transfer(i2cfd, addr << 1, tbuf, 3, rbuf, 0);
    if (ret == 0) {
      break;
    }
    usleep(10 * 1000);
  }
  if (retry == 0) {
    cerr << "Fail to refresh CPLD to activate image" << endl;
  }

  if (i2cfd >= 0) {
    close(i2cfd);
  }
  return ret;
}

int CpldComponent::update_cpld(const string& image, bool force) {
  int ret = FW_STATUS_FAILURE;
  char ver_key[MAX_KEY_LEN] = {0};
  char ver[16] = {0};
  uint8_t bmc_location = 0;
  image_info image_sts = check_image(image, force);

  ret = fby35_common_get_bmc_location(&bmc_location);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s() Cannot get the location of BMC", __func__);
    return ret;
  }

  if (image_sts.result == false) {
    syslog(LOG_CRIT, "Update %s on %s Fail. File: %s is not a valid image",
           get_component_name(fw_comp), fru().c_str(), image.c_str());
    return FW_STATUS_FAILURE;
  }

  if (force == false) {
    try {
      server.ready();
      expansion.ready();
    } catch (string& err) {
      printf("%s\n", err.c_str());
      return FW_STATUS_NOT_SUPPORTED;
    }
  }

  if (fw_comp == FW_CPLD) {
    syslog(LOG_CRIT, "Updating %s on %s. File: %s", get_component_name(fw_comp),
           fru().c_str(), image.c_str());

    if (cpld_intf_open(pld_type, INTF_I2C, &attr) == 0) {
      ret = cpld_program((char *)image.c_str(), NULL, false);
      cpld_intf_close();
      if (ret < 0) {
        printf("Error Occur at updating CPLD FW!\n");
        ret = FW_STATUS_FAILURE;
      }
    } else {
      printf("Cannot open i2c!\n");
      ret = FW_STATUS_FAILURE;
    }
    if ((force == true) && (bmc_location == NIC_BMC)) {
      cpld_refresh(attr.bus_id, attr.slv_addr);
    }
    syslog(LOG_CRIT, "Updated %s on %s. File: %s. Result: %s", get_component_name(fw_comp),
           fru().c_str(), image.c_str(), (ret) ? "Fail" : "Success");
  } else {  // CPLD on BIC Baseboard (Class 2)
    // create a tmp file
    int fd_r = open(image.c_str(), O_RDONLY);
    if (fd_r < 0) {
      cerr << "Cannot open " << image << " for reading" << endl;
      return FW_STATUS_FAILURE;
    }

    image_sts.new_path = image + "-tmp";
    int fd_w = open(image_sts.new_path.c_str(), O_WRONLY | O_CREAT, 0666);
    if (fd_w < 0) {
      cerr << "Cannot write to " << image_sts.new_path << endl;
      close(fd_r);
      return FW_STATUS_FAILURE;
    }

    uint8_t *memblock = new uint8_t [MAX10_RPD_SIZE];
    int r_b, w_b;
    for (r_b = 0; r_b < MAX10_RPD_SIZE;) {
      int rc = read(fd_r, memblock + r_b, MAX10_RPD_SIZE - r_b);
      if (rc > 0) {
        r_b += rc;
      } else {
        if (rc < 0 && errno == EINTR) {
          continue;
        }
        break;
      }
    }
    for (w_b = 0; w_b < r_b;) {
      int rc = write(fd_w, memblock + w_b, r_b - w_b);
      if (rc > 0) {
        w_b += rc;
      } else {
        if (rc < 0 && errno == EINTR) {
          continue;
        }
        break;
      }
    }
    close(fd_r);
    close(fd_w);
    delete[] memblock;

    if (w_b != r_b) {
      cerr << "Read: " << r_b << " Write: " << w_b << endl;
      remove(image_sts.new_path.c_str());
      return FW_STATUS_FAILURE;
    }

    ret = bic_update_fw(slot_id, fw_comp, (char *)image_sts.new_path.c_str(), force);
    remove(image_sts.new_path.c_str());
  }

  snprintf(ver_key, sizeof(ver_key), FRU_STR_COMPONENT_NEW_VER_KEY, fru().c_str(), component().c_str());
  if (ret == 0) {
    if (image_sts.sign == true) {
      if (fby35_common_get_img_ver(image.c_str(), ver, fw_comp) == 0) {
        kv_set(ver_key, ver, 0, 0);
      } else {
        kv_set(ver_key, "Unknown", 0, 0);
      }
    } else {
      kv_set(ver_key, "Unknown", 0, 0);
    }
  } else {
    kv_set(ver_key, "NA", 0, 0);
  }

  return ret;
}

int CpldComponent::update(const string image) {
  return update_cpld(image, false);
}

int CpldComponent::fupdate(const string image) {
  return update_cpld(image, true);
}

int CpldComponent::get_ver_str(string& s) {
  int ret = 0;
  uint8_t rbuf[4] = {0};
  char ver[32] = {0};

  if (fw_comp == FW_CPLD) {
    if (cpld_intf_open(pld_type, INTF_I2C, &attr)) {
      return -1;
    }

    ret = cpld_get_ver((uint32_t *)rbuf);
    cpld_intf_close();
  } else {  // CPLD on BIC Baseboard (Class 2)
    ret = bic_get_fw_ver(slot_id, fw_comp, rbuf);
  }

  if (!ret) {
    snprintf(ver, sizeof(ver), "%02X%02X%02X%02X", rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
    s = string(ver);
  }

  return ret;
}

int CpldComponent::print_version() {
  string ver("");
  string board_name = board;
  char ver_key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int ret = 0;

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
    if (get_ver_str(ver) < 0) {
      throw "Error in getting the version of " + board_name;
    }
    cout << board_name << " CPLD Version: " << ver << endl;
    snprintf(ver_key, sizeof(ver_key), FRU_STR_COMPONENT_NEW_VER_KEY, fru().c_str(), component().c_str());
    ret = kv_get(ver_key, value, NULL, 0);
    if (ret == 0) {
      cout << board_name << " CPLD Version after activation: " << value << endl;
    } else {  // no update before
      cout << board_name << " CPLD Version after activation: " << ver << endl;
    }
  } catch (string& err) {
    printf("%s CPLD Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }

  return FW_STATUS_SUCCESS;
}

int CpldComponent::get_version(json& j) {
  string ver("");
  string board_name = board;

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
    if (get_ver_str(ver) < 0) {
      throw "Error in getting the version of " + board_name;
    }
    j["VERSION"] = ver;
  } catch (string& err) {
    if (err.find("empty") != string::npos) {
      j["VERSION"] = "not_present";
    } else {
      j["VERSION"] = "error_returned";
    }
  }
  return FW_STATUS_SUCCESS;
}
