#include <openbmc/hgx.h>
#include <iostream>
#include "fw-util.h"

class HGXComponent : public Component {
  std::string _hgxcomp{};

 public:
  HGXComponent(
      const std::string& fru,
      const std::string& comp,
      const std::string& hgxcomp = "")
      : Component(fru, comp), _hgxcomp(hgxcomp) {}
  int update(std::string image) {
    try {
      hgx::update(image);
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      return -1;
    }
    return 0;
  }
  int get_version(json& j) {
    if (_hgxcomp == "") {
      return FW_STATUS_NOT_SUPPORTED;
    }
    try {
      j["VERSION"] = hgx::version(_hgxcomp);
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      return FW_STATUS_FAILURE;
    }
    return FW_STATUS_SUCCESS;
  }
};

HGXComponent autocomp("hgx", "auto");
// TODO
// HGXComponent("hgx", "hmc", "HMC");
