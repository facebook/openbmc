#include <cstdlib>
#include <fstream>
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
#include <facebook/bic.h>
#include <openbmc/pal.h>
#include <facebook/fbgc_common.h>
#include "bmc_fpga.h"

using namespace std;

bool BmcFpgaComponent::is_valid_image(string image, bool force) {
#define REVISION_ID(x) ((x >> 4) & 0x0f)
#define COMPONENT_ID(x) (x & 0x0f)

  char read_buffer[1] = {0};
  char identity = 0;
  int size = 0;
  bool ret = false;
  bool board_rev_is_invalid = false;
  ifstream fpgaFile;
  uint8_t fpga_id_table[2] = {UIC_FPGA_ID, BS_FPGA_ID};
  uint8_t fw_rev_id = 0xff, board_rev_id = 0xff;

  fpgaFile.open(image, ios::in | ios::binary | ios::ate);

  if (fpgaFile.is_open() == false) {
    cout << "Failed to open image: " << image << endl;
    return false;
  }

  if (force == true) { // skip image checking
    ret = true;
    goto end;
  }

  size = fpgaFile.tellg();
  if (size != (MAX10_RPD_SIZE + IDENTIFY_OFFSET + 1)) {
    cout << "Invalid image size: " << size << endl;
    cout << "If you are updating with old version firmware, please use force update."  <<endl;
    goto end;
  }

  // Compare MD5 of image
  if (check_image_md5(image.c_str(), MAX10_RPD_SIZE, (MAX10_RPD_SIZE + MD5_OFFSET)) < 0) {
    cout << "Image file has corrupted"<< endl;
    goto end;
  }

  // Compare signature of image
  if (check_image_signature(image.c_str(), (MAX10_RPD_SIZE + SIGNATURE_OFFSET)) < 0) {
    cout << "The image is not for Grand Canyon"<< endl;
    goto end;
  }

  // Check ID of image
  fpgaFile.seekg((MAX10_RPD_SIZE + IDENTIFY_OFFSET), ios::beg);
  fpgaFile.read(read_buffer, sizeof(read_buffer));

  if (fpgaFile.gcount() != sizeof(read_buffer)) {
    cout << "Failed to read image file" << image << endl;
    goto end;
  }

  identity = COMPONENT_ID(read_buffer[0]);
  if (identity != fpga_id_table[location]) {
    cout << "Invalid id:" << (int)identity << endl;
    goto end;
  }

  // Check f/w and board stage
  fw_rev_id = REVISION_ID(read_buffer[0]);

  switch (location) {
    case UIC_FPGA_LOCATION:
      if (fbgc_common_get_system_stage(&board_rev_id) < 0) {
        cout << "Failed to get stage of UIC" << endl;
        goto end;
      }

      // Align UIC board ID to system stage
      if ((board_rev_id == UIC_STAGE_DVT) || (board_rev_id == UIC_STAGE_DVT3)) {
        board_rev_id = STAGE_DVT;
      } else if ((board_rev_id == UIC_STAGE_PVT) || (board_rev_id == UIC_STAGE_PVT3)) {
        board_rev_id = STAGE_PVT;
      } else if (board_rev_id == UIC_STAGE_MP) {
        board_rev_id = STAGE_MP;
      }
      break;
    case BS_FPGA_LOCATION:
      if (get_server_board_revision_id(&board_rev_id, sizeof(board_rev_id)) < 0) {
        cout << "Failed to get stage of Barton Springs" << endl;
        goto end;
      }
      if (board_rev_id > 0) {
        board_rev_id--;
      }
      break;
    default:
      goto end;
  }

  if ((fw_rev_id > STAGE_MP) || (board_rev_id > STAGE_MP)) {
    cout << "Wrong revision ID, f/w REV ID: " << (int)fw_rev_id << ", board REV ID: " << (int)board_rev_id << endl;
    goto end;
  }

  // PVT & MP firmware could be used in common
  if (board_rev_id < STAGE_PVT) {
    if (fw_rev_id != board_rev_id) {
      board_rev_is_invalid = true;
    }
  } else {
    if (fw_rev_id < STAGE_PVT) {
      board_rev_is_invalid = true;
    }
  }

  if (board_rev_is_invalid == true) {
    cout << "If you want to update the " << board_stage[fw_rev_id] << " f/w on the " << board_stage[board_rev_id] << " system, please use force update." << endl;
    goto end;
  }

  ret = true;

end:
  fpgaFile.close();

  return ret;
}

int BmcFpgaComponent::get_ver_str(string& s) {
  int ret = 0;
  char ver[MAX_VALUE_LEN] = {0};
  
  ret = pal_get_fpga_ver_cache(bus, addr, ver);
  s = string(ver);

  return ret;
}

int BmcFpgaComponent::print_version()
{
  string ver("");
  string fru_name = fru();

  transform(fru_name.begin(), fru_name.end(), fru_name.begin(), ::toupper);

  if (get_ver_str(ver) < 0) {
    cout << "Error in getting the version of " + fru_name << endl;
  } else {
    cout << fru_name << " FPGA Version: 0x" << ver << endl;
  }

  return 0;
}

int BmcFpgaComponent::get_version(json& j) {
  string ver("");

  if (get_ver_str(ver) < 0) {
    cout << "Error in getting the version of " + fru() << endl;
    j["VERSION"] = "error_returned";
  } else {
    j["VERSION"] = ver;
  }
  return FW_STATUS_SUCCESS;
}

int BmcFpgaComponent::create_update_image(string image, string update_image)
{
  int ret = 0;
  char buffer[MAX10_RPD_SIZE] = {0};

  ifstream inFpgaFile(image, ifstream::binary);
  ofstream outFpgaFile(update_image, ofstream::binary);

  if (inFpgaFile.is_open() == false || outFpgaFile.is_open() == false) {
    goto end;
  }

  inFpgaFile.read(buffer, sizeof(buffer));
  if (inFpgaFile.bad() == true) {
    cout << "Failed to read image: " << image << endl;
    ret = -1;
    goto end;
  }

  outFpgaFile.write(buffer, sizeof(buffer));
  if (outFpgaFile.bad() == true) {
    cout << "Failed to write update image: " << update_image << endl;
    ret = -1;
    goto end;
  }

end:
  if (inFpgaFile.is_open() == true) {
    inFpgaFile.close();
  } else {
    cout << "Failed to open image: " << image << endl;
    ret = -1;
  }
  if (outFpgaFile.is_open() == true) {
    outFpgaFile.close();
  } else {
    cout << "Failed to open update image: " << update_image << endl;
    ret = -1;
  }

  return ret;
}

int BmcFpgaComponent::update_fpga(string image, string update_image)
{
  int ret = 0;
  char key[32] = {0};
  string fru_name = fru();

  transform(fru_name.begin(), fru_name.end(), fru_name.begin(), ::toupper);

  syslog(LOG_CRIT, "Updating %s FPGA. File: %s", fru_name.c_str(), image.c_str());
  if (cpld_intf_open(pld_type, INTF_I2C, &attr) != 0) {
    cout << "Cannot open i2c!" << endl;
    ret = FW_STATUS_FAILURE;
  } else {
    ret = cpld_program((char *)update_image.c_str(), key, false);
    cpld_intf_close();
    if ( ret < 0 ) {
      cout << "Error Occur at updating FPGA FW!" << endl;
    }
  }

  syslog(LOG_CRIT, "Updated %s FPGA. File: %s. Result: %s", fru_name.c_str(), image.c_str(), (ret < 0) ? "Fail" : "Success");
  remove(update_image.c_str());

  return ret;
}

int BmcFpgaComponent::update_wrapper(string image, bool force)
{
  string update_image(image + "-tmp");

  if (is_valid_image(image, force) == false) {
    return FW_STATUS_FAILURE;
  }

  if (create_update_image(image, update_image) < 0) {
    return FW_STATUS_FAILURE;
  }

  return update_fpga(image, update_image);
}

int BmcFpgaComponent::update(string image)
{
  return update_wrapper(image, false);
}

int BmcFpgaComponent::fupdate(string image)
{
  return update_wrapper(image, true);
}
