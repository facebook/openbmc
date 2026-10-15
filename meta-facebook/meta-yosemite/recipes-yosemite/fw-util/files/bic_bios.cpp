#include "fw-util.h"
#include <stdexcept>
#include <unistd.h>
#include "bic_bios.h"
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int BiosComponent::update(const string& image) {
  int ret;

  try {
    server.ready();
    pal_set_server_power(slot_id, SERVER_POWER_OFF);
    ret = bic_update_firmware(slot_id, UPDATE_BIOS, (char *)image.c_str(), 0);
    sleep(1);
    pal_set_server_power(slot_id, SERVER_12V_CYCLE);
    sleep(5);
    pal_set_server_power(slot_id, SERVER_POWER_ON);
  } catch(const std::exception& err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BiosComponent::fupdate(const string& image) {
  int ret;

  try {
    server.ready();
    pal_set_server_power(slot_id, SERVER_POWER_OFF);
    ret = bic_update_firmware(slot_id, UPDATE_BIOS, (char *)image.c_str(), 1);
    sleep(1);
    pal_set_server_power(slot_id, SERVER_12V_CYCLE);
    sleep(5);
    pal_set_server_power(slot_id, SERVER_POWER_ON);
  } catch(const std::exception& err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BiosComponent::get_version(json& j) {
  uint8_t ver[32] = {0};

  j["PRETTY_COMPONENT"] = "BIOS";
  try {
    server.ready();
    if (pal_get_sysfw_ver(slot_id, ver)) {
      j["VERSION"] = "NA";
    } else {
      // BIOS version response contains the length at offset 2 followed by ascii string
      string sver((const char *)(ver + 3), size_t(ver[2]));
      j["VERSION"] = sver;
    }
  } catch (const std::exception& err) {
    j["VERSION"] = "NA (" + string(err.what()) + ")";
  }
  return 0;
}
#endif
