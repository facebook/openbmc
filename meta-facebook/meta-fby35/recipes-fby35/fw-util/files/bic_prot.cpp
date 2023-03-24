#include <cstdio>
#include <fmt/format.h>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/kv.hpp>
#include <openbmc/pal.h>
#include "bic_prot.hpp"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

constexpr auto  MAX_RETRY = 30;

int ProtComponent::attempt_server_power_off(bool force) {
  int ret, retry_count;
  uint8_t status;

  for (retry_count = 0; retry_count < MAX_RETRY; retry_count++) {
    ret = pal_get_server_power(slot_id, &status);
    cerr << "Server power status: " << (int) status << endl;
    if ((ret == 0) && (status == SERVER_POWER_OFF)) {
      break;
    } else {
      uint8_t power_cmd = (force ? SERVER_POWER_OFF : SERVER_GRACEFUL_SHUTDOWN);
      if ((retry_count == 0) || force) {
        cerr << "Powering off the server (cmd " << ((int) power_cmd) << ")..." << endl;
        pal_set_server_power(slot_id, power_cmd);
      }
      sleep(2);
    }
  }
  if (retry_count == MAX_RETRY) {
    return -1;
}
  return 0;
}

bool ProtComponent::checkPfrUpdate(prot::ProtDevice& prot_dev) {
    cerr << "PRoT is PFR mode , update to recovery Flash" << endl;
    prot::BOOT_STATUS_ACK_PAYLOAD boot_sts{};
    if (prot_dev.portGetBootStatus(boot_sts) == prot::ProtDevice::DevStatus::SUCCESS) {
      prot::ProtSpiInfo::updateProperty upateProp(boot_sts, FW_PROT, FW_PROT_SPIB);
      fw_comp = upateProp.target;
      return upateProp.isPfrUpdate;
    } else {
      throw runtime_error("Error in getting XFR Boot Status");
    }
}

int ProtComponent::update_internal(const std::string& image, int fd, bool force) {
  int ret;
  bool pfr_update = false;
  uint8_t dev_i2c_bus;
  uint8_t dev_i2c_addr;
  if (pal_get_prot_address(slot_id, &dev_i2c_bus, &dev_i2c_addr) != 0) {
    cerr << "pal_get_prot_address failed, fru: " << slot_id << std::endl;
    return FW_STATUS_FAILURE;
  }
  prot::ProtDevice prot_dev(slot_id, dev_i2c_bus, dev_i2c_addr);
  if (!prot_dev.isDevOpen()) {
    return FW_STATUS_FAILURE;
  }

  try {
    cerr << "Checking if the server is ready..." << endl;
    server.ready_except();
  } catch(const std::exception& err) {
    cout << err.what() << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }

  if (image.empty() && fd < 0) {
    cerr << "File or fd is required." << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }

  if(isBypass) {
    cerr << "PRoT is Bypaas mode , try to off server first" << endl;
    if(attempt_server_power_off(force)) {
      cerr << "Failed to Power Off Server " << slot_id << ". Stopping the update!" << endl;
      return FW_STATUS_FAILURE;
    }
  } else {
    try {
      // --force to force recovery, updating to SPIA working area directly
      if(!force) {
        pfr_update = checkPfrUpdate(prot_dev);
      }
    } catch(const runtime_error& err) {
      cerr << "XFR Error: "<< err.what() <<  endl;
      return FW_STATUS_FAILURE;
    }

    if (pfr_update) {
      if(prot_dev.portRequestRecoverySpiUnlock() != prot::ProtDevice::DevStatus::SUCCESS) {
        return FW_STATUS_FAILURE;
      }
    }
  }

  if (!image.empty()) {
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), force);
  } else {
    ret = bic_update_fw_fd(slot_id, fw_comp, fd, force);
  }

  if (ret != 0) {
    cerr << "PRoT update failed. ret = " << ret << endl;
  }

  sleep(1);
  if(isBypass) {
    cerr << "Power-cycling the server..." << endl;
    pal_set_server_power(slot_id, SERVER_POWER_CYCLE);
  } else {
    if (pfr_update) {
      uint8_t status;

      cerr << "Sending XFR UPDATE_COMPLTE CMD..."<< endl;
      prot_dev.protUpdateComplete();
      cerr << "Sending XFR FW_UPDATE_INTENT ..."<< endl;
      prot_dev.protFwUpdateIntent();
      if (prot_dev.protGetUpdateStatus(status) != prot::ProtDevice::DevStatus::SUCCESS) {
        cerr << "Failed to get PRoT PFR update status"<< endl;
        return FW_STATUS_FAILURE;
      }
      cerr << "XFR update status : "<< prot::ProtUpdateStatus::updateStatusString(status) << endl;
      if (status) {
        cerr << "PRoT PFR update failed"<< endl;
        return FW_STATUS_FAILURE;
      }
    }
  }
  return ret;
}

int ProtComponent::update(const string image) {
  return update_internal(image, -1, false /* force */);
}

int ProtComponent::update(int fd, bool force) {
  return update_internal("", fd, force);
}

int ProtComponent::fupdate(const string image) {
  return update_internal(image, -1, true /* force */);
}

int ProtComponent::get_ver_str(string& s) {
  if (isBypass) {
    throw runtime_error("Not support to get version of Bypass Firmware");
  }

  auto ver_key = fmt::format("slot{}_{}_ver", slot_id, component());

  try {
    s = kv::get(ver_key);
    return FW_STATUS_SUCCESS;
  } catch (const std::exception& err) {
    //do nothing, if exception in get kv, read ver from PRoT device
  }

  uint8_t dev_i2c_bus;
  uint8_t dev_i2c_addr;
  if (pal_get_prot_address(slot_id, &dev_i2c_bus, &dev_i2c_addr) != 0) {
    cerr << "pal_get_prot_address failed, fru: " << slot_id << std::endl;
    return FW_STATUS_FAILURE;
  }
  prot::ProtDevice prot_dev(slot_id, dev_i2c_bus, dev_i2c_addr);
  if (!prot_dev.isDevOpen()) {
    return FW_STATUS_FAILURE;
  }

  prot::XFR_VERSION_READ_ACK_PAYLOAD prot_ver{};
  if (prot_dev.protReadXfrVersion(prot_ver) != prot::ProtDevice::DevStatus::SUCCESS) {
    return FW_STATUS_FAILURE;
  }

  s = fmt::format("Tektagon XFR : {}, SFB :{}, CFG :{}",
      prot::ProtVersion::getVerString(prot_ver.Active.XFRVersion),
      prot::ProtVersion::getVerString(prot_ver.Active.SFBVersion),
      prot::ProtVersion::getVerString(prot_ver.Active.CFGVersion));

  try {
    kv::set(ver_key, s);
  } catch (const std::exception& err) {
    syslog(LOG_WARNING,"%s: kv_set failed, key: %s, val: %s", __func__, ver_key.c_str(), s.c_str());
  }

  return FW_STATUS_SUCCESS;
}

int ProtComponent::print_version() {
  string ver("");
  try {
    server.ready_except();
    if ( get_ver_str(ver) < 0 ) {
      throw runtime_error("Error in getting the version of PRoT");
    }
    cout << "PRoT Version: " << ver << endl;
  } catch(const runtime_error& err) {
    cout << "PRoT Version: NA (" << err.what() << ")" << endl;
  }

  return FW_STATUS_SUCCESS;
}

int ProtComponent::get_version(json& j) {
  string ver("");
  try {
    server.ready_except();
    if ( get_ver_str(ver) < 0 ) {
      throw runtime_error("Error in getting the version of PRoT");
    }
    j["VERSION"] = ver;
  } catch(const runtime_error& err) {
    if ( string(err.what()).find("empty") != string::npos )
      j["VERSION"] = "not_present";
    else
      j["VERSION"] = "error_returned";
  }

  return FW_STATUS_SUCCESS;
}

#endif
