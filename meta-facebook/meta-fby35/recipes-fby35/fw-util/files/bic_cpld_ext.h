#ifndef _BIC_CPLD_EXT_H_
#define _BIC_CPLD_EXT_H_
#include "bic_cpld.h"
#include "server.h"
#include "expansion.h"

using namespace std;

class CpldExtComponent : public CpldComponent {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  string name;  
  Server server;
  ExpansionBoard expansion;
  private:
    int get_ver_str(string& s);
  public:
    CpldExtComponent(string fru, string comp, uint8_t _slot_id, string _name, uint8_t _fw_comp)
      : CpldComponent(fru, comp, _slot_id), slot_id(_slot_id), fw_comp(_fw_comp), name(_name), server(_slot_id, fru), expansion(_slot_id, fru, _name, _fw_comp) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
    void get_version(json& j);
};

#endif
