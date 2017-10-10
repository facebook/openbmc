#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "server.h"
#include <openbmc/pal.h>
#include <facebook/bic.h>

using namespace std;

class BiosComponent : public Component {
  uint8_t slot_id;
  Server server;
  public:
    BiosComponent(string fru, string comp, uint8_t _slot_id)
      : Component(fru, comp), slot_id(_slot_id), server(_slot_id) {}
    int update(string image) {
      char cmd[80];
      int ret;
      uint8_t status;
      int retry_count = 0;

      if (!server.ready()) {
        return FW_STATUS_NOT_SUPPORTED;
      }
      memset(cmd, 0, sizeof(cmd));
      sprintf(cmd, "/usr/local/bin/power-util slot%u graceful-shutdown", slot_id);
      system(cmd);

      //Checking Server Power Status to make sure Server is really Off
      while (retry_count < 20){
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
      memset(cmd, 0, sizeof(cmd));
      sprintf(cmd, "/usr/local/bin/power-util slot%u 12V-cycle", slot_id);
      system(cmd);
      sleep(5);
      memset(cmd, 0, sizeof(cmd));
      sprintf(cmd, "/usr/local/bin/power-util slot%u on", slot_id);
      system(cmd);
      return ret;
    }
    int print_version() {
      uint8_t ver[32] = {0};
      int i;

      if (!server.ready()) {
        return FW_STATUS_NOT_SUPPORTED;
      }
      if (pal_get_sysfw_ver(slot_id, ver)) {
        cout << "BIOS Version: NA" << endl;
      }
      else {
        // BIOS version response contains the length at offset 2 followed by ascii string
        printf("BIOS Version: ");
        for (i = 3; i < 3+ver[2]; i++) {
          printf("%c", ver[i]);
        }
        printf("\n");
      }
      return 0;
    }
};

BiosComponent bios1("slot1", "bios", 1);
BiosComponent bios2("slot2", "bios", 2);
BiosComponent bios3("slot3", "bios", 3);
BiosComponent bios4("slot4", "bios", 4);
