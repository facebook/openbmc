#include "swb_common.hpp"

SwbBicFwComponent bic("swb", "bic", 3, 0x0A, BIC_COMP);
SwbBicFwRecoveryComponent bic_recovery("swb", "bic_recovery", 3, 0x0A, BIC_COMP);
SwbPexFwComponent swb_pex0("swb", "pex0", 3, 0x0A, PEX0_COMP, 0);
SwbPexFwComponent swb_pex1("swb", "pex1", 3, 0x0A, PEX1_COMP, 1);
SwbPexFwComponent swb_pex2("swb", "pex2", 3, 0x0A, PEX2_COMP, 2);
SwbPexFwComponent swb_pex3("swb", "pex3", 3, 0x0A, PEX3_COMP, 3);

