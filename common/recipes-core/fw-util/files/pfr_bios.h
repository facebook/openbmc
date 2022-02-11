#ifndef _PFR_BIOS_H_
#define _PFR_BIOS_H_

#include "bios.h"

enum {
  PCH_PFM_ACTIVE = 0,
  PCH_PFM_RC,
};

class PfrBiosComponent : public BiosComponent {
  protected:
    uint8_t pfm_type;
    int update(std::string image, bool force);
  public:
    PfrBiosComponent(std::string fru, std::string comp, uint8_t type, std::string verp) :
      BiosComponent(fru, comp, "", verp), pfm_type(type) {}
    int print_version() override;
    int update(std::string image) override;
    int fupdate(std::string image) override;
};

#endif
