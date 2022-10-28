#ifndef BIC_CXL_H_
#define BIC_CXL_H_
#include "fw-util.h"
#include "server.h"
#include "bic_fw.h"

using std::string;

class CxlComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  Server server;
  ExpansionBoard expansion;
  private:
    int get_ver_str(std::string& s);
    int update_internal(const std::string& image, int fd, bool force);
  public:
    CxlComponent(const std::string& fru, const std::string& comp, const string& brd, uint8_t _fw_comp)
      : Component(fru, comp), slot_id(fru.at(4) - '0'), fw_comp(_fw_comp), server(slot_id, fru),
       expansion(slot_id, fru, brd, fw_comp) {}
    int update(std::string image) override;
    int update(int fd, bool force) override;
    int fupdate(std::string image) override;
    int print_version() override;
    int get_version(json& j) override;
};

#endif
