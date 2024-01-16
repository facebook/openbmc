#include "fw-util.h"
#include <unistd.h>
#include "bic_bios.h"
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int BiosComponent::update(string image) {
  int ret;
  uint8_t status;
  int retry_count = 0;

  try {
    server.ready();
    pal_set_server_power(slot_id, SERVER_GRACEFUL_SHUTDOWN);

    //Checking Server Power Status to make sure Server is really Off
    while (retry_count < 20) {
      ret = pal_get_server_power(slot_id, &status);
      if ( (ret == 0) && (status == SERVER_POWER_OFF) ){
        break;
      }
      else{
        retry_count++;
        sleep(1);
      }
    }
    if (retry_count == 20) {
      cerr << "Failed to Power Off Server " << slot_id << ". Stopping the update!" << endl;
      return -1;
    }

    me_recovery(slot_id, RECOVERY_MODE);
    sleep(1);
    ret = bic_update_fw(slot_id, UPDATE_BIOS, (char *)image.c_str());
    sleep(1);
    pal_set_server_power(slot_id, SERVER_GRACEFUL_POWER_ON);
  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int BiosComponent::fupdate(string image) {
  return FW_STATUS_NOT_SUPPORTED;
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
  } catch (string err) {
    j["VERSION"] = "NA (" + err + ")";
  }
  return 0;
}
#endif
