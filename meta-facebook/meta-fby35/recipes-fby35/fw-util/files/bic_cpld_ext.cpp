#include <cstdio>
#include <openbmc/pal.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include "bic_cpld_ext.h"
#include <sys/stat.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

img_info CpldExtComponent::check_image(string image, bool force) {
const string board_type[] = {"POC1", "POC2", "EVT", "DVT", "PVT", "MP"};
  string flash_image = image;
  uint8_t bmc_location = 0;
  string fru_name = fru();
  string slot_str = "slot";
  string bmc_str = "bmc";
  size_t slot_found = fru_name.find(slot_str);
  size_t bmc_found = fru_name.find(bmc_str);
  uint8_t slot_id = 0;
  uint8_t board_type_index = 0;
  struct stat file_info;

  img_info image_sts = {"", false, false};

  if (stat(image.c_str(), &file_info) < 0) {
    cerr << "Cannot check " << image << " file information" << endl;
    return image_sts;
  }
  if ( fby35_common_get_bmc_location(&bmc_location) < 0 ) {
    cerr << "Cannot open " << image << " for reading" << endl;
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

  uint8_t *memblock = new uint8_t [file_info.st_size];
  size_t r_b = read(fd_r, memblock, file_info.st_size);
  size_t w_b = 0;
  size_t tmp_size = 0;

  tmp_size = r_b;
  w_b = write(fd_w, memblock, tmp_size);
  //check size
  if ( tmp_size != w_b ) {
    cerr << "Cannot create the tmp file - " << image_sts.new_path << endl;
    cerr << "Read: " << tmp_size << " Write: " << endl;
    image_sts.result = false;
  }

  if ( force == false ) {
    // Read Board Revision from CPLD
    if (((bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC)) && (bmc_found != string::npos)) {
      if (get_board_rev(0, BOARD_ID_BB, &board_type_index) < 0) {
        cerr << "Failed to get board revision ID" << endl;
        goto err_exit;
      }
    } else if (slot_found != string::npos) {
      slot_id = fru_name.at(4) - '0';
      if (get_board_rev(slot_id, BOARD_ID_SB, &board_type_index) < 0) {
        cerr << "Failed to get board revision ID" << endl;
        goto err_exit;
      }
    }

    if (fby35_common_is_valid_img(image.c_str(), fw_comp, board_type_index) == false) {
      goto err_exit;
    } else {
      image_sts.result = true;
    }
  } else {
    image_sts.result = true;
  }

err_exit:
  //release the resource
  if (fd_r >= 0)
    close(fd_r);
  if (fd_w >= 0)
    close(fd_w);
  delete[] memblock;

  if (image_sts.result == false) {
     syslog(LOG_CRIT, "Update %s CPLD Fail. File: %s is not a valid image", fru().c_str(), image.c_str());
  }
  return image_sts;
}

int CpldExtComponent::update_cpld(string image, bool force) {
  int ret = 0;
  img_info image_sts = check_image(image, force);
  char ver_key[MAX_KEY_LEN] = {0};
  char ver[16] = {0};
  string image_tmp = image;

  if ( image_sts.result == false ) {
    remove(image_sts.new_path.c_str());
    return FW_STATUS_FAILURE;
  }

  //use the new path
  image = image_sts.new_path;
  ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), force);
  snprintf(ver_key, sizeof(ver_key), FRU_STR_COMPONENT_NEW_VER_KEY, fru().c_str(), component().c_str());
  if (fby35_common_get_img_ver(image_tmp.c_str(), ver, fw_comp) < 0) {
     kv_set(ver_key, "Unknown", 0, 0);
  } else {
    if (ret) {
      kv_set(ver_key, "NA", 0, 0);
    } else if (force) {
      kv_set(ver_key, "Unknown", 0, 0);
    } else {
      kv_set(ver_key, ver, 0, 0);
    }
  }
  remove(image_sts.new_path.c_str());

  return ret;
}

int CpldExtComponent::update(const string image) {
  int ret = 0;
  int intf = FEXP_BIC_INTF;
  string comp = component();

  try {
    server.ready();
    expansion.ready();
    if (comp == "1ou_cpld") {
      intf = FEXP_BIC_INTF;
    } else {
      intf = REXP_BIC_INTF;
    }
    remote_bic_set_gpio(slot_id, EXP_GPIO_RST_USB_HUB, VALUE_HIGH, intf);
    bic_set_gpio(slot_id, GPIO_RST_USB_HUB, VALUE_HIGH);
    ret = update_cpld(image, FORCE_UPDATE_UNSET);

    bic_set_gpio(slot_id, GPIO_RST_USB_HUB, VALUE_LOW);
    remote_bic_set_gpio(slot_id, EXP_GPIO_RST_USB_HUB, VALUE_LOW, intf);
  } catch (string& err) {
    printf("%s\n", err.c_str());
    return FW_STATUS_NOT_SUPPORTED;
  }

  return ret;
}

int CpldExtComponent::fupdate(const string image) {
  int ret = 0;

  try {
    ret = update_cpld(image, FORCE_UPDATE_SET);
  } catch (string& err) {
    printf("%s\n", err.c_str());
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int CpldExtComponent::get_ver_str(string& s) {
  int ret = 0;
  char ver[32] = {0};
  uint8_t rbuf[4] = {0};

  if (fw_comp == FW_CPLD) {
    ret = pal_get_cpld_ver(slot_id, rbuf);
  } else {
    ret = bic_get_fw_ver(slot_id, fw_comp, rbuf);
  }

  if (!ret) {
    snprintf(ver, sizeof(ver), "%02X%02X%02X%02X", rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
    s = string(ver);
  }

  return ret;
}

int CpldExtComponent::print_version() {
  string ver("");
  string board_name = name;
  char ver_key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int ret = 0;

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
    // Print Bridge-IC Version
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + board_name;
    }
    cout << board_name << " CPLD Version: " << ver << endl;
    snprintf(ver_key, sizeof(ver_key), FRU_STR_COMPONENT_NEW_VER_KEY, fru().c_str(), component().c_str());
    ret = kv_get(ver_key, value, NULL, 0);
    if ((ret < 0) && (errno == ENOENT)) { // no update before
      cout << board_name << " CPLD Version After activation: " << ver << endl;
    } else if (ret == 0) {
      cout << board_name << " CPLD Version After activation: " << value << endl;
    }
  } catch(string& err) {
    printf("%s CPLD Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }
  return FW_STATUS_SUCCESS;
}

void CpldExtComponent::get_version(json& j) {
  string ver("");
  try {
    server.ready();
    expansion.ready();
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + name;
    }

    j["VERSION"] = ver;
  } catch(string& err) {
    if ( err.find("empty") != string::npos ) j["VERSION"] = "not_present";
    else j["VERSION"] = "error_returned";
  }
}
#endif
