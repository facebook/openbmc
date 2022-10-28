#include <cstdio>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/kv.h>
#include <openbmc/pal.h>
#include <facebook/bic.h>
#include "bmc_cpld.h"

using std::string;
#define JBC_FILE_NAME ".jbc"
#define MAX10_RPD_SIZE 0x23000

image_info BmcCpldComponent::check_image(const string& image, bool force) {
  uint8_t bmc_location = 0;
  uint8_t board_id = 0, board_rev = 0;
  image_info image_sts = {"", false, false};

  if (image.find(JBC_FILE_NAME) != string::npos) {
    image_sts.result = true;
    return image_sts;
  }

  if (force == true) {
    image_sts.result = true;
  }

  if (fby35_common_get_bmc_location(&bmc_location) < 0) {
    printf("Failed to get BMC location\n");
    return image_sts;
  }

  board_id = (bmc_location == NIC_BMC) ? BOARD_ID_NIC_EXP : BOARD_ID_BB;
  if (get_board_rev(0, board_id, &board_rev) < 0) {
    cerr << "Failed to get board revision ID" << endl;
    return image_sts;
  }

  if (fby35_common_is_valid_img(image.c_str(), FW_BB_CPLD, board_id, board_rev) == true) {
    image_sts.result = true;
    image_sts.sign = true;
  }

  return image_sts;
}

int BmcCpldComponent::update_cpld(const string& image, bool force) {
  int ret = FW_STATUS_FAILURE;
  char ver_key[MAX_KEY_LEN] = {0};
  char ver[16] = {0};
  uint8_t bmc_location = 0;
  string bmc_location_str;
  image_info image_sts = check_image(image, force);

  if (fby35_common_get_bmc_location(&bmc_location) < 0) {
    printf("Failed to initialize the fw-util\n");
    return FW_STATUS_FAILURE;
  }

  if (bmc_location == NIC_BMC) {
    bmc_location_str = "NIC Expansion";
  } else {
    bmc_location_str = "Baseboard";
  }

  if (image_sts.result == false) {
    syslog(LOG_CRIT, "Update CPLD on %s Fail. File: %s is not a valid image",
           bmc_location_str.c_str(), image.c_str());
    return FW_STATUS_FAILURE;
  }

  syslog(LOG_CRIT, "Updating CPLD on %s. File: %s", bmc_location_str.c_str(), image.c_str());

  if (image.find(JBC_FILE_NAME) != string::npos) {
    string cmd("jbi -r -aPROGRAM -dDO_REAL_TIME_ISP=1 -W ");
    cmd += image;
    ret = system(cmd.c_str());
    if (ret) {
      ret = FW_STATUS_FAILURE;
    }
  } else {
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

    if (cpld_intf_open(pld_type, INTF_I2C, &attr) == 0) {
      ret = cpld_program((char *)image_sts.new_path.c_str(), NULL, false);
      cpld_intf_close();
      if (ret < 0) {
        printf("Error Occur at updating CPLD FW!\n");
        ret = FW_STATUS_FAILURE;
      }
    } else {
      printf("Cannot open i2c!\n");
      ret = FW_STATUS_FAILURE;
    }
    remove(image_sts.new_path.c_str());
  }

  syslog(LOG_CRIT, "Updated CPLD on %s. File: %s. Result: %s", bmc_location_str.c_str(),
         image.c_str(), (ret)?"Fail":"Success");

  snprintf(ver_key, sizeof(ver_key), FRU_STR_CPLD_NEW_VER_KEY, fru().c_str());
  if (ret == 0) {
    if (image_sts.sign == true) {
      if (fby35_common_get_img_ver(image.c_str(), ver, FW_BB_CPLD) == 0) {
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

int BmcCpldComponent::update(const string image)
{
  return update_cpld(image, false);
}

int BmcCpldComponent::fupdate(const string image)
{
  return update_cpld(image, true);
}

int BmcCpldComponent::get_ver_str(string& s) {
  int ret = 0;
  char ver[32] = {0};
  uint8_t rbuf[4] = {0};

  ret = pal_get_cpld_ver(FRU_BMC, rbuf);
  if (!ret) {
    snprintf(ver, sizeof(ver), "%02X%02X%02X%02X", rbuf[3], rbuf[2], rbuf[1], rbuf[0]);
    s = string(ver);
  }

  return ret;
}

int BmcCpldComponent::print_version() {
  string ver("");
  string board_name = fru();
  char ver_key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int ret = 0;

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    if (get_ver_str(ver) < 0) {
      throw "Error in getting the version of " + board_name;
    }
    cout << board_name << " CPLD Version: " << ver << endl;
    snprintf(ver_key, sizeof(ver_key), FRU_STR_CPLD_NEW_VER_KEY, fru().c_str());
    ret = kv_get(ver_key, value, NULL, 0);
    if (ret == 0) {
      cout << board_name << " CPLD Version after activation: " << value << endl;
    } else {
      cout << board_name << " CPLD Version after activation: " << ver << endl;
    }
  } catch (string& err) {
    printf("%s CPLD Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }

  return FW_STATUS_SUCCESS;
}

int BmcCpldComponent::get_version(json& j) {
  string ver("");
  string board_name = fru();

  try {
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
