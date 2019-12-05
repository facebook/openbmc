#include "bios.h"

AliasComponent bmc("mb", "bmc", "bmc", "bmc");
AliasComponent rom("mb", "rom", "bmc", "rom");
BiosComponent bios("mb", "bios", "\"pnor\"", "F0C_");
