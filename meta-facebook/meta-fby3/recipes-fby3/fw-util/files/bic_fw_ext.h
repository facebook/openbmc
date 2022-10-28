#ifndef _BIC_FW_EXT_H_
#define _BIC_FW_EXT_H_
#include "bic_fw.h"
#include "server.h"
#include "expansion.h"

using namespace std;

enum {
  BIC_FW_UPDATE_SUCCESS               = 0,
  BIC_FW_UPDATE_FAILED                = -1,
  BIC_FW_UPDATE_NOT_SUPP_IN_CURR_STATE = -2,
};

class BicFwExtComponent : public BicFwComponent {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  string name;  
  Server server;
  ExpansionBoard expansion;
  private:
    int get_ver_str(string& ver);
    int update_internal(string image, bool force);
  public:
    BicFwExtComponent(string fru, string comp, uint8_t _slot_id, string _name, uint8_t _fw_comp)
      : BicFwComponent(fru, comp, _slot_id), slot_id(_slot_id), fw_comp(_fw_comp), name(_name), server(_slot_id, fru), expansion(_slot_id, fru, _name, _fw_comp) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
    int get_version(json& j) override;
};

class BicFwExtBlComponent : public BicFwBlComponent {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  string name;
  Server server;
  ExpansionBoard expansion;
  private:
    int get_ver_str(string& ver);
    int update_internal(string image, bool force);
  public:
    BicFwExtBlComponent(string fru, string comp, uint8_t _slot_id, string _name, uint8_t _fw_comp)
      : BicFwBlComponent(fru, comp, _slot_id), slot_id(_slot_id), fw_comp(_fw_comp), name(_name), server(_slot_id, fru), expansion(_slot_id, fru, _name, _fw_comp) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
    int get_version(json& j) override;
};

#endif
