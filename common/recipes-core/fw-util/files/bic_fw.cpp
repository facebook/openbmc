#include "fw-util.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include "server.h"
#include <openbmc/pal.h>
#include "bic_fw.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int BicFwComponent::update(const std::string& image) {
  int ret;
  try {
    server.ready();
    ret = bic_update_fw(slot_id, UPDATE_BIC, (char *)image.c_str());
  } catch (const std::exception& err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BicFwComponent::fupdate(const string& image) {
  int ret;
  try {
    server.ready();
    ret = pal_force_update_bic_fw(slot_id, UPDATE_BIC, (char *)image.c_str());
  } catch (const std::exception& err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BicFwComponent::get_version(json& j) {
  uint8_t ver[32];
  j["PRETTY_COMPONENT"] = "Bridge-IC";
  try {
    server.ready();
    // Print Bridge-IC Version
    if (bic_get_fw_ver(slot_id, FW_BIC, ver)) {
      j["VERSION"] = "NA";
    } else {
      stringstream ss;
      ss << "v" << std::hex << +ver[0] << '.';
      ss << std::hex << std::setfill('0') << std::setw(2) << +ver[1];
      j["VERSION"] = ss.str();
    }
  } catch(const std::exception& err) {
    j["VERSION"] = "NA (" + string(err.what()) + ")";
  }
  return 0;
}

int BicFwBlComponent::update(const std::string& image) {
  int ret;
  try {
    server.ready();
    ret = bic_update_fw(slot_id, UPDATE_BIC_BOOTLOADER, (char *)image.c_str());
  } catch(const std::exception& err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BicFwBlComponent::get_version(json& j) {
  uint8_t ver[32];
  uint8_t comp;
  j["PRETTY_COMPONENT"] = "Bridge-IC Bootloader";
  try {
    server.ready();
#ifdef CONFIG_FBY2_ND
  int ret;
  uint8_t server_type = 0xFF;

  ret = bic_get_server_type(slot_id, &server_type);
  if (ret) {
    throw std::runtime_error("BIC get server type failed");
  }

  switch (server_type) {
    case SERVER_TYPE_ND:
      comp = FW_BOOTLOADER;
      break;
    case SERVER_TYPE_TL:
    default:
      comp = FW_BIC_BOOTLOADER;
      break;
  }
#else
  comp = FW_BIC_BOOTLOADER;
#endif

    // Print Bridge-IC Bootloader Version
    if (bic_get_fw_ver(slot_id, comp, ver)) {
      j["VERSION"] = "NA";
    } else {
      stringstream ss;
      ss << 'v' << std::hex << +ver[0] << '.';
      ss << std::hex << std::setfill('0') << std::setw(2) << +ver[1];
      j["VERSION"] = ss.str();
    }
  } catch(const std::exception& err) {
    j["VERSION"] = "NA (" + string(err.what()) + ")";
  }
  return 0;
}
#endif

