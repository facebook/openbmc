#include <cstdio>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/pal.h>
#include <openbmc/ProtCommonInterface.hpp>
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

int ProtComponent::update_internal(const std::string& image, int fd, bool force) {
  int ret;
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
    cerr << "PRoT is PFR mode , update to recovery Flash" << endl;
    prot::BOOT_STATUS_ACK_PAYLOAD boot_sts{};
    if (prot_dev.portGetBootStatus(boot_sts) == prot::ProtDevice::DevStatus::SUCCESS) {
      if (static_cast<prot::ProtSpiInfo::SpiStatus>(boot_sts.SPI_A) == prot::ProtSpiInfo::SpiStatus::ACTIVE) {
        cerr << "SPI_A is Active, updating SPI_B..." << endl;
        fw_comp = FW_PROT_SPIB;
      } else {
        cerr << "SPI_B is Active, updating SPI_A..." << endl;
      }
    }
    if(prot_dev.portRequestRecoverySpiUnlock() != prot::ProtDevice::DevStatus::SUCCESS) {
      return FW_STATUS_FAILURE;
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
    uint8_t status;

    prot_dev.protUpdateComplete();
    prot_dev.protFwUpdateIntent();
    if (prot_dev.protGetUpdateStatus(status) != prot::ProtDevice::DevStatus::SUCCESS || status !=0 ) {
      cerr << "PRoT PFR update failed"<< endl;
      return FW_STATUS_FAILURE;
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

int ProtComponent::get_ver_str(string&) {
  uint8_t fruid = 0;
  int ret = 0;

  ret = pal_get_fru_id((char *)_fru.c_str(), &fruid);
  if ( ret < 0 ) {
    syslog(LOG_WARNING, "Failed to get fru id");
    throw runtime_error("Error in getting fru id");
  }
  throw runtime_error("Not support to get version of Bypass Firmware");

  return FW_STATUS_NOT_SUPPORTED;
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
