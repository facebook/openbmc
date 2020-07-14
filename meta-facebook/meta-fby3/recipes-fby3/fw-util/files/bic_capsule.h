#include <string>
#include "fw-util.h"
#include "server.h"
#include "bmc_cpld.h"

using namespace std;

class CapsuleComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  Server server;
  private:
    int get_pfr_recovery_ver_str(string& s);
    image_info check_image(string image, bool force);
    int bic_update_capsule(string image);
  public:
    CapsuleComponent(string fru, string comp, uint8_t _slot_id, uint8_t _fw_comp)
      : Component(fru, comp), slot_id(_slot_id), fw_comp(_fw_comp), server(_slot_id, fru) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
};
