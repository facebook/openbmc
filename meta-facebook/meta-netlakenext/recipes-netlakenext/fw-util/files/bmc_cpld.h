#ifndef _BMC_CPLD_H_
#define _BMC_CPLD_H_
#include "fw-util.h"
#include <optional>
#include <iomanip>
#include <sstream>

class BmcCpldComponent : public Component {
  std::string cpld_type;
  std::string bus;
  std::string address;

  private:
    int run_command(const std::string& cmd, std::string* output) const;
    std::optional<std::string> get_ver_from_string(const std::string& result) const;
  public:
    BmcCpldComponent(const std::string& fru, const std::string& comp, const std::string& _cpld_type, const std::string& _bus, const std::string& _addr)
      : Component(fru, comp), cpld_type(_cpld_type), bus(_bus), address(_addr){}
    int print_version() override;
    int get_version(json& ver_json) override;
    int update(const std::string& image) override;
};

#endif
