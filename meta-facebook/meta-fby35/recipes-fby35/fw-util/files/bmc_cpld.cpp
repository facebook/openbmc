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
#define CPLD_NEW_VER_KEY "%s_cpld_new_ver"

image_info BmcCpldComponent::check_image(string image, bool force) {
  string flash_image = image;
  uint8_t bmc_location = 0;
  string fru_name = fru();
  string slot_str = "slot";
  string bmc_str = "bmc";
  size_t slot_found = fru_name.find(slot_str);
  size_t bmc_found = fru_name.find(bmc_str);
  size_t img_size = 0;
  size_t w_b = 0;
  uint8_t slot_id = 0;
  uint8_t board_type_index = 0;
  struct stat file_info;
  uint8_t fw_comp = 0;

  image_info image_sts = {"", false};

  if (image.find(JBC_FILE_NAME) != string::npos) {
    image_sts.new_path = image;
    image_sts.result = true;
    return image_sts;
  }
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
  if (read(fd_r, memblock, file_info.st_size) != file_info.st_size) {
    cerr << "Cannot read file " << image_sts.new_path << endl;
    goto err_exit;
  }

  w_b = write(fd_w, memblock, MAX10_RPD_SIZE);

  //check size
  if ( MAX10_RPD_SIZE != w_b ) {
    cerr << "Cannot create the tmp file - " << image_sts.new_path << endl;
    cerr << "Read: " << MAX10_RPD_SIZE << " Write: " << w_b << endl;
    image_sts.result = false;
  }
  if (file_info.st_size == MAX10_RPD_SIZE + IMG_POSTFIX_SIZE)
    image_sts.sign = true;

  if ( force == false ) {
    if (file_info.st_size != MAX10_RPD_SIZE + IMG_POSTFIX_SIZE) {
      cerr << "Image " << image << " is not a signed image, please use --force option" << endl;
      goto err_exit;
    }
    // Read Board Revision from CPLD
    if ( ((bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC)) && (bmc_found != string::npos)) {
      if (get_board_rev(0, BOARD_ID_BB, &board_type_index) < 0) {
        cerr << "Failed to get board revision ID" << endl;
        goto err_exit;
      }
      fw_comp = FW_BB_CPLD;
    } else if (slot_found != string::npos) {
      slot_id = fru_name.at(4) - '0';
      if (get_board_rev(slot_id, BOARD_ID_SB, &board_type_index) < 0) {
        cerr << "Failed to get board revision ID" << endl;
        goto err_exit;
      }
      fw_comp = FW_CPLD;
    }
    img_size = file_info.st_size - IMG_POSTFIX_SIZE;
    if (fby35_common_is_valid_img(image.c_str(), (FW_IMG_INFO*)(memblock + img_size), fw_comp, board_type_index) == false) {
      goto err_exit;
    } else {
      image_sts.result = true;
    }   
  }

err_exit:
  //release the resource
  if (fd_r >= 0)
    close(fd_r);
  if (fd_w >= 0)
    close(fd_w);
  delete[] memblock;

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
    snprintf(ver_key, sizeof(ver_key), CPLD_NEW_VER_KEY, fru().c_str());
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

int BmcCpldComponent::update_cpld(string image, bool force, bool sign) 
{
  int ret = 0;
  char key[32] = {0};
  char ver_key[MAX_KEY_LEN] = {0};
  char ver[16] = {0};
  uint8_t bmc_location = 0;
  string bmc_location_str;
  string image_tmp;
  size_t pos = image.find("-tmp"); 
  image_tmp = image.substr(0, pos);
  string fru_name = fru();
  string slot_str = "slot";
  size_t slot_found = fru_name.find(slot_str);

  if ( fby35_common_get_bmc_location(&bmc_location) < 0 ) {
    printf("Failed to initialize the fw-util\n");
    return FW_STATUS_FAILURE;
  }

  if ( bmc_location == NIC_BMC ) {
    bmc_location_str = "NIC Expansion";
  } else {
    bmc_location_str = "baseboard";
  }
  if (slot_found != string::npos) {
    bmc_location_str = fru_name+ " " + "SB";
  }

  syslog(LOG_CRIT, "Updating CPLD on %s. File: %s", bmc_location_str.c_str(), image_tmp.c_str());

  if (image.find(JBC_FILE_NAME) != string::npos) {
    string cmd("jbi -r -aPROGRAM -dDO_REAL_TIME_ISP=1 -W ");
    cmd += image;
    ret = system(cmd.c_str());
  } else {
    if (cpld_intf_open(pld_type, INTF_I2C, &attr)) {
      printf("Cannot open i2c!\n");
      ret = FW_STATUS_FAILURE;
    } else {
      //TODO: need to update 2 CFMs
      ret = cpld_program((char *)image.c_str(), key, false);
      cpld_intf_close(INTF_I2C);
      if ( ret < 0 ) {
        printf("Error Occur at updating CPLD FW!\n");
      }
    }
  }

  snprintf(ver_key, sizeof(ver_key), CPLD_NEW_VER_KEY, fru().c_str());
  if (fby35_common_get_img_ver(image_tmp.c_str(), ver, FW_BB_CPLD) < 0) {
     kv_set(ver_key, "Unknown", 0, 0);
  } else {
    if (ret) {
      kv_set(ver_key, "NA", 0, 0);
    } else if (force && (sign == false)) {
      kv_set(ver_key, "Unknown", 0, 0);
    } else {
      kv_set(ver_key, ver, 0, 0);
    }
  }

  syslog(LOG_CRIT, "Updated CPLD on %s. File: %s. Result: %s", bmc_location_str.c_str(), image_tmp.c_str(), (ret < 0)?"Fail":"Success"); 
  return ret;
}

int BmcCpldComponent::update(string image)
{
  int ret = 0;
  image_info image_sts = check_image(image, false);

  if ( image_sts.result == false ) {
    remove(image_sts.new_path.c_str());
    return FW_STATUS_FAILURE;
  } 

  //use the new path
  image = image_sts.new_path;

  ret = update_cpld(image, FORCE_UPDATE_UNSET, image_sts.sign);
  //remove the tmp file, but jbc has no temp file now
  if (image.find(JBC_FILE_NAME) == string::npos) {
    remove(image.c_str());
  }
  
  return ret;
}

int BmcCpldComponent::fupdate(string image) 
{
  int ret = 0;
  image_info image_sts = check_image(image, true);

  image = image_sts.new_path;

  ret = update_cpld(image, FORCE_UPDATE_SET, image_sts.sign);

  //remove the tmp file, but jbc has no temp file now
  if (image.find(JBC_FILE_NAME) == string::npos) {
    remove(image.c_str());
  }

  return ret;
}

