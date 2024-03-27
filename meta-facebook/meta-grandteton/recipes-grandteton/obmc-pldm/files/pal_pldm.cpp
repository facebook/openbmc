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

enum {
  SWB_FW_GET_SUCCESS                =  0,
  SWB_FW_GET_FAILED                 = -1,
  SWB_FW_BIC_ERROR                  = -2,
};

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

class pldm_firmware_parameter_handler
{
  private:
    enum {
      UPDATE_SWB_CACHE,
      NON_UPDATE_SWB_CACHE
    };
    std::string _fru_name{};
    std::string _active_ver{};
    void delete_swb_cache();
    int swb_cache_handle(
                          const pldm_get_firmware_parameters_resp&,
                          const variable_field&,
                          const variable_field&,
                          const pldm_component_parameter_entry&,
                          const variable_field&,
                          const variable_field&
                        );
    int get_firmware_parameter(
                        uint8_t bus, uint8_t eid, int state);

  public:
    pldm_firmware_parameter_handler() {}
    pldm_firmware_parameter_handler(string fru_name): _fru_name(fru_name) {}
    int update_swb_cache(uint8_t bus, uint8_t eid);
    int get_active_ver(uint8_t bus, uint8_t eid, std::string& active_ver);
};

void
pldm_firmware_parameter_handler::delete_swb_cache()
{
  for (auto&comp_name:comp_str_t) {
    try {
      auto key = fmt::format("{}_{}_active_ver", _fru_name, comp_name);
      kv::del(key, kv::region::temp);
    } catch (kv::key_does_not_exist&) {
      syslog(LOG_WARNING, "delete version cache failed.");
    }
  }
}

int
pldm_firmware_parameter_handler::swb_cache_handle (
       const pldm_get_firmware_parameters_resp& /*fwParams*/,
                          const variable_field& activeCompImageSetVerStr,
                          const variable_field& /*pendingCompImageSetVerStr*/,
          const pldm_component_parameter_entry& compEntry,
                          const variable_field& activeCompVerStr,
                          const variable_field& /*pendingCompVerStr*/) {

  string bic_ver = (activeCompImageSetVerStr.length == 0)?"":(const char*)activeCompImageSetVerStr.ptr;
  string bic_active_key  = fmt::format("{}_bic_active_ver", _fru_name);

  if (compEntry.comp_identifier >= comp_str_t.size()) {
    return SWB_FW_BIC_ERROR;
  }
  string comp_name = comp_str_t[compEntry.comp_identifier];
  string comp_active_key = fmt::format("{}_{}_active_ver", _fru_name, comp_name);
  string comp_active_ver = (activeCompVerStr.length == 0)?"":(const char*)activeCompVerStr.ptr;

  bic_ver.resize(activeCompImageSetVerStr.length);
  comp_active_ver.resize(activeCompVerStr.length);

  // when BIC cannot access its component, it would return ERROR:%d string.
  auto found = comp_active_ver.find("ERROR:");
  int ret = 0;
  if (found == std::string::npos) {
    kv::set(bic_active_key, bic_ver, kv::region::temp);
    kv::set(comp_active_key, comp_active_ver, kv::region::temp);
  } else {
    // BIC will return error code 8 when its component is power off
    found = comp_active_ver.find("8");
    if (found == std::string::npos) {
      ret = SWB_FW_BIC_ERROR;
    } else {
      kv::set(bic_active_key, bic_ver, kv::region::temp);
      kv::set(comp_active_key, "NA", kv::region::temp);
    }
  }

  return ret;
}

int
pldm_firmware_parameter_handler::get_firmware_parameter (uint8_t bus, uint8_t eid, int state)
{
  vector<uint8_t> response{};
  vector<uint8_t> request {
    0x80,
    PLDM_FWUP,
    PLDM_GET_FIRMWARE_PARAMETERS
  };

  int ret = oem_pldm_send_recv(bus, eid, request, response);

  if (ret == PLDM_SUCCESS) {
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
        if (state == UPDATE_SWB_CACHE) {
          ret = swb_cache_handle (
            fwParams,
            activeCompImageSetVerStr,
            pendingCompImageSetVerStr,
            compEntry,
            activeCompVerStr,
            pendingCompVerStr
          );

        } else {
          _active_ver = (activeCompImageSetVerStr.length == 0) ?
                        "" : (const char*)activeCompImageSetVerStr.ptr;
        }

        compParamPtr += sizeof(pldm_component_parameter_entry) +
                          activeCompVerStr.length + pendingCompVerStr.length;
        compParamTableLen -= sizeof(pldm_component_parameter_entry) +
                                activeCompVerStr.length + pendingCompVerStr.length;
      }
    }
  } else if (ret == PLDM_ERROR_UNSUPPORTED_PLDM_CMD) {
    if (state == UPDATE_SWB_CACHE)
      delete_swb_cache();
    ret = -1;
  } else {
    syslog(LOG_WARNING, "Function oem_pldm_send_recv() error, rc = %d.", ret);
    ret = -1;
  }

  return ret;
}

int
pldm_firmware_parameter_handler::update_swb_cache(uint8_t bus, uint8_t eid)
{
  return get_firmware_parameter(bus, eid, UPDATE_SWB_CACHE);
}

int
pldm_firmware_parameter_handler::get_active_ver(
                                uint8_t bus, uint8_t eid, std::string& active_ver)
{
  int ret = get_firmware_parameter(bus, eid, NON_UPDATE_SWB_CACHE);
  active_ver = _active_ver;
  return ret;
}

int get_pldm_active_ver(uint8_t bus, uint8_t eid, std::string& active_ver)
{
  pldm_firmware_parameter_handler handler;
  return handler.get_active_ver(bus, eid, active_ver);
}

int update_pldm_ver_cache(const std::string& fru, uint8_t bus, uint8_t eid)
{
  pldm_firmware_parameter_handler handler(fru);
  return handler.update_swb_cache(bus, eid);
}

