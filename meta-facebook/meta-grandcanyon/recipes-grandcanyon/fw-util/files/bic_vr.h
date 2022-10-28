#ifndef _BIC_VR_H_
#define _BIC_VR_H_
#include <string>
#include <facebook/fbgc_common.h>
#include "fw-util.h"
#include "server.h"


class VrComponent : public Component {
  Server server;

  private:
    int get_ver_str(uint8_t& addr, std::string& s);
  public:
    VrComponent(std::string fru, std::string comp)
      : Component(fru, comp), server(FRU_SERVER, fru){}
    int print_version();
    int update(std::string image);
    int get_version(json& j) override;
};

#endif
