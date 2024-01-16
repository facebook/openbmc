#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include "bic_bios.h"
#include <openbmc/pal.h>
#include <openbmc/misc-utils.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

class BiosExtComponent : public BiosComponent {
  uint8_t slot_id;
  Server server;
  int update_internal(const std::string &image, int fd, bool force);
  public:
    BiosExtComponent(string fru, string comp, uint8_t _slot_id)
      : BiosComponent(fru, comp, _slot_id), slot_id(_slot_id), server(_slot_id, fru) {
      if (!pal_is_slot_server(_slot_id)) {
        (*fru_list)[fru].erase(comp);
        if ((*fru_list)[fru].size() == 0) {
          fru_list->erase(fru);
        }
      }
    }
    int update(string image) override;
    int update(int fd, bool force) override;
    int fupdate(string image) override;
    int dump(string image) override;
    int get_version(json& j) override;
};

int BiosExtComponent::update_internal(const std::string &image, int fd, bool force) {
  int ret;
  uint8_t status = SERVER_POWER_ON;

  try {
    server.ready();

    //Checking Server Power Status to make sure Server is really Off
    if (!force) {
      cerr << "Powering off the server gracefully" << endl;
      pal_set_server_power(slot_id, SERVER_GRACEFUL_SHUTDOWN);
      if (retry_cond(!pal_get_server_power(slot_id, &status) && status == SERVER_POWER_OFF, 5*60, 1000)) {
        cerr << "Retry powering off the server..." << endl;
        pal_set_server_power(slot_id, SERVER_POWER_OFF);
        if (retry_cond(!pal_get_server_power(slot_id, &status) && status == SERVER_POWER_OFF, 1, 1000)) {
          cerr << "Failed to Power Off Server " << (int)slot_id << ". Stopping the update!" << endl;
          return -1;
        }
      }
    } else {
      cerr << "Powering off the server forcefully" << endl;
      pal_set_server_power(slot_id, SERVER_POWER_OFF);
      if (retry_cond(!pal_get_server_power(slot_id, &status) && status == SERVER_POWER_OFF, 1, 1000)) {
        cerr << "Failed to Power Off Server " << (int)slot_id << ". Stopping the update!" << endl;
        return -1;
      }
    }

#if !defined(CONFIG_FBY2_ND)
    cerr << "Putting ME into recovery mode..." << endl;
    me_recovery(slot_id, RECOVERY_MODE);
#endif
    sleep(1);

    if (!image.empty()) {
      ret = bic_update_firmware(slot_id, UPDATE_BIOS, (char *)image.c_str(), force);
    } else {
      ret = bic_update_firmware_fd(slot_id, UPDATE_BIOS, fd, force);
    }
    sleep(1);
    cerr << "Power-cycling the server..." << endl;
    pal_set_server_power(slot_id, SERVER_12V_CYCLE);
    sleep(5);
    pal_set_server_power(slot_id, SERVER_POWER_ON);
  } catch(exception& err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BiosExtComponent::update(string image) {
  return update_internal(image, -1, false /* force */);
}

int BiosExtComponent::update(int fd, bool force) {
  return update_internal("", fd, force);
}

int BiosExtComponent::fupdate(string image) {
  return update_internal(image, -1, true /* force */);
}

int BiosExtComponent::dump(string image) {
  int ret;
  uint8_t status;
  int retry_count = 60;

  try {
    server.ready();
    pal_set_server_power(slot_id, SERVER_GRACEFUL_SHUTDOWN);

    //Checking Server Power Status to make sure Server is really Off
    while (retry_count > 0) {
      ret = pal_get_server_power(slot_id, &status);
      if ((ret == 0) && (status == SERVER_POWER_OFF)) {
        break;
      }
      if ((--retry_count) > 0)
        sleep(1);
    }
    if (retry_count <= 0) {
      cerr << "Failed to Power Off Server " << (int)slot_id << ". Stopping the dump!" << endl;
      return -1;
    }

    me_recovery(slot_id, RECOVERY_MODE);
    sleep(1);

    ret = bic_dump_fw(slot_id, DUMP_BIOS, (char *)image.c_str());
    sleep(1);
    pal_set_server_power(slot_id, SERVER_12V_CYCLE);
    sleep(5);
    pal_set_server_power(slot_id, SERVER_POWER_ON);
  } catch(exception& err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BiosExtComponent::get_version(json& j) {
  if (fby2_get_slot_type(slot_id) == SLOT_TYPE_SERVER) {
    return BiosComponent::get_version(j);
  }
  return FW_STATUS_NOT_SUPPORTED;
}

// Register the BIOS components
BiosExtComponent bios1("slot1", "bios", 1);
BiosExtComponent bios2("slot2", "bios", 2);
BiosExtComponent bios3("slot3", "bios", 3);
BiosExtComponent bios4("slot4", "bios", 4);

#endif
