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
  uint8_t slot_id = 0;
  Server server;
  public:
    BiosExtComponent(string fru, string comp, uint8_t _slot_id)
      : BiosComponent(fru, comp, _slot_id), slot_id(_slot_id), server(_slot_id, fru) {}
    int update(string image);
    int dump(string image);
};

int BiosExtComponent::update(string image) {
  int ret;
  uint8_t status;
  int retry_count = 60;
#if defined(CONFIG_FBY2_EP)
  uint8_t server_type = 0xFF;
#endif

  try {
    server.ready();
#if defined(CONFIG_FBY2_EP)
    ret = fby2_get_server_type(slot_id, &server_type);
    if (ret) {
      syslog(LOG_ERR, "%s, Get server type failed\n", __func__);
      return -1;
    }
    switch (server_type) {
      case SERVER_TYPE_EP:
        pal_set_server_power(slot_id, SERVER_POWER_OFF);
        break;
      default:
        pal_set_server_power(slot_id, SERVER_GRACEFUL_SHUTDOWN);
        break;
    }
#else
    pal_set_server_power(slot_id, SERVER_GRACEFUL_SHUTDOWN);
#endif

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
      cerr << "Failed to Power Off Server " << (int)slot_id << ". Stopping the update!" << endl;
      return -1;
    }

#if defined(CONFIG_FBY2_EP)
    if (server_type != SERVER_TYPE_EP) {
      me_recovery(slot_id, RECOVERY_MODE);
      sleep(1);
    }
#else
    me_recovery(slot_id, RECOVERY_MODE);
    sleep(1);
#endif
    ret = bic_update_fw(slot_id, UPDATE_BIOS, (char *)image.c_str());
    sleep(1);
    pal_set_server_power(slot_id, SERVER_12V_CYCLE);
    sleep(5);
    pal_set_server_power(slot_id, SERVER_POWER_ON);
  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
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
  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

BiosExtComponent bios1("slot1", "bios", 1);
BiosExtComponent bios2("slot2", "bios", 2);
BiosExtComponent bios3("slot3", "bios", 3);
BiosExtComponent bios4("slot4", "bios", 4);

#endif
