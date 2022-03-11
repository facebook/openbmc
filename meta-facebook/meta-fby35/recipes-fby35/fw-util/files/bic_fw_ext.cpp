#include <cstdio>
#include <sys/stat.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#include "bic_fw_ext.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

#define BIC_IMG_SIZE 0x50000

image_info BicFwExtComponent::check_image(string image, bool force) {
const string board_type[] = {"POC1", "POC2", "EVT", "DVT", "PVT", "MP"};
  string flash_image = image;
  uint8_t bmc_location = 0;
  string fru_name = fru();
  string slot_str = "slot";
  string bmc_str = "bmc";
  uint8_t slot_id = 0;
  uint8_t board_type_index = 0;
  struct stat file_info;

  image_info image_sts = {"", false, false};

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
  if ((r_b == BIC_IMG_SIZE) || (r_b == BIC_IMG_SIZE + IMG_POSTFIX_SIZE)) {
    tmp_size = BIC_IMG_SIZE;
  } else {
    tmp_size = r_b;
  }
  w_b = write(fd_w, memblock, tmp_size);
  //check size
  if ( tmp_size != w_b ) {
    cerr << "Cannot create the tmp file - " << image_sts.new_path << endl;
    cerr << "Read: " << tmp_size << " Write: " << w_b << endl;
    goto err_exit;
  }

  if ( force == false ) {
    if (file_info.st_size != BIC_IMG_SIZE + IMG_POSTFIX_SIZE) {
      cerr << "Image " << image << " is not a signed image, please use --force option" << endl;
      goto err_exit;
    }
    // Read Board Revision from CPLD
    if (fw_comp == FW_BB_BIC) {
      if (get_board_rev(0, BOARD_ID_BB, &board_type_index) < 0) {
        cerr << "Failed to get board revision ID" << endl;
        goto err_exit;
      }
    } else {
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
     syslog(LOG_CRIT, "Update %s %s Fail. File: %s is not a valid image",
            fru().c_str(), get_component_name(fw_comp), image.c_str());
  }
  return image_sts;
}

int BicFwExtComponent::update_internal(const string& image, bool force) {
  int ret = 0;
  image_info image_sts = check_image(image, force);

  if ( image_sts.result == false ) {
    remove(image_sts.new_path.c_str());
    return FW_STATUS_FAILURE;
  }

  try {
    if (fw_comp != FW_BIC_RCVY) {
      server.ready();
      expansion.ready();
    }
    ret = bic_update_fw(slot_id, fw_comp, (char *)image_sts.new_path.c_str(), (force)?FORCE_UPDATE_SET:FORCE_UPDATE_UNSET);
    remove(image_sts.new_path.c_str());

    if (ret != BIC_FW_UPDATE_SUCCESS) {
      switch(ret) {
      case BIC_FW_UPDATE_FAILED:
        cerr << this->alias_component() << ": update process failed" << endl;
        break;
      case BIC_FW_UPDATE_NOT_SUPP_IN_CURR_STATE:
        cerr << this->alias_component() << ": firmware update not supported in current state." << endl;
        break;
      default:
        cerr << this->alias_component() << ": unknow error (ret: " << ret << ")" << endl;
        break;
      }
      return FW_STATUS_FAILURE;
    }
  } catch (string& err) {
    printf("%s\n", err.c_str());
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BicFwExtComponent::update(const string image) {
  return update_internal(image, false);
}

int BicFwExtComponent::fupdate(const string image) {
  return update_internal(image, true);
}

int BicFwExtComponent::get_ver_str(string& s) {
  uint8_t ver[32] = {0};
  char ver_str[32] = {0};
  uint8_t* plat_name = NULL;
  int ret = 0;
  // Get Bridge-IC Version
  ret = bic_get_fw_ver(slot_id, fw_comp, ver);
  if (strlen((char*)ver) == 2) { // old version format
    snprintf(ver_str, sizeof(ver_str), "v%x.%02x", ver[0], ver[1]);
  } else if (strlen((char*)ver) >= 4){ // new version format
    plat_name = ver + 4;
    snprintf(ver_str, sizeof(ver_str), "oby35-%s-v%02x%02x.%02x.%02x", (char*)plat_name, ver[0], ver[1], ver[2], ver[3]);
  } else {
    snprintf(ver_str, sizeof(ver_str), "Format not supported");
  }
  s = string(ver_str);
  return ret;
}

int BicFwExtComponent::print_version() {
  string ver("");
  string board_name = name;
  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);

  if (fw_comp == FW_BIC_RCVY) {
    return FW_STATUS_NOT_SUPPORTED;
  }

  try {
    server.ready();
    expansion.ready();
    // Print Bridge-IC Version
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + board_name;
    }
    cout << board_name << " Bridge-IC Version: " << ver << endl;
  } catch(string& err) {
    printf("%s Bridge-IC Version: NA (%s)\n", board_name.c_str(), err.c_str());
  }
  return FW_STATUS_SUCCESS;
}

void BicFwExtComponent::get_version(json& j) {
  string ver("");
  try {
    server.ready();
    expansion.ready();
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + component();
    } else {
      j["VERSION"] = ver;
    }
  } catch(string& err) {
    if ( err.find("empty") != string::npos ) j["VERSION"] = "not_present";
    else j["VERSION"] = "error_returned";
  }
}

#endif
