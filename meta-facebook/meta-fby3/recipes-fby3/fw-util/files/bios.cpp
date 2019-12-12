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
  int i, end;

  try {
    //TODO The function is not ready, we skip it now.
    //server.ready();

    // Print BIOS Version
    pal_get_fru_id((char *)_fru.c_str(), &fruid);
    if (!pal_get_sysfw_ver(fruid, ver)) {
      // BIOS version response contains the length at offset 2 followed by ascii string
      if ((end = 3+ver[2]) > (int)sizeof(ver)) {
        end = sizeof(ver);
      }

      printf("BIOS Version: ");
      for (i = 3; i < end; i++) {
        printf("%c", ver[i]);
      }
      printf("\n");
    } else {
      cout << "BIOS Version: NA" << endl;
    }
  } catch(string err) {
    printf("BIOS Version: NA (%s)\n", err.c_str());
  }
  return 0;
}

#endif
