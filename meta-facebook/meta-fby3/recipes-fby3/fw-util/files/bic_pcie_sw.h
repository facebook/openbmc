#ifndef _BIC_PCIE_SW_H_
#define _BIC_PCIE_SW_H_
#include "fw-util.h"
#include "server.h"
#include "expansion.h"
#include <string>

using namespace std;

class PCIESWComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  string name;  
  Server server;
  ExpansionBoard expansion;
  private:
    int get_ver_str(string& s, const uint8_t alt_fw_comp, const uint8_t board_type);
    int uart_update(string image);
    int update_internal(string& image, bool force);
  public:
    PCIESWComponent(string fru, string comp, uint8_t _slot_id, string _name, uint8_t _fw_comp)
      : Component(fru, comp), slot_id(_slot_id), fw_comp(_fw_comp), name(_name), server(_slot_id, fru), expansion(_slot_id, fru, _name, _fw_comp) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
};

#endif
