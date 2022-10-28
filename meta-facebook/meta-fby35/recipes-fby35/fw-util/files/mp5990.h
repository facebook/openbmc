#ifndef MP5990_H_
#define MP5990_H_
#include "fw-util.h"
#include "server.h"
#include "bic_fw.h"

using std::string;

#define MP5990_REG_CHECKSUM 0xF8
#define IS_BOARD_REV_MPS(board_rev) (board_rev == SB_REV_EVT_MPS) || (board_rev == SB_REV_DVT_MPS) \
              || (board_rev == SB_REV_PVT_MPS) || (board_rev == SB_REV_MP_MPS) ? true : false

class MP5990Component : public Component {
  uint8_t fru_id = 0;
  uint8_t bus = 0;
  uint8_t addr = 0;
  Server server; // for checking server is ready to be access
  private:
    int get_ver_str(std::string& s);
  public:
    MP5990Component(const std::string& fru, const std::string& comp, uint8_t _fru_id, uint8_t _bus, uint8_t _addr)
      : Component(fru, comp), fru_id(_fru_id), bus(_bus), addr(_addr), server(_fru_id, fru) {}
    int print_version() override;
    int get_version(json& j) override;
};

#endif
