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
#include "bmc_fpga.h"

using namespace std;

bool BmcFpgaComponent::is_valid_image(string image, bool force) {
  char read_buffer[1] = {0};
  char identity = 0;
  int size = 0;
  bool ret = false;
  ifstream fpgaFile;

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
  if (size != MAX10_RPD_SIZE + 1) {
    cout << "Invalid image size: " << size << endl;
    goto end;
  }

  fpgaFile.seekg(MAX10_RPD_SIZE, ios::beg);
  fpgaFile.read(read_buffer, sizeof(read_buffer));

  if (fpgaFile.gcount() != sizeof(read_buffer)) {
    cout << "Failed to read image file" << image << endl;
    goto end;
  }

  identity = read_buffer[0] & 0xf;
  if (identity != UIC_FPGA_ID) {
    cout << "Invalid id:" << (int)identity << endl;
    goto end;
  }

  ret = true;

end:
  fpgaFile.close();

  return ret;
}

int BmcFpgaComponent::get_ver_str(string& s) {
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

  if ((i2cfd = i2c_cdev_slave_open(bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM)) < 0) {
    cout << "Failed to open i2c bus: " << bus << endl;
    return -1;
  }

  if (ioctl(i2cfd, I2C_SLAVE, addr) < 0) {
    cout << "Failed to talk to slave@0x" << hex << addr << endl;
    ret = -1;
  } else {
    ret = i2c_rdwr_msg_transfer(i2cfd, addr << 1, tbuf, tlen, rbuf, rlen);
    snprintf(ver, sizeof(ver), "%02X%02X%02X%02X", rbuf[3], rbuf[2], rbuf[1], rbuf[0]);
  }

  close(i2cfd);
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
    cout << fru_name << " FPGA Version: " << ver << endl;
  }

  return 0;
}

void BmcFpgaComponent::get_version(json& j) {
  string ver("");

  if (get_ver_str(ver) < 0) {
    cout << "Error in getting the version of " + fru() << endl;
    j["VERSION"] = "error_returned";
  } else {
    j["VERSION"] = ver;
  }
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

int BmcFpgaComponent::update_fpga(string image)
{
  int ret = 0;
  char key[32] = {0};

  syslog(LOG_CRIT, "Updating UIC FPGA. File: %s", image.c_str());
  if (cpld_intf_open(pld_type, INTF_I2C, &attr) != 0) {
    cout << "Cannot open i2c!" << endl;
    ret = FW_STATUS_FAILURE;
  } else {
    ret = cpld_program((char *)image.c_str(), key, false);
    cpld_intf_close(INTF_I2C);
    if ( ret < 0 ) {
      cout << "Error Occur at updating FPGA FW!" << endl;
    }
  }

  syslog(LOG_CRIT, "Updated UIC FPGA. File: %s. Result: %s", image.c_str(), (ret < 0) ? "Fail" : "Success");
  remove(image.c_str());

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

  return update_fpga(update_image);
}

int BmcFpgaComponent::update(string image)
{
  return update_wrapper(image, false);
}

int BmcFpgaComponent::fupdate(string image)
{
  return update_wrapper(image, true);
}

