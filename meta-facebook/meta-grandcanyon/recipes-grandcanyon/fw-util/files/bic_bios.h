#ifndef BIC_BIOS_H_
#define BIC_BIOS_H_
#include <facebook/fbgc_common.h>
#include "fw-util.h"
#include "server.h"
#include <facebook/bic.h>


class BiosComponent : public Component {
  uint8_t fw_comp = 0;
  Server server;

  private:
    int get_ver_str(std::string& s);
  public:
    BiosComponent(std::string fru, std::string comp, uint8_t _fw_comp)
      : Component(fru, comp), fw_comp(_fw_comp), server(FRU_SERVER, fru) {}
    int update(std::string image);
    int fupdate(std::string image);
    int print_version();
    void get_version(json& j);
};

#endif
