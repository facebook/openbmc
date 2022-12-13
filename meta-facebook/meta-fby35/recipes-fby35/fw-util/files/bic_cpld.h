#ifndef _BIC_CPLD_H_
#define _BIC_CPLD_H_
#include "fw-util.h"
#include <openbmc/cpld.h>
#include <openbmc/obmc-i2c.h>
#include "server.h"
#include "expansion.h"
#include "bic_fw.h"

using namespace std;

class CpldComponent : public Component {
  uint8_t slot_id;
  uint8_t fw_comp;
  uint8_t pld_type;
  string board;
  i2c_attr_t attr;
  Server server;
  ExpansionBoard expansion;
  private:
    image_info check_image(const string& image, bool force);
    int update_cpld(const string& image, bool force);
    int get_ver_str(string& s);
    int cpld_refresh(uint8_t bus_id, uint8_t addr);
    void cpld_check_device();
  public:
    CpldComponent(const string& fru, const string& comp, const string& brd, uint8_t comp_id, uint8_t type, uint8_t addr)
      : Component(fru, comp), slot_id(fru.at(4) - '0'), fw_comp(comp_id), pld_type(type), board(brd),
        attr{(uint8_t)(slot_id+3), addr, nullptr}, server(slot_id, fru), expansion(slot_id, fru, brd, fw_comp) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
    int get_version(json& j) override;
};

#endif
