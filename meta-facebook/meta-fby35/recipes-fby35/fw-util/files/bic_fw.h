#ifndef _BIC_FW_H_
#define _BIC_FW_H_
#include "fw-util.h"
#include "server.h"
#include "expansion.h"

using std::string;

enum {
  BIC_FW_UPDATE_SUCCESS                = 0,
  BIC_FW_UPDATE_FAILED                 = -1,
  BIC_FW_UPDATE_NOT_SUPP_IN_CURR_STATE = -2,
};

typedef struct image_check {
  string new_path;
  bool result;
  bool sign;
} image_info;

class BicFwComponent : public Component {
  uint8_t slot_id;
  uint8_t fw_comp;
  string board;
  Server server;
  ExpansionBoard expansion;
  private:
    image_info check_image(const string& image, bool force);
    int update_internal(const string& image, bool force);
    int get_ver_str(string& s);
  public:
    BicFwComponent(const string& fru, const string& comp, const string& brd, uint8_t comp_id)
      : Component(fru, comp), slot_id(fru.at(4) - '0'), fw_comp(comp_id), board(brd),
        server(slot_id, fru), expansion(slot_id, fru, brd, fw_comp) {}
    int update(string image);
    int fupdate(string image);
    int print_version();
    int get_version(json& j) override;
    bool is_recovery();
};

#endif
