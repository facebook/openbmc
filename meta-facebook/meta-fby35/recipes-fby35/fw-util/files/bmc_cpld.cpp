#include <cstdio>
#include <syslog.h>
#include <sys/stat.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/kv.h>
#include "bmc_cpld.h"
#include "server.h"
#include <facebook/bic.h>

using namespace std;
#define JBC_FILE_NAME ".jbc"
#define MAX10_RPD_SIZE 0x23000

image_info BmcCpldComponent::check_image(const string& image, bool force) {
  string fru_name = fru();
  size_t bmc_found = fru_name.find("bmc");
  uint8_t slot_id = 0;
  uint8_t board_type_index = 0;
  struct stat file_info;
  uint8_t fw_comp = 0;
  image_info image_sts = {"", false, false};
  uint8_t bmc_location = 0;

  if (fby35_common_get_bmc_location(&bmc_location) < 0) {
    printf("Failed to get BMC location\n");
    return image_sts;
  }

  if (image.find(JBC_FILE_NAME) != string::npos) {
    image_sts.result = true;
    return image_sts;
  }

  if (stat(image.c_str(), &file_info) < 0) {
    cerr << "Cannot check " << image << " file information" << endl;
    return image_sts;
  }

  if (file_info.st_size == MAX10_RPD_SIZE + IMG_POSTFIX_SIZE)
    image_sts.sign = true;

  if (force == false) {
    if (image_sts.sign != true) {
      cerr << "Image " << image << " is not a signed image, please use --force option" << endl;
      return image_sts;
    }

    // Read Board Revision from CPLD
    if (bmc_found != string::npos) {
      if(bmc_location == NIC_BMC){
        if (get_board_rev(0, BOARD_ID_NIC_EXP, &board_type_index) < 0) {
          cerr << "Failed to get board revision ID" << endl;
          return image_sts;
        }
      } else{
        if (get_board_rev(0, BOARD_ID_BB, &board_type_index) < 0) {
          cerr << "Failed to get board revision ID" << endl;
          return image_sts;
        }
      }
      fw_comp = FW_BB_CPLD;
    } else {
      slot_id = fru_name.at(4) - '0';
      if (get_board_rev(slot_id, BOARD_ID_SB, &board_type_index) < 0) {
        cerr << "Failed to get board revision ID" << endl;
        return image_sts;
      }
      fw_comp = FW_CPLD;
    }

    if (fby35_common_is_valid_img(image.c_str(), fw_comp, board_type_index) == false) {
      return image_sts;
    }
    image_sts.result = true;
  } else {
    image_sts.result = true;
  }

  return image_sts;
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

int BmcCpldComponent::print_version()
{
  string ver("");
  string fru_name = fru();
  char ver_key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  size_t slot_found = fru_name.find("slot");
  int ret = 0;

  try {
    // Print CPLD Version
    if (slot_found != string::npos) {
      uint8_t slot_id = fru_name.at(4) - '0';
      Server server(slot_id, fru_name);
      fru_name = "SB";
      server.ready();
    } else {
     transform(fru_name.begin(), fru_name.end(), fru_name.begin(), ::toupper);
    }
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + fru_name;
    }
    cout << fru_name << " CPLD Version: " << ver << endl;
    snprintf(ver_key, sizeof(ver_key), FRU_STR_CPLD_NEW_VER_KEY, fru().c_str());
    ret = kv_get(ver_key, value, NULL, 0);
    if ((ret < 0) && (errno == ENOENT)) { // no update before
      cout << fru_name << " CPLD Version After activation: " << ver << endl;
    } else if (ret == 0) {
      cout << fru_name << " CPLD Version After activation: " << value << endl;
    }
  } catch(string& err) {
    printf("%s CPLD Version: NA (%s)\n", fru_name.c_str(), err.c_str());
  }
  return 0;
}

void BmcCpldComponent::get_version(json& j) {
  string ver("");
  string fru_name = fru();
  size_t slot_found = fru_name.find("slot");
  try {
    if (slot_found != string::npos) {
      uint8_t slot_id = fru_name.at(4) - '0';
      Server server(slot_id, fru_name);
      fru_name = "SB";
      server.ready();
    }

    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + fru_name;
    }
    j["VERSION"] = ver;
  } catch(string& err) {
    if ( err.find("empty") != string::npos ) j["VERSION"] = "not_present";
    else j["VERSION"] = "error_returned";
  }
}

int BmcCpldComponent::update_cpld(const string& image, bool force)
{
  int ret = FW_STATUS_FAILURE;
  char ver_key[MAX_KEY_LEN] = {0};
  char ver[16] = {0};
  uint8_t bmc_location = 0;
  string bmc_location_str;
  image_info image_sts = check_image(image, force);

  if (image_sts.result == false) {
    syslog(LOG_CRIT, "Update %s CPLD Fail. File: %s is not a valid image", fru().c_str(), image.c_str());
    return FW_STATUS_FAILURE;
  }

  if (fby35_common_get_bmc_location(&bmc_location) < 0) {
    printf("Failed to initialize the fw-util\n");
    return FW_STATUS_FAILURE;
  }

  if (bmc_location == NIC_BMC) {
    bmc_location_str = "NIC Expansion";
  } else {
    bmc_location_str = "Baseboard";
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
      } else if (!rc || (rc < 0 && errno != EINTR)) {
        break;
      }
    }
    for (w_b = 0; w_b < r_b;) {
      int rc = write(fd_w, memblock + w_b, r_b - w_b);
      if (rc > 0) {
        w_b += rc;
      } else if (!rc || (rc < 0 && errno != EINTR)) {
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

  snprintf(ver_key, sizeof(ver_key), FRU_STR_CPLD_NEW_VER_KEY, fru().c_str());
  if (fby35_common_get_img_ver(image.c_str(), ver, FW_BB_CPLD) < 0) {
     kv_set(ver_key, "Unknown", 0, 0);
  } else {
    if (ret) {
      kv_set(ver_key, "NA", 0, 0);
    } else if (force && (image_sts.sign == false)) {
      kv_set(ver_key, "Unknown", 0, 0);
    } else {
      kv_set(ver_key, ver, 0, 0);
    }
  }

  syslog(LOG_CRIT, "Updated CPLD on %s. File: %s. Result: %s", bmc_location_str.c_str(), image.c_str(), (ret < 0)?"Fail":"Success");
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
