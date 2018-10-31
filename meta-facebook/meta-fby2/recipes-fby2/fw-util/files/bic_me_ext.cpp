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
      : MeComponent(fru, comp, _slot_id), slot_id(_slot_id), server(_slot_id, fru) {}
    int print_version();
};

int MeExtComponent::print_version() {
  if (fby2_get_slot_type(slot_id) == SLOT_TYPE_SERVER) {
    MeComponent::print_version();
  }
  return 0;
}

// Register the ME components
MeExtComponent me1("slot1", "me", 1);
MeExtComponent me2("slot2", "me", 2);
MeExtComponent me3("slot3", "me", 3);
MeExtComponent me4("slot4", "me", 4);

#endif
