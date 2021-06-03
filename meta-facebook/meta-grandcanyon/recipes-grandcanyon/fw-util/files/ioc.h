#ifndef _IOC_H_
#define _IOC_H_

#include <cstdio>
#include <string>
#include <cstring>
#include <openbmc/pal.h>
#include <facebook/fbgc_common.h>
#include "fw-util.h"

using namespace std;

class IOCComponent : public Component {
  public:
    IOCComponent(string fru, string comp)
      : Component(fru, comp) {}
    int print_version();
};

#endif

