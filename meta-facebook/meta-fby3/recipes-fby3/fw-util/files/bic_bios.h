#ifndef BIC_BIOS_H_
#define BIC_BIOS_H_
#include "fw-util.h"
#include "server.h"


class BiosComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  Server server;
  private:
    int get_ver_str(std::string& s);
    int update_internal(const std::string &image, int fd, bool force);
  public:
    BiosComponent(std::string fru, std::string comp, uint8_t _slot_id, uint8_t _fw_comp)
      : Component(fru, comp), slot_id(_slot_id), fw_comp(_fw_comp), server(_slot_id, fru) {}
    int update(std::string image) override;
    int update(int fd, bool force) override;
    int fupdate(std::string image) override;
    int print_version() override;
    void get_version(json& j) override;
};

#endif
