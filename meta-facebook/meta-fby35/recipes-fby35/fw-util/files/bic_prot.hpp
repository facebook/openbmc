#ifndef BIC_PROT_H_
#define BIC_PROT_H_
#include "fw-util.h"
#include "server.h"
#include "bic_fw.h"
#include <openbmc/pal.h>
#include <openbmc/ProtCommonInterface.hpp>
#include <facebook/bic_xfer.h>

using std::string;

class ProtComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  Server server;
  bool isBypass= false;
  private:
    bool checkPfrUpdate(prot::ProtDevice& prot_dev);
    int get_ver_str(std::string& s);
    int update_internal(const std::string& image, int fd, bool force);
  public:
    ProtComponent(const std::string& fru, const std::string& comp, uint8_t _fw_comp)
      : Component(fru, comp), slot_id(fru.at(4) - '0'), fw_comp(_fw_comp), server(slot_id, fru),
       isBypass(bic_is_prot_bypass(slot_id)) {}
    int update(std::string image) override;
    int update(int fd, bool force) override;
    int fupdate(std::string image) override;
    int print_version() override;
    int get_version(json& j) override;
};

#endif
