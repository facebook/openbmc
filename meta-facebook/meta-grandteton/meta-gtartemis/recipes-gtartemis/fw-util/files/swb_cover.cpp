#include "swb_common.hpp"

SwbBicFwComponent acb_bic("acb", "bic", ACB_BIC_BUS, ACB_BIC_EID, BIC_COMP);
SwbBicFwComponent meb_bic("meb", "bic", MEB_BIC_BUS, MEB_BIC_EID, BIC_COMP);
SwbPexFwComponent acb_pesw0("acb", "pesw0", ACB_BIC_BUS, ACB_BIC_EID, PEX0_COMP, 0);
SwbPexFwComponent acb_pesw1("acb", "pesw1", ACB_BIC_BUS, ACB_BIC_EID, PEX1_COMP, 1);
