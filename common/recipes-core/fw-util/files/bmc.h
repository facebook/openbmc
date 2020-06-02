#ifndef _BMC_H_
#define _BMC_H_
#include <string>
#include "fw-util.h"

class BmcComponent : public Component {
  protected:
    System &system;
    std::string _mtd_name;
    std::string _vers_mtd;
    size_t _writable_offset;
    size_t _skip_offset;
    std::string get_bmc_version(void);
    std::string get_bmc_version(const std::string &mtd);
  public:
    BmcComponent(std::string fru, std::string comp, System &sys, std::string mtd, std::string vers = "", size_t w_offset = 0, size_t skip_offset = 0)
      : Component(fru, comp), system(sys), _mtd_name(mtd), _vers_mtd(vers), _writable_offset(w_offset), _skip_offset(skip_offset) {}

    int update(std::string image);
    int print_version();
    void get_ver_in_json(json& j);
    virtual bool is_valid(std::string &image, bool pfr_active);
};

#endif
