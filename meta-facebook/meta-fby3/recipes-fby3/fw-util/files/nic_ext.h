#ifndef _NIC_EXT_H_
#define _NIC_EXT_H_
#include "nic.h"


using namespace std;

class NicExtComponent : public NicComponent {
  uint8_t _if_idx;
  int _get_ncsi_vid(void);
  public:
    NicExtComponent(string fru, string board, string comp, uint8_t idx)
      : NicComponent(fru, board, comp), _if_idx(idx) {}
    int print_version();
};

#endif
