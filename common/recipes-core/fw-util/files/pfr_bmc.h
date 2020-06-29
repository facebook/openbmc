#ifndef _PFR_BMC_H_
#define _PFR_BMC_H_

#include "bmc.h"

class PfrBmcComponent : public BmcComponent {
  public:
    PfrBmcComponent(std::string fru, std::string comp, std::string mtd, std::string vers = "")
      : BmcComponent(fru, comp, sys, mtd, vers) {}
    int update(std::string image) override;
};

#endif
