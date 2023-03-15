#pragma once
#include <openbmc/obmc-i2c.h>
#include <openbmc/cpld.h>
#include "fw-util.h"
#include "signed_decoder.hpp"
#include "signed_info.hpp"

class CpldComponent : public Component {
  uint8_t pld_type;
  i2c_attr_t attr;
  int _update(const char *path, uint8_t is_signed, i2c_attr_t attr);

  public:
    CpldComponent(const std::string &fru, const std::string &comp, uint8_t type, uint8_t bus, uint8_t addr,
      int (*cpld_xfer)(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t))
      : Component(fru, comp), pld_type(type), attr{bus, addr, cpld_xfer} {}
    int update(std::string image) override;
    int fupdate(std::string image) override;
    int get_version(json& j) override;
};

class GTCpldComponent : public CpldComponent, public SignComponent {
  public:
    GTCpldComponent(const std::string &fru, const std::string &comp,
      uint8_t type, uint8_t bus, uint8_t addr, int (*cpld_xfer)(uint8_t, uint8_t, uint8_t *, uint8_t, uint8_t *, uint8_t),
      signed_header_t sign_info): CpldComponent(fru, comp, type, bus, addr, cpld_xfer),
      SignComponent(sign_info, fru) {}
    int update(std::string image) override;
    int fupdate(std::string image) override;
    int component_update(std::string image, bool force) {
      if (force)
        return CpldComponent::fupdate(image);
      else
        return CpldComponent::update(image);
    }
};

int cpld_pldm_wr(uint8_t bus, uint8_t addr, uint8_t *txbuf, uint8_t txlen, uint8_t *rxbuf, uint8_t rxlen);