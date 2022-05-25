#include <cstdio>
#include <memory>
#include <iostream>
#include <fstream>
#include <syslog.h>
#include <sys/stat.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/libgpio.h>
#include <openbmc/kv.h>
#include "bmc_cpld.h"

using namespace std;
#define JBC_FILE_NAME ".jbc"
#define CPLD_NEW_VER_KEY "%s_cpld_new_ver"

image_info BmcCpldComponent::check_image(string image, bool force) {
  string flash_image = image;
  size_t img_size = 0;
  size_t w_b = 0;
  uint8_t board_type_index = 0;
  long int MAX10_RPD_SIZE = 0;
  struct stat file_info;

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

  if (get_cpld_attr(attr) < 0) {
    std::cerr << "Cannot get CPLD attribute!" << std::endl;
  }

  //create a new tmp file
  image_sts.new_path = image + "-tmp";

  //open the binary
  ifstream fd_r(image);
  if (!fd_r.is_open()) {
    cerr << "Cannot open " << image << " for reading" << endl;
    return image_sts;
  }

  // create a tmp file for writing.
  ofstream fd_w(image_sts.new_path);
  if (!fd_w.is_open()) {
    cerr << "Cannot write to " << image_sts.new_path << endl;
    fd_r.close();
    return image_sts;
  }

  auto memblock = make_unique<char[]>(file_info.st_size);
  if (fd_r.read(memblock.get(), file_info.st_size).gcount() != file_info.st_size) {
    cerr << "Cannot read file " << image_sts.new_path << endl;
    goto err_exit;
  }

  MAX10_RPD_SIZE = attr.end_addr - attr.start_addr + 1;
  fd_w.write(memblock.get(), MAX10_RPD_SIZE);
  w_b = fd_w.tellp();

  //check size
  if ((unsigned)MAX10_RPD_SIZE != w_b) {
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
    if (netlakemtp_common_get_board_rev(&board_type_index) < 0) {
      cerr << "Failed to get board revision ID" << endl;
      goto err_exit;
    }

    img_size = file_info.st_size - IMG_POSTFIX_SIZE;
    if (netlakemtp_common_is_valid_img(image.c_str(), (FW_IMG_INFO*)(memblock.get() + img_size), board_type_index) == false) {
      goto err_exit;
    } else {
      image_sts.result = true;
    }
  }

err_exit:
  //release the resource
  if (fd_r.is_open())
    fd_r.close();
  if (fd_w.is_open())
    fd_w.close();

  return image_sts;
}

int BmcCpldComponent::get_ver_str(string& s) {
  int ret = 0;
  char rbuf[CPLD_VER_BYTE * 2] = {0};

  ret = pal_get_cpld_ver(FRU_SERVER, rbuf);

  if (!ret) {
    s = string(rbuf);
  }

  return ret;
}

int BmcCpldComponent::print_version()
{
  string ver("");
  string fru_name = fru();
  char ver_key[MAX_KEY_LEN] = {0};
  char value[MAX_VALUE_LEN] = {0};
  int ret = 0;

  try {
    // Print CPLD Version
    transform(fru_name.begin(), fru_name.end(), fru_name.begin(), ::toupper);

    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + fru_name;
    } else {
      cout << fru_name << " CPLD Version: " << ver << endl;
      snprintf(ver_key, sizeof(ver_key), CPLD_NEW_VER_KEY, fru().c_str());
      ret = kv_get(ver_key, value, NULL, 0);
      if ((ret < 0) && (errno == ENOENT)) { // no update before
        cout << fru_name << " CPLD Version After activation: " << ver << endl;
      } else if (ret == 0) {
        cout << fru_name << " CPLD Version After activation: " << value << endl;
      } else {
        throw "Error in getting the version of " + fru_name;
      }
    }
  } catch(string& err) {
    printf("%s CPLD Version: NA (%s)\n", fru_name.c_str(), err.c_str());
  }
  return 0;
}

void BmcCpldComponent::get_version(json& ver_json) {
  string ver("");
  string fru_name = fru();
  try {
    if ( get_ver_str(ver) < 0 ) {
      throw "Error in getting the version of " + fru_name;
    }
    ver_json["VERSION"] = ver;
  } catch(string& err) {
    if ( err.find("empty") != string::npos ) {
      ver_json["VERSION"] = "not_present";
    } else {
      ver_json["VERSION"] = "error_returned";
    }
  }
}

int BmcCpldComponent::get_cpld_attr(altera_max10_attr_t& attr)
{
  attr.bus_id = bus;
  attr.slv_addr = addr;
  attr.csr_base = ON_CHIP_FLASH_IP_CSR_BASE;
  attr.data_base = ON_CHIP_FLASH_IP_DATA_REG;
  attr.boot_base = DUAL_BOOT_IP_BASE;

  switch(pld_type) {
    case MAX10_10M04:
      attr.img_type = CFM_IMAGE_1_M04;
      attr.start_addr = M04_CFM1_START_ADDR;
      attr.end_addr = M04_CFM1_END_ADDR;
      break;
    case MAX10_10M08:
      attr.img_type = CFM_IMAGE_1;
      attr.start_addr = M08_CFM1_START_ADDR;
      attr.end_addr = M08_CFM1_END_ADDR;
      break;
    case MAX10_10M25:
      attr.img_type = CFM_IMAGE_1;
      attr.start_addr = M25_CFM1_START_ADDR;
      attr.end_addr = M25_CFM1_END_ADDR;
      break;
    default:
      cerr << "Unknown CPLD attr" << endl;
      return -1;
      break;
  }

  return 0;
}

int BmcCpldComponent::update_cpld(string image, bool force, bool sign)
{
  int ret = 0;
  char key[32] = {0};
  char ver_key[MAX_KEY_LEN] = {0};
  char ver[16] = {0};
  string image_tmp;
  size_t pos = image.find("-tmp");
  image_tmp = image.substr(0, pos);

  syslog(LOG_CRIT, "Updating CPLD on baseboard. File: %s",  image_tmp.c_str());

  if (image.find(JBC_FILE_NAME) != string::npos) {
    gpio_desc_t *jtag_iso_r_en = gpio_open_by_shadow("JTAG_ISO_R_EN");
    if (jtag_iso_r_en == NULL) {
      syslog(LOG_ERR, "%s() Fail to open gpio for enabling jtag", __func__);
    } else {
      if (gpio_set_value(jtag_iso_r_en, GPIO_VALUE_HIGH)) {
        syslog(LOG_ERR, "%s() Fail to set gpio for enabling jtag", __func__);
      } else {
        string cmd("jbi -r -aPROGRAM -dDO_REAL_TIME_ISP=1 -W ");
        cmd += image;
        ret = system(cmd.c_str());

        if (gpio_set_value(jtag_iso_r_en, GPIO_VALUE_LOW)) {
          syslog(LOG_ERR, "%s() Fail to set gpio for disabling jtag", __func__);
        }
      }
      gpio_close(jtag_iso_r_en);
    }
  } else {
    if (cpld_intf_open(pld_type, INTF_I2C, &attr)) {
      std::cerr << "Cannot open i2c!" << std::endl;
      ret = FW_STATUS_FAILURE;
    } else {
      //TODO: need to update 2 CFMs
      ret = cpld_program((char *)image.c_str(), key, false);
      cpld_intf_close();
      if ( ret < 0 ) {
        std::cerr << "Error Occur at updating CPLD FW!" << std::endl;
      }
    }
  }

  snprintf(ver_key, sizeof(ver_key), CPLD_NEW_VER_KEY, fru().c_str());
  if (netlakemtp_common_get_img_ver(image_tmp.c_str(), ver) < 0) {
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

  syslog(LOG_CRIT, "Updated CPLD on baseboard. File: %s. Result: %s", image_tmp.c_str(), (ret < 0)?"Fail":"Success");
  return ret;
}

int BmcCpldComponent::update(string image)
{
  image_info image_sts = check_image(image, false);

  if ( image_sts.result == false ) {
    remove(image_sts.new_path.c_str());
    return FW_STATUS_FAILURE;
  }

  return update_process(image, image_sts, false);
}

int BmcCpldComponent::fupdate(string image)
{
  image_info image_sts = check_image(image, true);
  return update_process(image, image_sts, true);
}

int BmcCpldComponent::update_process(string image, image_info image_sts, bool force)
{
  int ret = 0;

  image = image_sts.new_path;

  ret = update_cpld(image, force, image_sts.sign);

  //remove the tmp file, but jbc has no temp file now
  if (image.find(JBC_FILE_NAME) == string::npos) {
    remove(image.c_str());
  }

  return ret;
}
