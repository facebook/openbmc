#ifndef _BIOS_H_
#define _BIOS_H_

#include "fw-util.h"

class BiosComponent : public Component {
  protected:
  std::string _mtd_name;
  std::string _ver_prefix;

  virtual int extract_signature(const char *path, std::string &sig);
  virtual int check_image(const char *path);
  int update(const char *path, uint8_t force);
  public:
    BiosComponent(std::string fru, std::string comp, std::string mtd, std::string verp)
      : Component(fru, comp), _mtd_name(mtd), _ver_prefix(verp) {}
    int update(std::string image);
    int fupdate(std::string image);
    int print_version();
};

#endif
