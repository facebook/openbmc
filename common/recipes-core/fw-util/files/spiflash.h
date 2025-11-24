#ifndef _SPI_FLASH_H_
#define _SPI_FLASH_H_
#include <string>
#include "fw-util.h"


// Upgrade flash whose partitions are mounted as MTD
class MTDComponent : public Component {
  protected:
    std::string _mtd_name;
    std::string flash_method = "flashcp";
  public:
    MTDComponent(const std::string& fru, const std::string& comp, const std::string& mtd) :
      Component(fru, comp), _mtd_name(mtd) {}
    int update(const std::string& image) override;
    int dump(const std::string& image) override;
  private:
    int getFlashSize(const std::string& mtdIndex);
    int update_by_flashcp(const std::string& image);
    int update_by_flashrom(const std::string& image);
};

// Upgrade SPI Flash whose partitions need to be temporarily mounted as MTD
class SPIMTDComponent : public MTDComponent {
  protected:
    std::string spipath = "/sys/bus/spi/drivers/m25p80";
    std::string spidev;
  public:
    SPIMTDComponent(const std::string& fru, const std::string& comp, const std::string& mtd, const std::string& dev) :
      MTDComponent(fru, comp, mtd), spidev(dev) {}
    int update(const std::string& image) override;
    int dump(const std::string& image) override;
};

// These SPI devices are connected only a GPIO is asserted.
class GPIOSwitchedSPIMTDComponent : public SPIMTDComponent {
  protected:
    std::string gpio_shadow;
    bool access_level;
    bool change_direction;
  public:
  GPIOSwitchedSPIMTDComponent(const std::string& fru, const std::string& comp, const std::string& mtd, const std::string& dev, const std::string& shadow, bool level, bool change = true) :
    SPIMTDComponent(fru, comp, mtd, dev), gpio_shadow(shadow), access_level(level), change_direction(change) {}
    int update(const std::string& image) override;
    int dump(const std::string& image) override;
};

// Upgrade SPI Flash using external programmers
class ExternalSPIComponent : public Component {
  private:
    std::string _programmer_type;
    std::string _chip_params;
    
  public:
    ExternalSPIComponent(const std::string& fru, 
                        const std::string& comp,
                        const std::string& programmer_type = "ft2232_spi",
                        const std::string& chip_params = "type=2232H,port=B") :
      Component(fru, comp),
      _programmer_type(programmer_type),
      _chip_params(chip_params) {}
    
    int update(const std::string& image) override;
    int dump(const std::string& image) override;
};

#endif
