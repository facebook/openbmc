#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <sys/mman.h>
#include <syslog.h>
#include <openbmc/obmc-i2c.h>
#include "server.h"
#include "bmc_cpld.h"
#include <facebook/fby3_common.h>

using namespace std;

image_info BmcCpldComponent::check_image(string image, bool force) {
const string board_type[] = {"Unknown", "EVT", "DVT", "PVT", "MP"};
  string flash_image = image;
  uint8_t bmc_location = 0;
  string fru_name = fru();
  string slot_str = "slot";
  string bmc_str = "bmc";
  size_t slot_found = fru_name.find(slot_str);
  size_t bmc_found = fru_name.find(bmc_str);
  uint8_t slot_id = 0;
  int ret = -1;
  uint8_t board_rev = 0;
  uint8_t hsc_det = 0;
  
  int board_type_index = 0;
  bool board_rev_is_invalid = false;

  image_info image_sts = {"", false};

  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
    printf("Failed to initialize the fw-util\n");
    exit(EXIT_FAILURE);
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

  uint8_t *memblock = new uint8_t [image_size + 1];//data_size + signed byte
  uint8_t signed_byte = 0;
  size_t r_b = read(fd_r, memblock, image_size + 1);
  size_t w_b = 0;

  //it's an old image
  if ( r_b == image_size ) {
    signed_byte = 0x0;
  } else if ( r_b == (image_size + 1) ) {
    signed_byte = memblock[image_size] & 0xff;
    r_b = r_b - 1;  //only write data to tmp file
  }

  w_b = write(fd_w, memblock, r_b);

  //check size
  if ( r_b != w_b ) {
    cerr << "Cannot create the tmp file - " << image_sts.new_path << endl;
    cerr << "Read: " << r_b << " Write: " << w_b << endl;
    image_sts.result = false;
  }
  
  if ( force == false ) {
    // Read Board Revision from CPLD
    if ( ((bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC)) && (bmc_found != string::npos)) {
      if ( fby3_common_get_bb_board_rev(&board_rev) ) {
        cout << "Failed to get bb board rev" << endl;
        return image_sts;
      }
      // respin board, Need to process BOARD_REV through HSC_DETECT, keeping the original logic work
      if ( fby3_common_get_hsc_bb_detect(&hsc_det) ) {
        cout << "Failed to get bb board rev" << endl;
        return image_sts;
      }
      if ( hsc_det == HSC_DET_ADM1278 ) {
        // old board, BOARD_REV_ID3 is floating.
        board_rev &= 0x7;
      } else {
        // new respin board is the MP stage,
        // rise bit3 to keep "if (board_type_index < CPLD_BOARD_PVT_REV)" work.
        board_rev |= 0x8;
      }
    } else if (slot_found != string::npos) {
      bic_gpio_t gpio = {0};
      
      slot_id = fru_name.at(4) - '0';
      if ( fby3_common_get_sb_board_rev(slot_id, &board_rev) ) {
        cout << "Failed to get sb board rev" << endl;
        return image_sts;
      }
      // respin board, Need to process BOARD_REV through HSC_DETECT, keeping the original logic work
      ret = bic_get_gpio(slot_id, &gpio, NONE_INTF);
      if ( ret < 0 ) {
        printf("%s() bic_get_gpio returns %d\n", __func__, ret);
        return image_sts;
      }
      hsc_det |= ((BIT_VALUE(gpio, 77)) << 0); // 77    HSC_DETECT0
      hsc_det |= ((BIT_VALUE(gpio, 78)) << 1); // 78    HSC_DETECT1
      hsc_det |= ((BIT_VALUE(gpio, 79)) << 2); // 79    HSC_DETECT2
      if ( hsc_det == HSC_DET_ADM1278 ) {
        // old board, BOARD_REV_ID3 is floating.
        board_rev &= 0x7;
      } else {
        // new respin board is the MP stage,
        // rise bit3 to keep "if (board_type_index < CPLD_BOARD_PVT_REV)" work.
        board_rev |= 0x8;
      }
    }

    board_type_index = board_rev - 1;
    if (board_type_index < 0) {
      board_type_index = 0;
    }

    // PVT & MP firmware could be used in common
    if (board_type_index < CPLD_BOARD_PVT_REV) {
      if (REVISION_ID(signed_byte) != board_type_index) {
        board_rev_is_invalid = true;
      }
    } else {
      if (REVISION_ID(signed_byte) < CPLD_BOARD_PVT_REV) {
        board_rev_is_invalid = true;
      }
    }

    //CPLD is located on class 2(NIC_EXP)
    if ( bmc_location == NIC_BMC ) {
      if ( (COMPONENT_ID(signed_byte) == NICEXP) && (bmc_found != string::npos) ) {
        image_sts.result = true;
      } else if ( !board_rev_is_invalid && (COMPONENT_ID(signed_byte) == BICDL) && (slot_found != string::npos)) {
        image_sts.result = true;
      } else {
        if (board_rev_is_invalid && (slot_found != string::npos)) {
             cout << "To prevent this update on " << fru_name <<", please use the f/w of "
             << board_type[board_type_index].c_str() <<" on the " << board_type[board_type_index].c_str() <<" system." << endl;
             cout << "To force the update, please use the --force option." << endl;
        }

        cout << "image is not a valid CPLD image for " << fru_name << endl;
      }
    } else if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
      if ( !board_rev_is_invalid && (COMPONENT_ID(signed_byte) == BICBB) && (bmc_found != string::npos) ) {
        image_sts.result = true;
      } else if ( !board_rev_is_invalid && (COMPONENT_ID(signed_byte) == BICDL) && (slot_found != string::npos) ) {
        image_sts.result = true;
      } else {
        if (board_rev_is_invalid) {
             cout << "To prevent this update on " << fru_name <<", please use the f/w of "
             << board_type[board_type_index].c_str() <<" on the " << board_type[board_type_index].c_str() <<" system." << endl;
             cout << "To force the update, please use the --force option." << endl;
        }

        cout << "image is not a valid CPLD image for " << fru_name << endl;
      }
    }
  }

  //release the resource
  close(fd_r);
  close(fd_w);
  delete[] memblock;

  return image_sts;
}

int BmcCpldComponent::get_ver_str(string& s) {
  int ret = 0;
  char ver[32] = {0};
  uint32_t ver_reg = ON_CHIP_FLASH_USER_VER;
  uint8_t tbuf[4] = {0x00};
  uint8_t rbuf[4] = {0x00};
  uint8_t tlen = 4;
  uint8_t rlen = 4;
  int i2cfd = 0;

  memcpy(tbuf, (uint8_t *)&ver_reg, tlen);
  reverse(tbuf, tbuf + 4);
  string i2cdev = "/dev/i2c-" + to_string(bus);

  if ((i2cfd = open(i2cdev.c_str(), O_RDWR)) < 0) {
    printf("Failed to open %s\n", i2cdev.c_str());
    return -1;
  }

  if (ioctl(i2cfd, I2C_SLAVE, addr) < 0) {
    printf("Failed to talk to slave@0x%02X\n", addr);
  } else {
    ret = i2c_rdwr_msg_transfer(i2cfd, addr << 1, tbuf, tlen, rbuf, rlen);
    snprintf(ver, sizeof(ver), "%02X%02X%02X%02X", rbuf[3], rbuf[2], rbuf[1], rbuf[0]);
  }

  if ( i2cfd > 0 ) close(i2cfd);
  s = string(ver);
  return ret;
}

int BmcCpldComponent::print_version()
{
  string ver("");
  string fru_name = fru();
  size_t slot_found = fru_name.find("slot");
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

int BmcCpldComponent::update_cpld(string image) 
{
  int ret = 0;
  char key[32] = {0};
  uint8_t bmc_location = 0;
  string bmc_location_str;
  string image_tmp;
  size_t pos = image.find("-tmp"); 
  image_tmp = image.substr(0, pos);
  string fru_name = fru();
  string slot_str = "slot";
  size_t slot_found = fru_name.find(slot_str);

  if ( fby3_common_get_bmc_location(&bmc_location) < 0 ) {
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
    //Check Server Board is Ready
    try {
      uint8_t slot_id = fru_name.at(4) - '0';
      Server server(slot_id, fru_name);
      server.ready();
    } catch(string& err) {
      printf("%s\n", err.c_str());
      return FW_STATUS_NOT_SUPPORTED;
    }
  }

  syslog(LOG_CRIT, "Updating CPLD on %s. File: %s", bmc_location_str.c_str(), image_tmp.c_str());
  if (cpld_intf_open(pld_type, INTF_I2C, &attr)) {
    printf("Cannot open i2c!\n");
    ret = FW_STATUS_FAILURE;
  } else {
    //TODO: need to update 2 CFMs
    ret = cpld_program((char *)image.c_str(), key, false);
    cpld_intf_close();
    if ( ret < 0 ) {
      printf("Error Occur at updating CPLD FW!\n");
    }
  }

  syslog(LOG_CRIT, "Updated CPLD on %s. File: %s. Result: %s", bmc_location_str.c_str(), image_tmp.c_str(), (ret < 0)?"Fail":"Success"); 
  return ret;
}

void BmcCpldComponent::check_module() {
  string ver("");

  if (get_ver_str(ver) < 0 || ver.length() < VER_STR_LEN) {
    cout << "Fail to get current CPLD module: use default settings" << endl;
    return;
  }

  if (ver[4] == MAX10M08_VER_CHAR) {
    attr.start_addr = MAX10M08_CFM1_START_ADDR;
    attr.end_addr = MAX10M08_CFM1_END_ADDR;
    image_size = MAX10M08_CFM1_END_ADDR - MAX10M08_CFM1_START_ADDR + 1;
  }

  return;
}

int BmcCpldComponent::update(string image)
{
  int ret = 0;
  image_info image_sts;

  check_module();
  image_sts = check_image(image, false);

  if ( image_sts.result == false ) {
    remove(image_sts.new_path.c_str());
    return FW_STATUS_FAILURE;
  } 

  //use the new path
  image = image_sts.new_path;

  ret = update_cpld(image);

  //remove the tmp file
  remove(image.c_str());
  return ret;
}

int BmcCpldComponent::fupdate(string image) 
{
  int ret = 0;
  image_info image_sts;

  check_module();
  image_sts = check_image(image, true);

  image = image_sts.new_path;

  ret = update_cpld(image);

  //remove the tmp file
  remove(image.c_str());
  return ret;
}
