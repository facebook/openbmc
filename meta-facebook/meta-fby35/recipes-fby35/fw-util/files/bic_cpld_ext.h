#ifndef _BIC_CPLD_EXT_H_
#define _BIC_CPLD_EXT_H_
#include "fw-util.h"
#include "server.h"
#include "expansion.h"

using namespace std;

typedef struct img_check {
  std::string new_path;
  bool result;
  bool sign;
} img_info;

class CpldExtComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  string name;  
  Server server;
  ExpansionBoard expansion;
  private:
    int get_ver_str(string& s);
  public:
    CpldExtComponent(const string& fru, const string& comp, uint8_t _slot_id, const string& _name, uint8_t _fw_comp)
      : Component(fru, comp), slot_id(_slot_id), fw_comp(_fw_comp), name(_name), server(_slot_id, fru), expansion(_slot_id, fru, _name, _fw_comp) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
    void get_version(json& j);
    bool is_valid_image(string image);
    img_info check_image(string image, bool force);
    int update_cpld(string image, bool force);
};

#endif
