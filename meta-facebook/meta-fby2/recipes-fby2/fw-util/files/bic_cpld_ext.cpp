#include "fw-util.h"
#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include "bic_cpld.h"
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

class CpldExtComponent : public CpldComponent {
  uint8_t slot_id = 0;
  Server server;
  public:
    CpldExtComponent(string fru, string comp, uint8_t _slot_id)
      : CpldComponent(fru, comp, _slot_id), slot_id(_slot_id), server(_slot_id, fru) {}
    int get_version(json& j) override;
};

int CpldExtComponent::get_version(json& j) {
  if (fby2_get_slot_type(slot_id) != SLOT_TYPE_GPV2) {
    return CpldComponent::get_version(j);
  }
  // Custom version display for GPv2.
  j["PRETTY_COMPONENT"] = "CPLD";
  try {
    server.ready();
    uint8_t ver[32] = {0};
    if (bic_get_fw_ver(slot_id, FW_CPLD, ver)) {
      j["VERSION"] = "NA";
    } else {
      stringstream ss;
      ss << "0x" << std::hex << std::setfill('0') << std::setw(2)
        << +ver[0];
      j["VERSION"] = ss.str();
    }
  } catch(string err) {
    j["VERSION"] = "NA (" + err + ")";
  }
  return 0;
}

// Register CPLD components
CpldExtComponent cpld1("slot1", "cpld", 1);
CpldExtComponent cpld2("slot2", "cpld", 2);
CpldExtComponent cpld3("slot3", "cpld", 3);
CpldExtComponent cpld4("slot4", "cpld", 4);

#endif
