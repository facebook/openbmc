#include "fw-util.h"
#include <openbmc/pal.h>

// Set aliases for BMC components to MB components
Component bmc("mb", "bmc", "bmc", "bmc");
Component rom("mb", "rom", "bmc", "rom");
Component mb_fscd("mb", "fscd", "bmc", "fscd");

