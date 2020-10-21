#include <unistd.h>
#include <openbmc/pal.h>
#include <facebook/asic.h>
#include "fw-util.h"

using namespace std;

class ASICComponent : public Component {
  uint8_t _slot;
  public:
    ASICComponent(string fru, string comp, uint8_t slot)
      : Component(fru, comp), _slot(slot) {}
    int print_version() override;
};

int ASICComponent::print_version()
{
  int lock, ret;
  char ver[16] = {0};

  if (!is_asic_prsnt(_slot))
    return 0;

  cout << "ASIC" << (int)_slot << " Version: ";

  lock = open("/tmp/asic_lock", O_CREAT | O_RDWR, 0666);
  if (lock < 0 || pal_flock_retry(lock) < 0) {
    cout << "Not available" << endl;
    return 0;
  }

  ret = asic_show_version(_slot, ver);

  if (ret == ASIC_SUCCESS)
    cout << string(ver) << endl;
  else if (ret == ASIC_NOTSUP)
    cout << "Not supported" << endl;
  else
    cout << "NA" << endl;

  pal_unflock_retry(lock);
  close(lock);
  return 0;
}

ASICComponent asic0("asic", "slot0", 0);
ASICComponent asic1("asic", "slot1", 1);
ASICComponent asic2("asic", "slot2", 2);
ASICComponent asic3("asic", "slot3", 3);
ASICComponent asic4("asic", "slot4", 4);
ASICComponent asic5("asic", "slot5", 5);
ASICComponent asic6("asic", "slot6", 6);
ASICComponent asic7("asic", "slot7", 7);
