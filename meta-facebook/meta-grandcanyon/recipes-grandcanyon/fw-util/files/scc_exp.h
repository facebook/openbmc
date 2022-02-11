#ifndef _SCC_EXP_H_
#define _SCC_EXP_H_

#include <cstdio>
#include <string>
#include <openbmc/pal.h>
#include <facebook/fbgc_common.h>
#include "fw-util.h"

#define FW_VERSION_BYTES 4

using namespace std;

class ExpanderComponent : public Component {
  public:
    ExpanderComponent(string fru, string comp)
      : Component(fru, comp) {}
    int print_version();
};

#endif

