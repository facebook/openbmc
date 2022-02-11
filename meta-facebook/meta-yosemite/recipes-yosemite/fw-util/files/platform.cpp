#include "bic_fw.h"
#include "bic_bios.h"
#include "bic_me.h"
#include "bic_cpld.h"
#include "nic.h"

// Shared NIC
NicComponent nic("nic", "nic");

// Register BIC FW and BL components
BicFwComponent bicfw1("slot1", "bic", 1);
BicFwComponent bicfw2("slot2", "bic", 2);
BicFwComponent bicfw3("slot3", "bic", 3);
BicFwComponent bicfw4("slot4", "bic", 4);
BicFwBlComponent bicfwbl1("slot1", "bicbl", 1);
BicFwBlComponent bicfwbl2("slot2", "bicbl", 2);
BicFwBlComponent bicfwbl3("slot3", "bicbl", 3);
BicFwBlComponent bicfwbl4("slot4", "bicbl", 4);

// Register the BIOS components
BiosComponent bios1("slot1", "bios", 1);
BiosComponent bios2("slot2", "bios", 2);
BiosComponent bios3("slot3", "bios", 3);
BiosComponent bios4("slot4", "bios", 4);

// Register the ME components
MeComponent me1("slot1", "me", 1);
MeComponent me2("slot2", "me", 2);
MeComponent me3("slot3", "me", 3);
MeComponent me4("slot4", "me", 4);

// Register CPLD components
CpldComponent cpld1("slot1", "cpld", 1);
CpldComponent cpld2("slot2", "cpld", 2);
CpldComponent cpld3("slot3", "cpld", 3);
CpldComponent cpld4("slot4", "cpld", 4);
