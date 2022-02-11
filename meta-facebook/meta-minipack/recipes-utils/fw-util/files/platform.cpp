#include "bic_fw.h"
#include "bic_me.h"
#include "bic_cpld.h"
#include "bic_bios.h"

/* Register BIC FW and BL components */
BicFwComponent bicfw("scm", "bic", 0);
BicFwBlComponent bicfwbl("scm", "bicbl", 0);

/* Register ME components */
MeComponent me("scm", "me", 0);

/* Register CPLD components */
CpldComponent uservercpld("scm", "uservercpld", 0);
