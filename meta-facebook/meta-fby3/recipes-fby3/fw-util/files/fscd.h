#ifndef _FSCD_H_
#define _FSCD_H_
#include "fw-util.h"

using namespace std;

class FscdComponent : public Component {
  public:
  FscdComponent(string fru, string board, string comp)
    : Component(fru, board, comp) {}
  int print_version();
};

#endif
