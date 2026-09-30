#ifndef _SCC_EXP_H_
#define _SCC_EXP_H_

#include <cstdio>
#include <string>
#include <openbmc/pal.h>
#include <facebook/fbgc_common.h>
#include "fw-util.h"

#ifdef CONFIG_GRANDCANYON2
#define FW_VERSION_BYTES 5
#else
#define FW_VERSION_BYTES 4
#endif

using namespace std;

class ExpanderComponent : public Component {
  public:
    ExpanderComponent(string fru, string comp)
      : Component(fru, comp) {}
    int print_version();
    int get_version(json& j) override;
};

#endif

