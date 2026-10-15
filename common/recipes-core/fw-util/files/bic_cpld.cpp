#include "fw-util.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <stdlib.h>
#include "bic_cpld.h"
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int CpldComponent::get_version(json& j) {
  try {
    server.ready();
    uint8_t ver[32] = {0};
    j["PRETTY_COMPONENT"] = "CPLD";
    if (bic_get_fw_ver(slot_id, FW_CPLD, ver)) {
      j["VERSION"] = "NA";
    }
    else {
      stringstream ss;
      ss << "0x";
      ss << std::hex << setfill('0')
        << setw(2) << +ver[0]
        << setw(2) << +ver[1]
        << setw(2) << +ver[2]
        << setw(2) << +ver[3];
      j["VERSION"] = ss.str();
    }
  } catch(const std::exception& err) {
    j["VERSION"] = "NA (" + string(err.what()) + ")";
  }
  return 0;
}
int CpldComponent::update(const string& image) {
  int ret = 0;
  try {
    server.ready();
    ret = bic_update_fw(slot_id, UPDATE_CPLD, (char *)image.c_str());
  } catch(const std::exception& err) {
    ret = FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}
#endif
