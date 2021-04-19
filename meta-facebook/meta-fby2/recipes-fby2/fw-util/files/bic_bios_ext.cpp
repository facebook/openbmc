#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include "bic_bios.h"
#include <openbmc/pal.h>
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
    int print_version() override;
};

int BiosExtComponent::update_internal(const std::string &image, int fd, bool force) {
  int ret;
  uint8_t status;
  int retry_count = 60;

#if defined(CONFIG_FBY2_EP) || defined(CONFIG_FBY2_RC)
  uint8_t server_type = 0xFF;
  uint8_t gs_action = 1;
#endif

  try {
    server.ready();
#if defined(CONFIG_FBY2_EP) || defined(CONFIG_FBY2_RC)
    ret = fby2_get_server_type(slot_id, &server_type);
    if (ret) {
      syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
      return -1;
    }

    uint8_t power_cmd;
    switch (server_type) {
      case SERVER_TYPE_EP:
        power_cmd = SERVER_POWER_OFF;
        break;
      case SERVER_TYPE_RC:
      case SERVER_TYPE_TL:
      default:
        power_cmd = SERVER_GRACEFUL_SHUTDOWN;
        break;
    }
    if (force) power_cmd = SERVER_POWER_OFF;
    syslog(LOG_INFO, "%s: Slot%d power cmd %u", __func__, slot_id, power_cmd);
    pal_set_server_power(slot_id, power_cmd);

power_check:
    //Checking Server Power Status to make sure Server is really Off
    while (retry_count > 0) {
      ret = pal_get_server_power(slot_id, &status);
      if ((ret == 0) && (status == SERVER_POWER_OFF)) {
        break;
      }
      if ((--retry_count) > 0) {
        sleep(1);
      }
    }
    if (retry_count <= 0) {
      switch (server_type) {
        case SERVER_TYPE_EP:
          cerr << "Failed to Power Off Server " << (int)slot_id << ". Stopping the update!" << endl;
          break;
        case SERVER_TYPE_RC:
          if (gs_action == 1) {
            syslog(LOG_WARNING, "%s: Graceful Shutdown failed before BIOS update on RC, using power off instead for slot%u", __func__, slot_id);
            pal_set_server_power(slot_id, SERVER_POWER_OFF);     // Change to power off server due to graceful shutdown failed
            retry_count = 60;
            gs_action = 0;
            goto power_check;
          } else {
            syslog(LOG_WARNING, "%s: Power-off still failed on RC before BIOS update for slot%u", __func__, slot_id);
            cerr << "Failed to Power Off Server " << (int)slot_id << ". Stopping the update!" << endl;
          }
          break;
        case SERVER_TYPE_TL:
        default:
          syslog(LOG_WARNING, "%s: Graceful Shutdown failed before BIOS update on TL for slot%u", __func__, slot_id);
          cerr << "Failed to Power Off Server " << (int)slot_id << ". Stopping the update!" << endl;
          break;
      }
      return -1;
    }
#else
    uint8_t power_cmd = (force ? SERVER_POWER_OFF : SERVER_GRACEFUL_SHUTDOWN);

    pal_set_server_power(slot_id, power_cmd);

    //Checking Server Power Status to make sure Server is really Off
    while (retry_count > 0) {
      ret = pal_get_server_power(slot_id, &status);
      if ((ret == 0) && (status == SERVER_POWER_OFF)) {
        break;
      }
      if ((--retry_count) > 0) {
        sleep(1);
      }
    }
    if (retry_count <= 0) {
      cerr << "Failed to Power Off Server " << (int)slot_id << ". Stopping the update!" << endl;
      return -1;
    }
#endif

#if defined(CONFIG_FBY2_EP) || defined(CONFIG_FBY2_RC)
    if (server_type != SERVER_TYPE_EP && server_type != SERVER_TYPE_RC) {
      cerr << "Putting ME into recovery mode..." << endl;
      me_recovery(slot_id, RECOVERY_MODE);
      sleep(1);
    }
#elif defined(CONFIG_FBY2_ND)
    sleep(1);
#else
    cerr << "Putting ME into recovery mode..." << endl;
    me_recovery(slot_id, RECOVERY_MODE);
    sleep(1);
#endif
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
  } catch(string err) {
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
#if defined(CONFIG_FBY2_EP) || defined(CONFIG_FBY2_RC)
  uint8_t server_type = 0xFF;
  uint8_t gs_action = 1;
#endif

  try {
    server.ready();
#if defined(CONFIG_FBY2_EP) || defined(CONFIG_FBY2_RC)
    ret = fby2_get_server_type(slot_id, &server_type);
    if (ret) {
      syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
      return -1;
    }
    switch (server_type) {
      case SERVER_TYPE_EP:
        pal_set_server_power(slot_id, SERVER_POWER_OFF);
        break;
      case SERVER_TYPE_RC:
        syslog(LOG_INFO, "%s: Start Graceful Shutdown before BIOS dump on RC for slot%u", __func__, slot_id);
        pal_set_server_power(slot_id, SERVER_GRACEFUL_SHUTDOWN);
        break;
      case SERVER_TYPE_TL:
      default:
        syslog(LOG_INFO, "%s: Start Graceful Shutdown before BIOS dump on TL for slot%u", __func__, slot_id);
        pal_set_server_power(slot_id, SERVER_GRACEFUL_SHUTDOWN);
        break;
    }

power_check:
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
      switch (server_type) {
        case SERVER_TYPE_EP:
          cerr << "Failed to Power Off Server " << (int)slot_id << ". Stopping the dump!" << endl;
          break;
        case SERVER_TYPE_RC:
          if (gs_action == 1) {
            syslog(LOG_WARNING, "%s: Graceful Shutdown failed before BIOS dump on RC, using power off instead for slot%u", __func__, slot_id);
            pal_set_server_power(slot_id, SERVER_POWER_OFF);     // Change to power off server due to graceful shutdown failed
            retry_count = 60;
            gs_action = 0;
            goto power_check;
          } else {
            syslog(LOG_WARNING, "%s: Power-off still failed on RC before BIOS dump for slot%u", __func__, slot_id);
            cerr << "Failed to Power Off Server " << (int)slot_id << ". Stopping the dump!" << endl;
          }
          break;
        case SERVER_TYPE_TL:
        default:
          syslog(LOG_WARNING, "%s: Graceful Shutdown failed before BIOS dump on TL for slot%u", __func__, slot_id);
          cerr << "Failed to Power Off Server " << (int)slot_id << ". Stopping the dump!" << endl;
          break;
      }
      return -1;
    }
#else
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
#endif

#if defined(CONFIG_FBY2_EP) || defined(CONFIG_FBY2_RC)
    if (server_type != SERVER_TYPE_EP && server_type != SERVER_TYPE_RC) {
      me_recovery(slot_id, RECOVERY_MODE);
      sleep(1);
    }
#else
    me_recovery(slot_id, RECOVERY_MODE);
    sleep(1);
#endif
    ret = bic_dump_fw(slot_id, DUMP_BIOS, (char *)image.c_str());
    sleep(1);
    pal_set_server_power(slot_id, SERVER_12V_CYCLE);
    sleep(5);
    pal_set_server_power(slot_id, SERVER_POWER_ON);
  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BiosExtComponent::print_version() {
  if (fby2_get_slot_type(slot_id) == SLOT_TYPE_SERVER) {
    BiosComponent::print_version();
  }
  return 0;
}

// Register the BIOS components
BiosExtComponent bios1("slot1", "bios", 1);
BiosExtComponent bios2("slot2", "bios", 2);
BiosExtComponent bios3("slot3", "bios", 3);
BiosExtComponent bios4("slot4", "bios", 4);

#endif
