#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include "bios.h"
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int BiosComponent::update(string image) {
  return FW_STATUS_NOT_SUPPORTED;
}

int BiosComponent::fupdate(string image) {
  return FW_STATUS_NOT_SUPPORTED;
}

int BiosComponent::print_version() {
  uint8_t ver[32];
  uint8_t fruid = 0;
  int ret = 0;
  try {
    server.ready();
    ret = pal_get_fru_id((char *)_fru.c_str(), &fruid);
    if ( ret < 0 ) {
      throw "get " + _fru + " fru id failed";
    }

    ret = pal_get_sysfw_ver(fruid, ver);
    // Print BIOS Version
    if ( ret < 0 ) {
      cout << "BIOS Version: NA" << endl;
    } else {
      printf("BIOS Version: ");
      cout << &ver[3] << endl;
    }
  } catch(string err) {
    printf("BIOS Version: NA (%s)\n", err.c_str());
  }

  return ret;
}

#endif
