#ifndef _BIOS_H_
#define _BIOS_H_

#include "spiflash.h"

class BiosComponent : public GPIOSwitchedSPIMTDComponent {
  protected:
    std::string _ver_prefix;
    virtual int extract_signature(const char *path, std::string &sig);
    virtual int check_image(const char *path);
    int update(std::string image, bool force);
  public:
    BiosComponent(std::string fru, std::string comp, std::string mtd, std::string devpath, std::string dev,
      std::string shadow, bool level, std::string verp) :
      GPIOSwitchedSPIMTDComponent(fru, comp, mtd, dev, shadow, level), _ver_prefix(verp) { spipath = devpath; }
    BiosComponent(std::string fru, std::string comp, std::string mtd, std::string dev, std::string shadow, bool level, std::string verp) :
      GPIOSwitchedSPIMTDComponent(fru, comp, mtd, dev, shadow, level), _ver_prefix(verp) {}
    BiosComponent(std::string fru, std::string comp, std::string mtd, std::string verp) :
      GPIOSwitchedSPIMTDComponent(fru, comp, mtd, "spi1.0", "BMC_BIOS_FLASH_CTL", true), _ver_prefix(verp) {}
    int get_version(json& j) override;
    int update(std::string image) override;
    int fupdate(std::string image) override;
    virtual int setMeRecovery(uint8_t retry);
    virtual int setDeepSleepWell(bool setting);
    virtual int reboot(uint8_t fruid);
};

#endif
