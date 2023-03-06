#include "spiflash.h"
#include <fstream>
#include <thread>
#include <chrono>
#include <syslog.h>
#include <openbmc/libgpio.h>

using namespace std;

int MTDComponent::update(std::string image)
{
  string dev;
  string cmd;
  string comp = this->component();
  int ret;

  if (!sys().get_mtd_name(_mtd_name, dev)) {
    return FW_STATUS_FAILURE;
  }
  syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());

  sys().output << "Flashing to device: " << dev << endl;
  cmd = "flashcp -v " + image + " " + dev;
  ret = sys().runcmd(cmd);
  if (ret == 0) {
    syslog(LOG_CRIT, "Component %s upgrade completed", comp.c_str());
    return FW_STATUS_SUCCESS;
  }
  return FW_STATUS_FAILURE;
}

int MTDComponent::dump(std::string image)
{
  string dev;
  string comp = this->component();
  int ret;

  size_t size, erasesize;
  if (!sys().get_mtd_name(_mtd_name, dev, size, erasesize)) {
    return FW_STATUS_FAILURE;
  }

  sys().output << "Reading fom device: " << dev << endl;
  stringstream cmd;
  cmd << "mtd_debug" << ' '
      << "read" << ' '
      << dev << ' '
      << "0" << ' '
      << size <<' '
      << image;

  ret = sys().runcmd(cmd.str());
  if (ret == 0) {
    return FW_STATUS_SUCCESS;
  }
  return FW_STATUS_FAILURE;
}


int SPIMTDComponent::update(std::string image)
{
  string cmd;
  std::ofstream ofs;
  int rc;

  ofs.open(spipath + "/bind");
  if (!ofs.is_open()) {
    return -1;
  }
  ofs << spidev;
  if (ofs.bad()) {
    ofs.close();
    sys().error << spidev << " Bind failed" << std::endl;
    return -1;
  }
  ofs.close();
  rc = MTDComponent::update(image);
  ofs.open(spipath + "/unbind");
  if (!ofs.is_open()) {
    sys().error << "ERROR: Cannot unbind " << spidev << " rc=" << rc << std::endl;
    return -1;
  }
  ofs << spidev;
  ofs.close();
  return rc;
}

int SPIMTDComponent::dump(std::string image)
{
  string cmd;
  std::ofstream ofs;
  int rc;

  ofs.open(spipath + "/bind");
  if (!ofs.is_open()) {
    return -1;
  }
  ofs << spidev;
  if (ofs.bad()) {
    ofs.close();
    sys().error << spidev << " Bind failed" << std::endl;
    return -1;
  }
  ofs.close();
  rc = MTDComponent::dump(image);
  ofs.open(spipath + "/unbind");
  if (!ofs.is_open()) {
    sys().error << "ERROR: Cannot unbind " << spidev << " rc=" << rc << std::endl;
    return -1;
  }
  ofs << spidev;
  ofs.close();
  return rc;
}

int GPIOSwitchedSPIMTDComponent::update(std::string  image)
{
  int rc;
  gpio_desc_t *desc = gpio_open_by_shadow(gpio_shadow.c_str());
  if (!desc) {
    return -1;
  }
  if (gpio_set_direction(desc, GPIO_DIRECTION_OUT) ||
      gpio_set_value(desc, access_level ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW)) {
    gpio_close(desc);
    return -1;
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  rc = SPIMTDComponent::update(image);
  if (gpio_set_value(desc, access_level ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH) ||
      (change_direction && gpio_set_direction(desc, GPIO_DIRECTION_IN))) {
    gpio_close(desc);
    sys().error << "ERROR: Cannot release control of SPI Dev"  << std::endl;
    return -1;
  }
  gpio_close(desc);
  return rc;
}

int GPIOSwitchedSPIMTDComponent::dump(std::string  image)
{
  int rc;
  gpio_desc_t *desc = gpio_open_by_shadow(gpio_shadow.c_str());
  if (!desc) {
    return -1;
  }
  if (gpio_set_direction(desc, GPIO_DIRECTION_OUT) ||
      gpio_set_value(desc, access_level ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW)) {
    gpio_close(desc);
    return -1;
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  rc = SPIMTDComponent::dump(image);
  if (gpio_set_value(desc, access_level ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH) ||
      (change_direction && gpio_set_direction(desc, GPIO_DIRECTION_IN))) {
    gpio_close(desc);
    sys().error << "ERROR: Cannot release control of SPI Dev"  << std::endl;
    return -1;
  }
  gpio_close(desc);
  return rc;
}
