#include <string>
#include "bmc.h"

using namespace std;

int BmcComponent::update(const string& image_path [[maybe_unused]])
{
  return 0;
}

std::string BmcComponent::get_bmc_version()
{
  return 0;
}

std::string BmcComponent::get_bmc_version(const std::string &mtd [[maybe_unused]])
{
  return 0;
}

int BmcComponent::print_version()
{
  return FW_STATUS_NOT_SUPPORTED;
}

int BmcComponent::get_version(json& j) {
  j["VERSION"] = sys().version();
  return FW_STATUS_SUCCESS;
}

static BmcComponent bmc("bmc", "bmc", "flash0", "u-boot");
