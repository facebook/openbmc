#ifndef _NIC_EXT_H_
#define _NIC_EXT_H_

#include "nic.h"

class NicExtComponent : public NicComponent {
  std::string _vid_key;  // key name in kv store
  uint8_t _fru_id;
  uint8_t _if_idx;
  uint8_t _ch_id;
  int _get_ncsi_vid(void);
  public:
    NicExtComponent(std::string fru, std::string comp, std::string key, uint8_t fruid, uint8_t idx, uint8_t chid = 0x00)
      : NicComponent(fru, comp, "/tmp/cache_store/"+key), _vid_key(key), _fru_id(fruid), _if_idx(idx), _ch_id(chid) {}
    int print_version();
};

#endif
