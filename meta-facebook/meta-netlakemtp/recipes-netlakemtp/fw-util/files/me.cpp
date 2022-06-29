#include "fw-util.h"
#include <facebook/netlakemtp_common.h>
#include <iostream>
#include <iomanip>

using namespace std;

class MeComponent : public Component {
  public:
    MeComponent(string fru, string comp)
      : Component(fru, comp) {}
    int print_version() {
      int ret = 0;
      ipmi_dev_id_t dev_id;
      // Print ME Version
      ret = netlakemtp_common_me_get_fw_ver(&dev_id);
      if (ret < 0){
        cout << "ME Version: NA\n";
      } else {
	     cout << "ME Version: SPS_" << setfill('0') << setw(2) << hex << (dev_id.fw_rev1 & 0xff)
	     << "." << setfill('0') << setw(2) << hex << (dev_id.fw_rev2 >> 4) << "."
	     << setfill('0') << setw(2) << hex << (dev_id.fw_rev2 & 0x0f) << "."
	     << setfill('0') << setw(2) << hex << (dev_id.aux_fw_rev[1] & 0xff)
	     << hex << (dev_id.aux_fw_rev[2] >> 4) << "."
	     << hex << (dev_id.aux_fw_rev[2] & 0x0F) << "\n";
      }
      return 0;
    }
};

MeComponent me("server", "me");
