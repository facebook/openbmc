#include "fw-util.h"

class HmcComponent : public Component {
  public:
    HmcComponent(const std::string& fru, const std::string& comp)
      : Component(fru, comp) {}

    int update(std::string image);
    int print_version();
};


int HmcComponent::update(const std::string image)
{
  std::stringstream cmd_str;
  int ret;

  cmd_str << "hmc-util --update " << component() << " " << image;
  ret = sys().runcmd(cmd_str.str());
  return ret;
}

int HmcComponent::print_version()
{
  std::stringstream cmd_str;
  int ret;

  cmd_str << "hmc-util --version " << component();
  ret = sys().runcmd(cmd_str.str());
  return ret;
}


static HmcComponent hmc_bundle("hmc", "bundle");
static HmcComponent hmc_sbios("hmc", "sbios");
