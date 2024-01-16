#include "fw-util.h"
#include <sstream>
#include <iomanip>
#include "bic_me.h"
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int MeComponent::get_version(json& j) {
  uint8_t ver[32] = {0};
  char ctarget_name[16] = {0};

  pal_get_me_name(slot_id, ctarget_name);
  string target_name(ctarget_name);
  j["PRETTY_COMPONENT"] = target_name;
  try {
    server.ready();
    if (bic_get_fw_ver(slot_id, FW_ME, ver)) {
      j["VERSION"] = "NA";
    }
    else {
      stringstream ss;
      if (target_name == "ME") {
        ss << std::hex << +ver[0] << '.' << +ver[1] << '.' << +ver[2] << '.'
          << +ver[3] << +ver[4];
        j["VERSION"] = ss.str();
      } else if (target_name == "IMC") {
        ss << "IMC.DF." << std::hex << +ver[0] << '.' << +ver[1] << '.'
          << +ver[2] << '-' << +ver[3] << +ver[4] << +ver[5] << +ver[6] << +ver[7];
        j["VERSION"] = ss.str();
      } else if (target_name == "M3") {
        ss << std::hex << +ver[0] << '.';
        ss << std::hex << std::setfill('0') << std::setw(2) << +ver[1];
        j["VERSION"] = ss.str();
      }
    }
  } catch(string err) {
    j["VERSION"] = "NA (" + err + ")";
  }
  return 0;
}
#endif

