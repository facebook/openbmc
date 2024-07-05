#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "bic_me.h"
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

class MeExtComponent : public MeComponent {
  uint8_t slot_id = 0;
  Server server;
  public:
    MeExtComponent(string fru, string comp, uint8_t _slot_id)
      : MeComponent(fru, comp, _slot_id), slot_id(_slot_id), server(_slot_id, fru) {
      if (!pal_is_slot_server(_slot_id)) {
        (*fru_list)[fru].erase(comp);
        if ((*fru_list)[fru].size() == 0) {
          fru_list->erase(fru);
        }
      }
    }
    int get_version(json& j) override;
};

int MeExtComponent::get_version(json& j) {
  int ret = FW_STATUS_NOT_SUPPORTED;
  if (fby2_get_slot_type(slot_id) == SLOT_TYPE_SERVER) {
#ifdef CONFIG_FBY2_ND
    uint8_t server_type = 0xFF;
    if (fby2_get_server_type(slot_id, &server_type)) {
      return FW_STATUS_FAILURE;
    }
    switch (server_type) {
      case SERVER_TYPE_ND:
        // ND does not have a ME.
        break;
      case SERVER_TYPE_TL:
        ret = MeComponent::get_version(j);
        break;
      default:
        break;
    }
#else
  ret = MeComponent::get_version(j);
#endif
  }
  return ret;
}

// Register the ME components
MeExtComponent me1("slot1", "me", 1);
MeExtComponent me2("slot2", "me", 2);
MeExtComponent me3("slot3", "me", 3);
MeExtComponent me4("slot4", "me", 4);

#endif
