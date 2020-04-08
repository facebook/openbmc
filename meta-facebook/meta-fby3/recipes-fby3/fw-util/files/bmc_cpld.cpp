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
#include "bmc_cpld.h"
#include <facebook/bic.h>
#include <facebook/fby3_common.h>

using namespace std;

image_info BmcCpldComponent::check_image(string image, bool force) {
#define MAX10_RPD_SIZE 0x5C000
  string flash_image = image;
  uint8_t bmc_location = 0;

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

  uint8_t *memblock = new uint8_t [MAX10_RPD_SIZE + 1];//data_size + signed byte
  uint8_t signed_byte = 0;
  size_t r_b = read(fd_r, memblock, MAX10_RPD_SIZE + 1);
  size_t w_b = 0;

  //it's an old image
  if ( r_b == MAX10_RPD_SIZE ) {
    signed_byte = 0x0;
  } else if ( r_b == (MAX10_RPD_SIZE + 1) ) {
    signed_byte = memblock[MAX10_RPD_SIZE] & 0xf;
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
    //CPLD is located on class 2(NIC_EXP)
    if ( bmc_location == NIC_BMC ) {
      if ( signed_byte != NICEXP ) {
        cout << "image is not a valid CPLD image for NIC expansion board" << endl;
      } else {
        image_sts.result = true;
      }
    } else if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
      if ( signed_byte != BICBB ) {
        cout << "image is not a valid CPLD image for baseboard" << endl;
      } else {
        image_sts.result = true;
      }
    }
  }

  //release the resource
  close(fd_r);
  close(fd_w);
  delete[] memblock;

  return image_sts;
}

int BmcCpldComponent::get_cpld_version(uint8_t *ver) {
  int ret = 0;
  uint8_t tbuf[4] = {0x00, 0x20, 0x00, 0x28};
  uint8_t tlen = 4;
  uint8_t rlen = 4;
  int i2cfd = 0;
  string i2cdev = "/dev/i2c-" + to_string(bus);

  if ((i2cfd = open(i2cdev.c_str(), O_RDWR)) < 0) {
    printf("Failed to open %s\n", i2cdev.c_str());
    return -1;
  }

  if (ioctl(i2cfd, I2C_SLAVE, addr) < 0) {
    printf("Failed to talk to slave@0x%02X\n", addr);
  } else {
    ret = i2c_rdwr_msg_transfer(i2cfd, addr << 1, tbuf, tlen, ver, rlen);
  }

  if ( i2cfd > 0 ) close(i2cfd);

  return ret;
}

/*
 * The definition of ON_CHIP_FLASH_USER_VER in fby3 is different to libfpga. 
 * TODO: We should add ON_CHIP_FLASH_USER_VER into altera_max10_attr_t, 
 * so it can be defined by each project.
*/
int BmcCpldComponent::print_version()
{
  uint8_t ver[4] = {0};
  string fru_name = fru();
  try {
    // Print CPLD Version
    transform(fru_name.begin(), fru_name.end(), fru_name.begin(), ::toupper);
    if ( get_cpld_version(ver) < 0 ) {
      printf("%s CPLD Version: NA\n", fru_name.c_str());
    }
    else {
      printf("%s CPLD Version: %02X%02X%02X%02X\n", fru_name.c_str(), ver[3], ver[2], ver[1], ver[0]);
    }
  } catch(string err) {
    printf("%s CPLD Version: NA (%s)\n", fru_name.c_str(), err.c_str());
  }
  return 0;
}

int BmcCpldComponent::update(string image)
{
  int ret = 0;
  char key[32] = {0};
  image_info image_sts = check_image(image, false);

  if ( image_sts.result == false ) {
    remove(image_sts.new_path.c_str());
    return FW_STATUS_FAILURE;
  } 

  //use the new path
  image = image_sts.new_path;

  if (cpld_intf_open(pld_type, INTF_I2C, &attr)) {
    printf("Cannot open i2c!\n");
    return -1;
  }

  ret = cpld_program((char *)image.c_str(), key, false);
  cpld_intf_close(INTF_I2C);
  if (ret) {
    printf("Error Occur at updating CPLD FW!\n");
  }

  //remove the tmp file
  remove(image.c_str());
  return ret;
}

int BmcCpldComponent::fupdate(string image) 
{
  int ret = 0;
  char key[32] = {0};
  image_info image_sts = check_image(image, true);

  image = image_sts.new_path;

  if (cpld_intf_open(pld_type, INTF_I2C, &attr)) {
    printf("Cannot open i2c!\n");
    return -1;
  }

  ret = cpld_program((char *)image.c_str(), key, false);
  cpld_intf_close(INTF_I2C);
  if (ret) {
    printf("Error Occur at updating CPLD FW!\n");
  }

  //remove the tmp file
  remove(image.c_str());
  return ret;
}

