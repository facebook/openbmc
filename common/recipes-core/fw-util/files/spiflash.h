#ifndef _SPI_FLASH_H_
#define _SPI_FLASH_H_
#include <string>
#include "fw-util.h"


// Upgrade flash whose partitions are mounted as MTD
class MTDComponent : public Component {
  protected:
    std::string _mtd_name;

  public:
    MTDComponent(std::string fru, std::string comp, std::string mtd) :
      Component(fru, comp), _mtd_name(mtd) {}
    int update(std::string image) override;
};

// Upgrade SPI Flash whose partitions need to be temporarily mounted as MTD
class SPIMTDComponent : public MTDComponent {
  protected:
    std::string spidev;
  public:
    SPIMTDComponent(std::string fru, std::string comp, std::string mtd, std::string dev) :
      MTDComponent(fru, comp, mtd), spidev(dev) {}
    int update(std::string image) override;
};

// These SPI devices are connected only a GPIO is asserted.
class GPIOSwitchedSPIMTDComponent : public SPIMTDComponent {
  protected:
    std::string gpio_shadow;
    bool access_level;
  public:
  GPIOSwitchedSPIMTDComponent(std::string fru, std::string comp, std::string mtd, std::string dev, std::string shadow, bool level) :
    SPIMTDComponent(fru, comp, mtd, dev), gpio_shadow(shadow), access_level(level) {}
  int update(std::string image) override;
};

#endif
