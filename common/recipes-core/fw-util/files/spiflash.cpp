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
  size_t size, erasesize;
  static constexpr size_t BUFSIZE = 4096;
  auto KB = [](auto x){ return x / 1024; };

  if (!sys().get_mtd_name(_mtd_name, dev, size, erasesize)) {
    return FW_STATUS_FAILURE;
  }

  ifstream ifs(dev.c_str(), ios::in | ios::binary);
  if (!ifs.is_open()) {
    sys().error << "ERROR: Failed to open " << dev << endl;
    return FW_STATUS_FAILURE;
  }

  ofstream ofs(image.c_str(), ios::out | ios::binary);
  if (!ofs.is_open()) {
    ifs.close();
    sys().error << "ERROR: Failed to open " << image << endl;
    return FW_STATUS_FAILURE;
  }

  char buf[BUFSIZE];
  size_t remaining = size;
  uint64_t written;

  sys().output << "Reading from device: " << dev << endl;
  for (written = 0; written < size;) {
    ifs.read(buf, min(sizeof(buf), remaining));
    if (ifs.fail()) {
      if (errno == EINTR) {
        ifs.clear();
        continue;
      }
      sys().error << "ERROR: Failed to read " << dev << endl;
      return FW_STATUS_FAILURE;
    }
    ofs.write(buf, ifs.gcount());
    if (ofs.fail()) {
      if (errno == EINTR) {
        ofs.clear();
        ifs.seekg(-ifs.gcount(), ios::cur);
        continue;
      }
      sys().error << "ERROR: Failed to write " << image << endl;
      return FW_STATUS_FAILURE;
    }
    written += ifs.gcount();
    remaining -= ifs.gcount();

    cout << "\rReading kb: " << KB(written) << "/" << KB(size)
         << " (" << (written * 100 / size) << "%)" << flush;
  }
  cout << endl;
  ifs.close();
  ofs.close();

  return FW_STATUS_SUCCESS;
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
