#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "server.h"
#include <openbmc/pal.h>
#include <facebook/bic.h>

using namespace std;

class BiosComponent : public Component {
  uint8_t slot_id = 0;
  Server server;
  public:
    BiosComponent(string fru, string comp, uint8_t _slot_id)
      : Component(fru, comp), slot_id(_slot_id), server(_slot_id, fru) {}
    int update(string image) {
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
        pal_set_server_power(slot_id, SERVER_12V_CYCLE);
        sleep(3);
        pal_set_server_power(slot_id, SERVER_POWER_ON);
      } catch(string err) {
        return FW_STATUS_NOT_SUPPORTED;
      }
      return ret;
    }
    int print_version() {
      uint8_t ver[32] = {0};
      int i;

      try {
        server.ready();
        if (pal_get_sysfw_ver(slot_id, ver)) {
          cout << "BIOS Version: NA" << endl;
        } else {
          // BIOS version response contains the length at offset 2 followed by ascii string
          printf("BIOS Version: ");
          for (i = 3; i < 3+ver[2]; i++) {
            printf("%c", ver[i]);
          }
          printf("\n");
        }
      } catch (string err) {
        printf("BIOS Version: NA (%s)\n", err.c_str());
      }
      return 0;
    }
};

BiosComponent bios1("server", "bios", 1);
