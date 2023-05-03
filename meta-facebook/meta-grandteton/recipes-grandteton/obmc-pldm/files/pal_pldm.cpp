#include "pal_pldm.hpp"
#include "oem_pldm.hpp"
#include <iostream>
#include <vector>
#include <set>
#include <syslog.h>
#include <fmt/format.h>
#include <openbmc/kv.hpp>
#include <libpldm/firmware_update.h>

using namespace std;

const std::vector<std::string> comp_str_t = {
  "vr0",
  "vr1",
  "bic",
  "pex0",
  "pex1",
  "pex2",
  "pex3",
  "cpld"
};

static void delete_version_cache()
{
  for (auto&comp_name:comp_str_t) {
    try {
      auto key = fmt::format("swb_{}_active_ver", comp_name);
      kv::del(key, kv::region::temp);
    } catch (kv::key_does_not_exist&) {
      syslog(LOG_WARNING, "delete version cache failed.");
    }
  }
}

static void
pldm_firmware_parameter_handle (
       const pldm_get_firmware_parameters_resp& /*fwParams*/,
                          const variable_field& activeCompImageSetVerStr,
                          const variable_field& /*pendingCompImageSetVerStr*/,
          const pldm_component_parameter_entry& compEntry,
                          const variable_field& activeCompVerStr,
                          const variable_field& /*pendingCompVerStr*/) {
  string bic_ver = (activeCompImageSetVerStr.length == 0)?"":(const char*)activeCompImageSetVerStr.ptr;
  string bic_active_key  = "swb_bic_active_ver";
  string comp_name = comp_str_t[compEntry.comp_identifier];
  string comp_active_ver = (activeCompVerStr.length == 0)?"":(const char*)activeCompVerStr.ptr;
  string comp_active_key = fmt::format("swb_{}_active_ver", comp_name);

  bic_ver.resize(activeCompImageSetVerStr.length);
  comp_active_ver.resize(activeCompVerStr.length);

  // when BIC cannot access its component, it would return ERROR:%d string.
  auto found = comp_active_ver.find("ERROR:");
  comp_active_ver = (found == std::string::npos) ? comp_active_ver:"NA";

  kv::set(bic_active_key, bic_ver, kv::region::temp);
  kv::set(comp_active_key, comp_active_ver, kv::region::temp);
}

int
pal_pldm_get_firmware_parameter (uint8_t bus, uint8_t eid)
{
  vector<uint8_t> response{};
  vector<uint8_t> request {
    0x80,
    PLDM_FWUP,
    PLDM_GET_FIRMWARE_PARAMETERS
  };

  int ret = oem_pldm_send_recv(bus, eid, request, response);

  if (ret == 0) {
    auto pldm_resp = reinterpret_cast<pldm_msg*>(response.data());
    if (pldm_resp->payload[0] != 0x00) {
      delete_version_cache();
      return -1;
    }

    pldm_get_firmware_parameters_resp fwParams{};
    variable_field activeCompImageSetVerStr{};
    variable_field pendingCompImageSetVerStr{};
    variable_field compParamTable{};
    ret = decode_get_firmware_parameters_resp(
        (pldm_msg*)response.data(), response.size(), &fwParams, &activeCompImageSetVerStr,
        &pendingCompImageSetVerStr, &compParamTable);

    // decode failed
    if (ret) {
      syslog(LOG_WARNING, "Decoding GetFirmwareParameters response failed, rc = %d.", ret);
      std::cerr << "Decoding GetFirmwareParameters response failed, EID="
                << unsigned(eid) << ", RC=" << ret << "\n";
      ret = -1;

    // completion code != 0x00
    // } else if (fwParams.completion_code) {
    //   syslog(LOG_WARNING, "GetFirmwareParameters response failed with error, rc = %d.", ret);
    //   std::cerr << "GetFirmwareParameters response failed with error "
    //                 "completion code, EID="
    //             << unsigned(eid)
    //             << ", CC=" << unsigned(fwParams.completion_code) << "\n";
    //   delete_version_cache();
    //   ret = -1;

    // success
    } else {
      auto compParamPtr = compParamTable.ptr;
      auto compParamTableLen = compParamTable.length;
      pldm_component_parameter_entry compEntry{};
      variable_field activeCompVerStr{};
      variable_field pendingCompVerStr{};

      while (fwParams.comp_count-- && (compParamTableLen > 0)) {
        ret = decode_get_firmware_parameters_resp_comp_entry(
            compParamPtr, compParamTableLen, &compEntry,
            &activeCompVerStr, &pendingCompVerStr);

        // decode component table failed
        if (ret) {
          syslog(LOG_WARNING, "Decoding component parameter table entry failed, rc = %d.", ret);
          std::cerr << "Decoding component parameter table entry failed, EID="
                    << unsigned(eid) << ", RC=" << ret << "\n";

          ret = -1;
          break;
        }

        // stored
        pldm_firmware_parameter_handle (
          fwParams,
          activeCompImageSetVerStr,
          pendingCompImageSetVerStr,
          compEntry,
          activeCompVerStr,
          pendingCompVerStr
        );

        compParamPtr += sizeof(pldm_component_parameter_entry) +
                          activeCompVerStr.length + pendingCompVerStr.length;
        compParamTableLen -= sizeof(pldm_component_parameter_entry) +
                                activeCompVerStr.length + pendingCompVerStr.length;
      }
    }
  } else {
    syslog(LOG_WARNING, "Function oem_pldm_send_recv() error, rc = %d.", ret);
    std::cerr << "Function oem_pldm_send_recv() error, EID="
                << unsigned(eid) << ", RC=" << ret << "\n";
  }

  return ret;
}

int
is_pldm_supported(uint8_t bus, uint8_t eid)
{
  return pal_pldm_get_firmware_parameter(bus, eid);
}